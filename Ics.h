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