#include <WiFi.h>
#include <HTTPClient.h>

class ICSCalendar 
{
  String icsData;
  String lastData;   // 🆕 pro detekci změny

private:
  char lastWeek[7][3][64];   // max 63 znaků + \0
  bool weekInitialized = false;  

public:

  void safeCopy(char* dest, const String& src)
  {
    strncpy(dest, src.c_str(), 63);
    dest[63] = '\0';
  }

  bool isWeekChanged()
  {
    struct tm t;
    if (!getLocalTime(&t)) return false;

    for (int d = 0; d < 7; d++)
    {
      struct tm day = t;
      time_t tt = mktime(&day) + d * 86400;
      localtime_r(&tt, &day);

      String e1, e2, e3;
      getEventsForDay(day, e1, e2, e3);

      if (e1.length() > 60) e1 = e1.substring(0, 60);
      if (e2.length() > 60) e2 = e2.substring(0, 60);
      if (e3.length() > 60) e3 = e3.substring(0, 60);

      if (!weekInitialized ||
          strcmp(e1.c_str(), lastWeek[d][0]) != 0 ||
          strcmp(e2.c_str(), lastWeek[d][1]) != 0 ||
          strcmp(e3.c_str(), lastWeek[d][2]) != 0)
      {
        return true;
      }
    }

    return false;
  }

  void storeWeek()
  {
    struct tm t;
    if (!getLocalTime(&t)) return;

    for (int d = 0; d < 7; d++)
    {
      struct tm day = t;
      time_t tt = mktime(&day) + d * 86400;
      localtime_r(&tt, &day);

      String e1, e2, e3;
      getEventsForDay(day, e1, e2, e3);

      // omez délku (KRITICKÉ)
      if (e1.length() > 60) e1 = e1.substring(0, 60);
      if (e2.length() > 60) e2 = e2.substring(0, 60);
      if (e3.length() > 60) e3 = e3.substring(0, 60);

      safeCopy(lastWeek[d][0], e1);
      safeCopy(lastWeek[d][1], e2);
      safeCopy(lastWeek[d][2], e3);
    }

    weekInitialized = true;
  }

  // =========================
  // 📅 STAŽENÍ
  // =========================
  bool fetchICS(const String& url)
  {
      icsData.reserve(20000);

      HTTPClient http;
      http.begin(url);
      http.setTimeout(15000);

      int httpCode = http.GET();

      if (httpCode != 200)
      {
          http.end();
          return false;
      }

      String newData = http.getString();
      http.end();

      // jen uložit
      icsData = newData;

      return true;
  }


  // =========================
  String extract(const String& block, const String& key) 
  {
    int idx = block.indexOf(key);
    if (idx == -1) return "";
    int start = block.indexOf(":", idx);
    int end = block.indexOf("\n", start);
    return block.substring(start + 1, end);
  }

  // =========================
  bool isSameDay(struct tm& t, int y, int m, int d) 
  {
    return (t.tm_year + 1900 == y &&
            t.tm_mon + 1 == m &&
            t.tm_mday == d);
  }

  // =========================
  bool parseDate(const String& s, int& y, int& m, int& d) 
  {
    if (s.length() < 8) return false;
    y = s.substring(0, 4).toInt();
    m = s.substring(4, 6).toInt();
    d = s.substring(6, 8).toInt();
    return true;
  }


  // =========================
  // 🆕 3 UDÁLOSTI
  // =========================
  void getEventsForDay(struct tm& t, String& out1, String& out2, String& out3) 
  {
    out1 = "";
    out2 = "";
    out3 = "";

    struct Event 
    {
      String uid;
      String summary;
    };

    Event found[6]; // 🆕 víc místa
    int count = 0;

    int pos = 0;

    while (true) 
    {
      int start = icsData.indexOf("BEGIN:VEVENT", pos);
      if (start == -1) break;

      int end = icsData.indexOf("END:VEVENT", start);
      if (end == -1) break;

      String block = icsData.substring(start, end);

      //int len = end - start;
      //if (len > 256) len = 256;

      //String block = icsData.substring(start, start + len);      

      String uid = extract(block, "UID");
      String summary = extract(block, "SUMMARY");
      String dtstart = extract(block, "DTSTART");
      String recId = extract(block, "RECURRENCE-ID");
      String rrule = extract(block, "RRULE");

      int y, m, d;
      bool match = false;

      if (recId.length() > 0) 
      {
        if (parseDate(recId, y, m, d) && isSameDay(t, y, m, d)) 
        {
          match = true;
        }
      }
      else if (dtstart.length() > 0) 
      {
        if (parseDate(dtstart, y, m, d)) 
        {
          if (isSameDay(t, y, m, d)) 
          {
            match = true;
          }
          else if (rrule.indexOf("FREQ=YEARLY") != -1) 
          {
            if ((t.tm_mon + 1 == m) && (t.tm_mday == d)) 
            {
              match = true;
            }
          }
        }
      }

      if (match && count < 6)
      {
        found[count++] = {uid, summary};
      }

      pos = end;
    }

    // odstranění duplicit
    for (int i = 0; i < count; i++) 
    {
      for (int j = i + 1; j < count; j++) 
      {
        if (found[i].uid == found[j].uid) 
        {
          found[j].summary = "";
        }
      }
    }

    // 🆕 kompaktace
    int newCount = 0;
    for (int i = 0; i < count; i++)
    {
      if (found[i].summary != "")
      {
        found[newCount++] = found[i];
      }
    }
    count = newCount;

    // 🆕 stabilní řazení
    for (int i = 0; i < count - 1; i++)
    {
      for (int j = i + 1; j < count; j++)
      {
        if (found[i].summary > found[j].summary ||
          (found[i].summary == found[j].summary && found[i].uid > found[j].uid))
        {
          Event tmp = found[i];
          found[i] = found[j];
          found[j] = tmp;
        }
      }
    }

    // výstup 3 položky
    int outCount = 0;

    for (int i = 0; i < count; i++) 
    {
      if (found[i].summary == "") continue;

      if (outCount == 0) out1 = found[i].summary;
      else if (outCount == 1) out2 = found[i].summary;
      else if (outCount == 2) out3 = found[i].summary;

      outCount++;
      if (outCount >= 3) break;
    }

  }
};

/*
#include <WiFi.h>
#include <HTTPClient.h>

class ICSCalendar 
{
  String icsData;

public:


  bool fetchICS(const String& url) 
  {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != 200) 
    {
      http.end();
      return false;
    }
    icsData = http.getString();
    http.end();
    return true;
  }


  String extract(const String& block, const String& key) 
  {
    int idx = block.indexOf(key);
    if (idx == -1) return "";
    int start = block.indexOf(":", idx);
    int end = block.indexOf("\n", start);
    return block.substring(start + 1, end);
  }


  bool isSameDay(struct tm& t, int y, int m, int d) 
  {
    return (t.tm_year + 1900 == y &&
            t.tm_mon + 1 == m &&
            t.tm_mday == d);
  }

  bool parseDate(const String& s, int& y, int& m, int& d) 
  {
    if (s.length() < 8) return false;
    y = s.substring(0, 4).toInt();
    m = s.substring(4, 6).toInt();
    d = s.substring(6, 8).toInt();
    return true;
  }


  void getEventsForDay(struct tm& t, String& out1, String& out2) 
  {
    out1 = "";
    out2 = "";
    struct Event 
    {
      String uid;
      String summary;
    };
    Event found[4];
    int count = 0;
    int pos = 0;
    while (true) 
    {
      int start = icsData.indexOf("BEGIN:VEVENT", pos);
      if (start == -1) break;
      int end = icsData.indexOf("END:VEVENT", start);
      if (end == -1) break;
      String block = icsData.substring(start, end);
      String uid = extract(block, "UID");
      String summary = extract(block, "SUMMARY");
      String dtstart = extract(block, "DTSTART");
      String recId = extract(block, "RECURRENCE-ID");
      String rrule = extract(block, "RRULE");
      int y, m, d;
      bool match = false;
      bool isOverride = false;
      if (recId.length() > 0) 
      {
        if (parseDate(recId, y, m, d) && isSameDay(t, y, m, d)) 
        {
          match = true;
          isOverride = true;
        }
      }
      else if (dtstart.length() > 0) 
      {
        if (parseDate(dtstart, y, m, d)) 
        {
          if (isSameDay(t, y, m, d)) 
          {
            match = true;
          }
          else if (rrule.indexOf("FREQ=YEARLY") != -1) {
            if ((t.tm_mon + 1 == m) && (t.tm_mday == d)) {
              match = true;
            }
          }
        }
      }
      if (match && count < 4) 
      {
        found[count++] = {uid, summary};
      }
      pos = end;
    }
    for (int i = 0; i < count; i++) 
    {
      for (int j = i + 1; j < count; j++) 
      {
        if (found[i].uid == found[j].uid) 
        {
          found[j].summary = "";
        }
      }
    }
    int outCount = 0;
    for (int i = 0; i < count; i++) 
    {
      if (found[i].summary == "") continue;

      if (outCount == 0) out1 = found[i].summary;
      else if (outCount == 1) out2 = found[i].summary;

      outCount++;
      if (outCount >= 2) break;
    }
  }
};
*/