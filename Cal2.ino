//
// Pred kompilaci nainstalovat ESP32 v manazeru desek od Espressif Systems

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <SPI.h>
#include <GxEPD2_3C.h> // nutno doinstalovat
#include <ArduinoJson.h> // nutno doinstalovat
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h> // ESP Async WebServer by ESP32Async
#include <AsyncTCP.h> // Async TCP by ESP32Async
#include <Preferences.h>
#include "Ics.h"
#include "Icons.h"
#include "Fonts/fontinc.h"

// ===== ICS =====
float lat = 0;
float lon = 0;
String ics = "";
String nameDay = "";
String sunRise = "";
String sunSet = "";


#define CLEAR_PIN 4 // smaze komplet nastaveni
#define RESET_PIN 2 // reset desky ESP32

#define CS_PIN   5
#define DC_PIN   12
#define RST_PIN  16
#define BUSY_PIN 21

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(
  GxEPD2_750c_Z08(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN)
);


struct tm timeinfo; // globalni casovy udaj


struct Rect 
{
    int x;
    int y;
    int w;
    int h;
    int right() const { return x + w; }
    int bottom() const { return y + h; }
};


struct DayResult 
{
  float tMin;
  float tMax;
  uint8_t icon;
  uint8_t wind;
  uint8_t rainIntensity;
  float wind_ms;
  float wind_max;
  float rain_mm;  
  bool valid;
};

DayResult weatherResult[7] = {0};


struct Area 
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
};


struct LayoutResult 
{
  Area top;
  Area bottom;
};


struct DayInfo 
{
  String dayName;
  String dayFull;
  String dayNumber;
  String monthName;
};



#define ICON_SUNNY          0
#define ICON_PARTLY         1
#define ICON_CLOUDY         2
#define ICON_RAIN           3
#define ICON_SHOWERS        4
#define ICON_SNOW           5
#define ICON_STORM          6
#define ICON_FOG            7
#define ICON_UNKNOWN        255

/*
#define WIND_NONE    0
#define WIND_LOW     1
#define WIND_MEDIUM  2
#define WIND_HIGH    3

#define RAIN_NONE    0
#define RAIN_LOW     1
#define RAIN_MEDIUM  2
#define RAIN_HIGH    3


#define ICONWIDTH 24
*/

ICSCalendar calendar;

AsyncWebServer server(80);
Preferences prefs;


// ===== CONFIG =====
String ssidList[5];
String passList[5];

// ===== HTML =====
String htmlPage() {

    String wifiList = "";

    for (int i = 0; i < 3; i++) {
        if (ssidList[i] != "") {
            wifiList += "<div class='wifi'>";
            wifiList += "<span>" + ssidList[i] + "</span>";
            wifiList += "<a href='/del?id=" + String(i) + "'>✕</a>";
            wifiList += "</div>";
        }
    }

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>

* {
  box-sizing: border-box;
}

body {
  font-family: Arial, sans-serif;
  background: #f2f2f2;
  margin: 0;
}

.container {
  max-width: 420px;
  margin: auto;
  padding: 15px;
}

.card {
  background: white;
  padding: 15px;
  margin-bottom: 15px;
  border-radius: 12px;
  box-shadow: 0 3px 8px rgba(0,0,0,0.15);
}

h2 {
  text-align: center;
  margin-bottom: 20px;
}

h3 {
  margin-top: 0;
}

input {
  width: 100%;
  padding: 10px;
  margin-top: 6px;
  margin-bottom: 10px;
  border-radius: 6px;
  border: 1px solid #ccc;
  font-size: 14px;
}

button {
  width: 100%;
  padding: 12px;
  background: #007BFF;
  color: white;
  border: none;
  border-radius: 6px;
  font-size: 16px;
}

button:hover {
  background: #0056b3;
}

.wifi {
  background: #eee;
  padding: 8px 10px;
  border-radius: 6px;
  margin-bottom: 6px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.wifi a {
  color: red;
  text-decoration: none;
  font-weight: bold;
}

.small {
  font-size: 12px;
  color: #666;
}

</style>
</head>

<body>

<div class="container">

<h2>📅 E-ink Kalendář</h2>

<!-- WIFI -->
<div class="card">
<h3>📶 WiFi sítě</h3>
)rawliteral";

    html += wifiList;

    html += R"rawliteral(

<form action="/add">
<input name="ssid" placeholder="SSID">
<input name="pass" placeholder="Heslo">
<button>Přidat WiFi</button>
</form>

</div>

<!-- LOKALITA + ICS -->
<div class="card">
<h3>📍 Nastavení</h3>

<form action="/save">

<input name="lat" placeholder="Latitude">
<div class="small">např. 49.1951</div>

<input name="lon" placeholder="Longitude">
<div class="small">např. 16.6068</div>

<input name="ics" placeholder="ICS URL">
<div class="small">Google kalendář – veřejný odkaz</div>

<button>Uložit nastavení</button>

</form>

</div>

<!-- RESTART -->
<div class="card">

<form action="/restart">
<button>Restart zařízení</button>
</form>

</div>

</div>

</body>
</html>
)rawliteral";

    return html;
}


void loadConfig() 
{
    prefs.begin("cfg", true);
    char key[8];
    for (int i = 0; i < 5; i++) 
    {
        sprintf(key, "s%d", i);
        ssidList[i] = prefs.getString(key, "");
        sprintf(key, "p%d", i);
        passList[i] = prefs.getString(key, "");
        Serial.println("WiFi[" + String(i) + "] = " + ssidList[i]);
    }
    lat = prefs.getFloat("lat", 0);
    lon = prefs.getFloat("lon", 0);
    ics = prefs.getString("ics", "");
    Serial.println("LAT: " + String(lat));
    Serial.println("LON: " + String(lon));
    Serial.println("ICS: " + ics);
    prefs.end();
}

void saveWiFi(int id, String s, String p) 
{
    prefs.begin("cfg", false);
    char key[8];
    sprintf(key, "s%d", id);
    prefs.putString(key, s);
    sprintf(key, "p%d", id);
    prefs.putString(key, p);
    prefs.end();
    Serial.println("SAVING:");
    Serial.println("SSID: " + s);
    Serial.println("PASS: " + p);    
}


void deleteWiFi(int id) 
{
    prefs.begin("cfg", false);
    char key[8];
    sprintf(key, "s%d", id);
    prefs.remove(key);
    sprintf(key, "p%d", id);
    prefs.remove(key);
    prefs.end();
}

bool connectBestWiFi() 
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(200);
    int n = WiFi.scanNetworks();
    Serial.println("Networks found: " + String(n));
    String bestSSID = "";
    String bestPASS = "";
    int bestRSSI = -1000;
    for (int i = 0; i < n; i++) 
    {
        String found = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        Serial.println("SSID: " + found + " RSSI: " + String(rssi));
        for (int j = 0; j < 5; j++) 
        {

            if (found == ssidList[j] && rssi > bestRSSI) 
            {
                bestSSID = ssidList[j];
                bestPASS = passList[j];
                bestRSSI = rssi;
            }
        }
    }
    if (bestSSID == "") 
    {
        Serial.println("No network found in saved!");
        return false;
    }
    Serial.println("Connecting to WiFi: " + bestSSID);
    WiFi.begin(bestSSID.c_str(), bestPASS.c_str());
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) 
    {
        delay(500);
        Serial.print(".");
        tries++;
    }
    Serial.println();
    Serial.println("Status: " + String(WiFi.status()));
    return WiFi.status() == WL_CONNECTED;
}


bool hasConfig() 
{
    prefs.begin("cfg", true);
    String test = prefs.getString("s0", "");
    prefs.end();
    return test != "";
}


void startConfig() 
{
    WiFi.softAP("Kalendář-Setup");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", htmlPage());
    });
    server.on("/add", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        if (request->hasParam("ssid") && request->hasParam("pass")) 
        {
            String s = request->getParam("ssid")->value();
            String p = request->getParam("pass")->value();
            for (int i = 0; i < 5; i++) 
            {
                if (ssidList[i].length() == 0) 
                {
                    saveWiFi(i, s, p);
                    loadConfig();
                    break;
                }
            }
        }
        request->send(200, "text/html; charset=UTF-8",
        "<html><meta charset='UTF-8'><body>Přidáno ✔<br><a href='/'>Zpět</a></body></html>");
    });
    server.on("/del", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        int id = request->getParam("id")->value().toInt();
        deleteWiFi(id);
        loadConfig();
        request->redirect("/");
    });
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        float la = request->getParam("lat")->value().toFloat();
        float lo = request->getParam("lon")->value().toFloat();
        String i = request->getParam("ics")->value();
        saveConfig(la, lo, i);
        request->send(200, "text/html; charset=UTF-8", 
        "<html><meta charset='UTF-8'><body>Uloženo ✔, restart...</body></html>");
        delay(200);
        ESP.restart();   
    });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        request->send(200, "text/html; charset=UTF-8", "Restartuji...");
        delay(200);
        ESP.restart();
    });    
    server.begin();
    while (true) delay(10);
}

void saveConfig(float la, float lo, String i) 
{
    prefs.begin("cfg", false);
    prefs.putFloat("lat", la);
    prefs.putFloat("lon", lo);
    prefs.putString("ics", i);
    prefs.end();
}

void clearConfig()
{
  prefs.begin("cfg", false);
  prefs.clear();
  prefs.end();    
  delay(500);
  ESP.restart();  
}


void syncTime()
{
  Serial.println("Sync time...");
  configTime(3600, 3600, "cz.pool.ntp.org");
  while (!getLocalTime(&timeinfo)) delay(200);
  //timeinfo.tm_year += 1900;
  //timeinfo.tm_mon += 1;
  Serial.print("Date: ");
  Serial.print(timeinfo.tm_mday); Serial.print(".");
  Serial.print(timeinfo.tm_mon+1); Serial.print(".");
  Serial.println(timeinfo.tm_year+1900);
  Serial.print("Time: ");
  Serial.print(timeinfo.tm_hour); Serial.print(":");
  Serial.println(timeinfo.tm_min);
}


void downloadICS()
{
  Serial.println("Downloading ICS...");
  if (calendar.fetchICS(ics)) 
  {
    Serial.println("ICS donwloaded");
  } 
  else
  {
    Serial.println("ICS downloading error!");
  }
}


bool connectWiFi() 
{
  Serial.println("Connecting WiFi...");
  loadConfig();
  if (!connectBestWiFi()) 
  {
      Serial.println("WiFi fail!");
      return false;
  }
  else
  {
    Serial.println("WiFi OK!");
    return true;
  }
  return false;
}


bool hasWiFi() 
{
    for (int i = 0; i < 5; i++) 
    {
        if (ssidList[i] != "") return true;
    }
    return false;
}

static String toHHMM(double t)
{
  int h = (int)t;
  int m = (int)((t - h) * 60 + 0.5);
  if (m >= 60) { m -= 60; h++; }
  if (h >= 24) h -= 24;
  char buf[6];
  sprintf(buf, "%02d:%02d", h, m);
  return String(buf);
}


String twoDigitsFromText(String value)
{
  return (value.length() < 2) ? "0" + value : value;
}


String twoDigits(int value)
{
  return (value < 10) ? "0" + String(value) : String(value);
}


static bool CalcSun(double lat, double lon, String &sunriseStr, String &sunsetStr)
{
  struct tm t;
  if (!getLocalTime(&t)) return false;
  int y = t.tm_year + 1900;
  int m = t.tm_mon + 1;
  int d = t.tm_mday;
  int tz = t.tm_isdst ? 2 : 1;
  int N1 = floor(275 * m / 9);
  int N2 = floor((m + 9) / 12);
  int N3 = (1 + floor((y - 4 * floor(y / 4) + 2) / 3));
  int N = N1 - (N2 * N3) + d - 30;
  double lngHour = lon / 15.0;
  auto calc = [&](bool sunrise)
  {
    double t0 = N + ((sunrise ? 6 : 18) - lngHour) / 24.0;
    double M = (0.9856 * t0) - 3.289;
    double L = M + 1.916*sin(M*DEG_TO_RAD)
                + 0.020*sin(2*M*DEG_TO_RAD)
                + 282.634;
    L = fmod(L, 360);
    double RA = atan(0.91764 * tan(L*DEG_TO_RAD)) * RAD_TO_DEG;
    RA = fmod(RA, 360);
    double sinDec = 0.39782 * sin(L*DEG_TO_RAD);
    double cosDec = cos(asin(sinDec));
    double cosH = (cos(90.833*DEG_TO_RAD) -
                  (sinDec * sin(lat*DEG_TO_RAD))) /
                  (cosDec * cos(lat*DEG_TO_RAD));
    if (cosH > 1 || cosH < -1) return -1.0;
    double H = sunrise ? (360 - acos(cosH)*RAD_TO_DEG)
                      : (acos(cosH)*RAD_TO_DEG);
    H /= 15.0;
    double T = H + RA/15 - (0.06571 * t0) - 6.622;
    double UT = T - lngHour;
    return UT;
  };
  double riseUTC = calc(true);
  double setUTC  = calc(false);
  double rise = riseUTC + tz;
  double set  = setUTC  + tz;
  if (rise < 0) rise += 24;
  if (set  < 0) set  += 24;
  if (rise >= 24) rise -= 24;
  if (set  >= 24) set  -= 24;
  sunriseStr = toHHMM(rise);
  sunsetStr  = toHHMM(set);
  return true;
}  


bool CalcSunFromAPI(double lat, double lon, String &sunriseStr, String &sunsetStr)
{
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(lat,6);
  url += "&longitude=" + String(lon,6);
  url += "&daily=sunrise,sunset";
  url += "&timezone=Europe%2FPrague";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return false;
  String sunrise = doc["daily"]["sunrise"][0];
  String sunset  = doc["daily"]["sunset"][0];
  sunriseStr = sunrise.substring(11,16);
  sunsetStr  = sunset.substring(11,16);
  return true;
}  


String getNameDayCZ(int day, int month, int year) 
{
  HTTPClient http;
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", year, month, day);
  String url = "https://svatkyapi.cz/api/day/" + String(dateStr);  
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != 200) 
  {
    http.end();
    Serial.println("GetNameDay > Chyba API");
    return " ";
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) 
  {
    Serial.println("GetNameDay > Chyba JSON");
    return " ";
  }
  if (doc.containsKey("name")) 
  {
    return String((const char*)doc["name"]);
  }
  if (doc.containsKey("svatek")) 
  {
    return String((const char*)doc["svatek"]);
  }
  Serial.println("GetNameDay > Neznama chyba");
  return " ";
}


void printCZ(const char* text) 
{
  while (*text) 
  {
    uint8_t c = (uint8_t)*text;
    if (c >= 32 && c <= 126) 
    {
      display.write(c - 32);
    }
    else if (c == 0xC3) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0xA1: display.write(95); break;  // á
        case 0xA9: display.write(98); break;  // é
        case 0xAD: display.write(100); break; // í
        case 0xB3: display.write(102); break; // ó
        case 0xBA: display.write(106); break; // ú
        case 0xBD: display.write(108); break; // ý
        case 0x81: display.write(110); break; // Á
        case 0x89: display.write(113); break; // É
        case 0x8D: display.write(115); break; // Í
        case 0x93: display.write(117); break; // Ó
        case 0x9A: display.write(121); break; // Ú
        case 0x9D: display.write(123); break; // Ý
      }
    }
    else if (c == 0xC4) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0x8D: display.write(96); break;  // č
        case 0x8F: display.write(97); break;  // ď
        case 0x9B: display.write(99); break;  // ě
        case 0x88: display.write(101); break; // ň
        case 0x8C: display.write(111); break; // Č
        case 0x8E: display.write(112); break; // Ď
        case 0x9A: display.write(114); break; // Ě
        case 0x87: display.write(116); break; // Ň
      }
    }
    else if (c == 0xC5) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0x99: display.write(103); break; // ř
        case 0xA1: display.write(104); break; // š
        case 0xA5: display.write(105); break; // ť
        case 0xBE: display.write(109); break; // ž
        case 0xAF: display.write(107); break; // ů
        case 0x98: display.write(118); break; // Ř
        case 0xA0: display.write(119); break; // Š
        case 0xA4: display.write(120); break; // Ť
        case 0xBD: display.write(124); break; // Ž
        case 0xAE: display.write(122); break; // Ů
      }
    }
    text++;
  }
}


int getTextHeightCZ(const char* text, const GFXfont* font)
{
  int maxAbove = 0;
  int maxBelow = 0;
  while (*text)
  {
    uint8_t c = (uint8_t)*text;
    uint8_t index = 0;
    if (c >= 32 && c <= 126) 
    {
      index = c - 32;
    }
    else if (c == 0xC3) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0xA1: index = 95; break;
        case 0xA9: index = 98; break;
        case 0xAD: index = 100; break;
        case 0xB3: index = 102; break;
        case 0xBA: index = 106; break;
        case 0xBD: index = 108; break;

        case 0x81: index = 110; break;
        case 0x89: index = 113; break;
        case 0x8D: index = 115; break;
        case 0x93: index = 117; break;
        case 0x9A: index = 121; break;
        case 0x9D: index = 123; break;
      }
    }
    else if (c == 0xC4) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0x8D: index = 96; break;
        case 0x8F: index = 97; break;
        case 0x9B: index = 99; break;
        case 0x88: index = 101; break;

        case 0x8C: index = 111; break;
        case 0x8E: index = 112; break;
        case 0x9A: index = 114; break;
        case 0x87: index = 116; break;
      }
    }
    else if (c == 0xC5) 
    {
      text++;
      switch ((uint8_t)*text) {
        case 0x99: index = 103; break;
        case 0xA1: index = 104; break;
        case 0xA5: index = 105; break;
        case 0xBE: index = 109; break;
        case 0xAF: index = 107; break;
        case 0x98: index = 118; break;
        case 0xA0: index = 119; break;
        case 0xA4: index = 120; break;
        case 0xBD: index = 124; break;
        case 0xAE: index = 122; break;
      }
    }
    const GFXglyph *glyph = &font->glyph[index];
    int yOffset = glyph->yOffset;
    int h = glyph->height;
    int above = -yOffset;
    int below = h + yOffset;
    if (above > maxAbove) maxAbove = above;
    if (below > maxBelow) maxBelow = below;
    text++;
  }
  return maxAbove + maxBelow;
}


int getTextWidthCZ(const char* text, const GFXfont* font) 
{
  int width = 0;
  while (*text) 
  {
    uint8_t c = (uint8_t)*text;
    uint8_t index = 0;
    if (c >= 32 && c <= 126) 
    {
      index = c - 32;
    }
    else if (c == 0xC3) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0xA1: index = 95; break;
        case 0xA9: index = 98; break;
        case 0xAD: index = 100; break;
        case 0xB3: index = 102; break;
        case 0xBA: index = 106; break;
        case 0xBD: index = 108; break;
        case 0x81: index = 110; break;
        case 0x89: index = 113; break;
        case 0x8D: index = 115; break;
        case 0x93: index = 117; break;
        case 0x9A: index = 121; break;
        case 0x9D: index = 123; break;
      }
    }
    else if (c == 0xC4) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0x8D: index = 96; break;
        case 0x8F: index = 97; break;
        case 0x9B: index = 99; break;
        case 0x88: index = 101; break;
        case 0x8C: index = 111; break;
        case 0x8E: index = 112; break;
        case 0x9A: index = 114; break;
        case 0x87: index = 116; break;
      }
    }
    else if (c == 0xC5) 
    {
      text++;
      switch ((uint8_t)*text) 
      {
        case 0x99: index = 103; break;
        case 0xA1: index = 104; break;
        case 0xA5: index = 105; break;
        case 0xBE: index = 109; break;
        case 0xAF: index = 107; break;
        case 0x98: index = 118; break;
        case 0xA0: index = 119; break;
        case 0xA4: index = 120; break;
        case 0xBD: index = 124; break;
        case 0xAE: index = 122; break;
      }
    }
    const GFXglyph *glyph = &font->glyph[index];
    width += glyph->xAdvance;
    text++;
  }
  return width;
}


DayInfo getDayInfo(struct tm timeinfo)
{
    DayInfo info;
    const char* days[] = {"Ne", "Po", "Út", "St", "Čt", "Pá", "So"};
    const char* daysFull[] = {"Neděle", "Pondělí", "Úterý", "Středa", "Čtvrtek", "Pátek", "Sobota"};
    const char* months[] = {
        "LEDEN", "ÚNOR", "BŘEZEN", "DUBEN", "KVĚTEN", "ČERVEN",
        "ČERVENEC", "SRPEN", "ZÁŘÍ", "ŘÍJEN", "LISTOPAD", "PROSINEC"
    };
    info.dayName = days[timeinfo.tm_wday];
    info.dayFull = daysFull[timeinfo.tm_wday];
    info.dayNumber = String(timeinfo.tm_mday);
    info.monthName = months[timeinfo.tm_mon];
    return info;
}


struct tm addDays(int daysToAdd)
{
    struct tm timeinfo;
    // vezmi aktuální čas (musíš mít nastavené NTP)
    if (!getLocalTime(&timeinfo))
    {
        // fallback - vrátí prázdný čas
        struct tm empty = {};
        return empty;
    }
    // převedení na timestamp (sekundy od 1970)
    time_t t = mktime(&timeinfo);
    // přičti dny (1 den = 86400 sekund)
    t += (daysToAdd * 86400);
    // zpět na struct tm
    struct tm result;
    localtime_r(&t, &result);
    return result;
}

/*
bool getWeatherForDay(float lat, float lon, tm timeinfo, DayResult &out) 
{
  out.valid = false;
  char targetDate[11];
  sprintf(targetDate, "%04d-%02d-%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday);
  Serial.print("GET WEATHER FOR: ");
  Serial.println(targetDate);
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(lat, 6);
  url += "&longitude=" + String(lon, 6);
  url += "&daily=temperature_2m_max,temperature_2m_min,weathercode,precipitation_sum,precipitation_probability_max,precipitation_hours,windspeed_10m_max";
  url += "&forecast_days=7&timezone=auto";
  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);
  int httpCode = http.GET();
  if (httpCode != 200)
  {
    Serial.print("GetWeatherForDay > HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  StaticJsonDocument<768> filter;
  filter["daily"]["time"] = true;
  filter["daily"]["temperature_2m_max"] = true;
  filter["daily"]["temperature_2m_min"] = true;
  filter["daily"]["weathercode"] = true;
  filter["daily"]["precipitation_sum"] = true;
  filter["daily"]["precipitation_probability_max"] = true;
  filter["daily"]["precipitation_hours"] = true;
  filter["daily"]["windspeed_10m_max"] = true;
  StaticJsonDocument<3072> doc;
  if (deserializeJson(doc, payload, DeserializationOption::Filter(filter))) 
  {
    Serial.println("GetWeatherForDay > deserializeJson chyba!");
    return false;
  }
  JsonArray time = doc["daily"]["time"];
  JsonArray tMax = doc["daily"]["temperature_2m_max"];
  JsonArray tMin = doc["daily"]["temperature_2m_min"];
  JsonArray wCode = doc["daily"]["weathercode"];
  JsonArray rain = doc["daily"]["precipitation_sum"];
  JsonArray rainProb = doc["daily"]["precipitation_probability_max"];
  JsonArray rainHours = doc["daily"]["precipitation_hours"];
  JsonArray wind = doc["daily"]["windspeed_10m_max"];
  for (int i = 0; i < time.size(); i++) 
  {
    if (strcmp(time[i], targetDate) == 0) 
    {
      out.tMin = tMin[i];
      out.tMax = tMax[i];
      int code = wCode[i];
      float r = rain[i];              // mm za den
      float prob = rainProb[i];
      float hours = rainHours[i];
      float w_kmh = wind[i];
      out.rain_mm = r;                // mm/den
      out.wind_ms = w_kmh / 3.6;      // m/s
      bool willRain = (prob >= 30.0 && hours > 0 && r > 0.2);
      if (code == 0)
      {
        out.icon = ICON_SUNNY;
      }
      else if (code <= 2)
      {
        out.icon = willRain ? ICON_SHOWERS : ICON_PARTLY;
      }
      else if (code == 3)
      {
        out.icon = willRain ? ICON_SHOWERS : ICON_CLOUDY;
      }
      else if (code == 45 || code == 48)
      {
        out.icon = ICON_FOG;
      }
      else if (code >= 51 && code <= 67)
      {
        out.icon = willRain ? ICON_RAIN : ICON_CLOUDY;
      }
      else if (code >= 71 && code <= 77)
      {
        out.icon = ICON_SNOW;
      }
      else if (code >= 80 && code <= 82)
      {
        out.icon = willRain ? ICON_SHOWERS : ICON_CLOUDY;
      }
      else if (code == 85 || code == 86)
      {
        out.icon = ICON_SNOW;
      }
      else if (code >= 95)
      {
        out.icon = ICON_STORM;
      }
      else
      {
        out.icon = ICON_UNKNOWN;
      }
      Serial.print("Rain (mm/day): "); Serial.println(out.rain_mm);
      Serial.print("Wind (m/s): "); Serial.println(out.wind_ms);
      out.valid = true;
      return true;
    }
  }
  return false;
}
*/

/* tato
bool getWeatherWeekAvgWind(float lat, float lon, DayResult out[7])
{
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(lat, 6);
  url += "&longitude=" + String(lon, 6);
  url += "&hourly=windspeed_10m";
  url += "&daily=temperature_2m_max,temperature_2m_min,weathercode,precipitation_sum,precipitation_probability_max,precipitation_hours";
  url += "&forecast_days=7&timezone=auto";

  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);

  int httpCode = http.GET();
  if (httpCode != 200)
  {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<8192> doc;
  if (deserializeJson(doc, payload))
  {
    return false;
  }

  // DAILY
  JsonArray tMax = doc["daily"]["temperature_2m_max"];
  JsonArray tMin = doc["daily"]["temperature_2m_min"];
  JsonArray wCode = doc["daily"]["weathercode"];
  JsonArray rain = doc["daily"]["precipitation_sum"];
  JsonArray rainProb = doc["daily"]["precipitation_probability_max"];
  JsonArray rainHours = doc["daily"]["precipitation_hours"];

  // HOURLY
  JsonArray hourlyWind = doc["hourly"]["windspeed_10m"];

  // --- průměry větru ---
  for (int d = 0; d < 7; d++)
  {
    float sum = 0;
    int count = 0;

    // každých 24 hodin = 1 den
    for (int h = d * 24; h < (d + 1) * 24; h++)
    {
      sum += hourlyWind[h].as<float>();
      count++;
    }

    float avg_kmh = sum / count;

    out[d].wind_ms = avg_kmh / 3.6; // převod na m/s
  }

  // --- ostatní data ---
  for (int i = 0; i < 7; i++)
  {
    out[i].valid = true;

    out[i].tMin = tMin[i];
    out[i].tMax = tMax[i];
    out[i].rain_mm = rain[i];

    int code = wCode[i];
    float prob = rainProb[i];
    float hours = rainHours[i];

    bool willRain = (prob >= 30.0 && hours > 0 && rain[i] > 0.2);

    if (code == 0)
      out[i].icon = ICON_SUNNY;
    else if (code <= 2)
      out[i].icon = willRain ? ICON_SHOWERS : ICON_PARTLY;
    else if (code == 3)
      out[i].icon = willRain ? ICON_SHOWERS : ICON_CLOUDY;
    else if (code == 45 || code == 48)
      out[i].icon = ICON_FOG;
    else if (code >= 51 && code <= 67)
      out[i].icon = willRain ? ICON_RAIN : ICON_CLOUDY;
    else if (code >= 71 && code <= 77)
      out[i].icon = ICON_SNOW;
    else if (code >= 80 && code <= 82)
      out[i].icon = willRain ? ICON_SHOWERS : ICON_CLOUDY;
    else if (code == 85 || code == 86)
      out[i].icon = ICON_SNOW;
    else if (code >= 95)
      out[i].icon = ICON_STORM;
    else
      out[i].icon = ICON_UNKNOWN;
  }

  return true;
}
*/


bool getWeatherWeekAfternoon(float lat, float lon, DayResult out[7])
{
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(lat, 6);
  url += "&longitude=" + String(lon, 6);
  url += "&hourly=weathercode,precipitation,precipitation_probability,windspeed_10m";
  url += "&daily=temperature_2m_max,temperature_2m_min";
  url += "&forecast_days=7&timezone=auto";

  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);

  int httpCode = http.GET();
  if (httpCode != 200)
  {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<10000> doc;
  if (deserializeJson(doc, payload))
  {
    return false;
  }

  // DAILY (teploty)
  JsonArray tMax = doc["daily"]["temperature_2m_max"];
  JsonArray tMin = doc["daily"]["temperature_2m_min"];

  // HOURLY
  JsonArray hCode = doc["hourly"]["weathercode"];
  JsonArray hRain = doc["hourly"]["precipitation"];
  JsonArray hProb = doc["hourly"]["precipitation_probability"];
  JsonArray hWind = doc["hourly"]["windspeed_10m"];

  // definice odpoledne
  int startHour = 12;
  int endHour = 18;

  for (int d = 0; d < 7; d++)
  {
    int start = d * 24 + startHour;
    int end = d * 24 + endHour;

    float maxWind = 0;
    float rainSum = 0;
    int rainHits = 0;

    int bestCode = 0;
    int bestScore = -1;

    for (int h = start; h < end; h++)
    {
      float w = hWind[h].as<float>();
      if (w > maxWind) maxWind = w;

      float r = hRain[h].as<float>();
      float p = hProb[h].as<float>();

      if (p > 40 && r > 0.1)
      {
        rainHits++;
        rainSum += r;
      }

      int code = hCode[h].as<int>();

      // váhování důležitosti počasí
      int score = 1;

      if (code >= 95) score = 6;       // bouřka
      else if (code >= 80) score = 5;  // přeháňky
      else if (code >= 60) score = 4;  // déšť
      else if (code == 3) score = 3;   // zataženo
      else if (code <= 2) score = 2;   // polojasno
      else if (code == 0) score = 1;   // jasno

      if (score > bestScore)
      {
        bestScore = score;
        bestCode = code;
      }
    }

    // naplnění výstupu
    out[d].valid = true;

    out[d].tMin = tMin[d].as<float>();
    out[d].tMax = tMax[d].as<float>();

    out[d].wind_ms = maxWind / 3.6; // km/h → m/s
    out[d].rain_mm = rainSum;

    bool willRain = (rainHits >= 2);

    // mapování ikon (můžeš doladit)
    if (bestCode == 0)
      out[d].icon = ICON_SUNNY;
    else if (bestCode <= 2)
      out[d].icon = willRain ? ICON_SHOWERS : ICON_PARTLY;
    else if (bestCode == 3)
      out[d].icon = willRain ? ICON_SHOWERS : ICON_CLOUDY;
    else if (bestCode >= 80)
      out[d].icon = ICON_SHOWERS;
    else if (bestCode >= 60)
      out[d].icon = ICON_RAIN;
    else if (bestCode >= 95)
      out[d].icon = ICON_STORM;
    else
      out[d].icon = ICON_UNKNOWN;
  }

  return true;
}



void drawDate()
{
  DayInfo d = getDayInfo(timeinfo);
  display.setFont(&LexendPeta_Medium);
  display.setTextColor(GxEPD_RED);
  display.setCursor(5, 60);
  printCZ(String(d.dayFull).c_str());
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(7, 63);
  printCZ(String(d.dayFull).c_str());
  //printCZ("Pondělí");
  display.setFont(&IBMPlexSerif_Medium);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(360, 90);
  display.print(String(d.dayNumber).c_str());
  display.setTextColor(GxEPD_RED);
  display.setCursor(357, 87);
  display.print(String(d.dayNumber).c_str());
  //display.print("31");
  display.setFont(&Inter_24pt_Bold);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10, 120);
  printCZ(String(d.monthName).c_str());
  //printCZ("LISTOPAD");
}

void drawNameDay()
{
  display.setFont(&OpenSans_Light18);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10, 160);
  String textNameDay = "";//"Svátek slaví ";
  textNameDay += nameDay;
  printCZ(textNameDay.c_str());
}

void drawRiseSun()
{
  display.drawBitmap(10, 175, sunrise, 32, 32, GxEPD_BLACK);
  display.setFont(&Lexend_Regular10);
  display.setTextColor(GxEPD_RED);
  display.setCursor(50, 200);
  printCZ(sunRise.c_str());
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(110, 200);
  printCZ(sunSet.c_str());
}

String floatToString(float value)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.1f", value);
    return String(buffer);
}

String getDayShortName(int offsetDays)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "?";
    }
    // přičtení dnů
    time_t t = mktime(&timeinfo);
    t += offsetDays * 86400;
    struct tm result;
    localtime_r(&t, &result);
    const char* days[] = {"Ne", "Po", "Út", "St", "Čt", "Pá", "So"};
    return String(days[result.tm_wday]);
}


void drawWeatherForDay()
{
  int yPos = 110;
  int xPos = display.width() - 150;
  int iconSize = 48;
  int centerX = xPos + (iconSize / 2);
  String minTemp = floatToString(weatherResult[0].tMin);
  String maxTemp = floatToString(weatherResult[0].tMax);
  String wind = floatToString(weatherResult[0].wind_ms);
  String rain = floatToString(weatherResult[0].rain_mm);

  String totalTemp = minTemp + " / " + maxTemp + " C";
  String totalOther = wind + " / " + rain;

  int totalTempWidth = getTextWidthCZ(totalTemp.c_str(), &LexendDeca_Regular12);
  int otherTempWidth = getTextWidthCZ(totalOther.c_str(), &LexendDeca_Regular12);

  display.setFont(&LexendDeca_Regular12);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(xPos, yPos + 65);
  printCZ(totalTemp.c_str());

  display.setCursor(xPos, yPos + 85);
  printCZ(totalOther.c_str());

  display.setTextColor(GxEPD_BLACK);
  if (weatherResult[0].valid)
  {
    switch (weatherResult[0].icon)
    {
      case ICON_SUNNY:
      {
        display.drawBitmap(xPos, yPos, sunny, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_PARTLY:
      {
        display.drawBitmap(xPos, yPos, partly, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_CLOUDY:
      {
        display.drawBitmap(xPos, yPos, cloudy, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_SHOWERS:
      {
        display.drawBitmap(xPos, yPos, showers, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_RAIN:
      {
        display.drawBitmap(xPos, yPos, rainy, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_FOG:
      {
        display.drawBitmap(xPos, yPos, fog, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_STORM:
      {
        display.drawBitmap(xPos, yPos, storm, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_SNOW:
      {
        display.drawBitmap(xPos, yPos, snowy, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
      case ICON_UNKNOWN:
      {
        display.drawBitmap(xPos, yPos, unknown, iconSize, iconSize, GxEPD_BLACK);
        break;
      }
    }
  }
  else
  {
    display.drawBitmap(xPos, yPos, unknown, iconSize, iconSize, GxEPD_BLACK);
  }

}


void drawWeather()
{
  int yPos = 675;
  int iconSize = 48;
  int totalIcons = 6 * iconSize;
  int spacing = (display.width() - totalIcons) / 7;
  String minTemp = "";
  String maxTemp = "";
  String wind = "";
  String rain = "";

  for (int i = 0; i < 6; i++)
  {
    int xPos = spacing + i * (iconSize + spacing);

    if (weatherResult[i+1].valid)
    {
      minTemp = floatToString(weatherResult[i+1].tMin);
      maxTemp = floatToString(weatherResult[i+1].tMax);
      wind = floatToString(weatherResult[i+1].wind_ms);
      rain = floatToString(weatherResult[i+1].rain_mm);

      minTemp += " C";
      maxTemp += " C";
      //wind += " ms";
      //rain += " mm";

      String totalOther = wind + " / " + rain;

      int centerX = xPos + (iconSize / 2);

      display.setTextColor(GxEPD_BLACK);
      display.setFont(&LexendDeca_Regular12);
      int shortDayWidth = getTextWidthCZ(getDayShortName(i+1).c_str(), &LexendDeca_Regular12);
      display.setCursor(centerX - (shortDayWidth / 2), yPos-5);
      printCZ(getDayShortName(i+1).c_str());



      display.setFont(&OpenSans_SemiCondensed_Bold10);

      //int minTempWidth = getTextWidthCZ("16.9/30.2", &OpenSans_SemiCondensed_Bold10);
      int minTempWidth = getTextWidthCZ(minTemp.c_str(), &OpenSans_SemiCondensed_Bold10);
      int maxTempWidth = getTextWidthCZ(maxTemp.c_str(), &OpenSans_SemiCondensed_Bold10);
      //int windWidth = getTextWidthCZ(wind.c_str(), &LexendDeca_Light5);
      //int rainWidth = getTextWidthCZ(rain.c_str(), &LexendDeca_Light5);
      int totalOtherWidth = getTextWidthCZ(totalOther.c_str(), &OpenSans_Medium8);

      display.setTextColor(GxEPD_BLACK);
      display.setCursor(centerX - (minTempWidth / 2), yPos + 65);
      printCZ(minTemp.c_str());
      //printCZ("16.9/30.2");

      display.setTextColor(GxEPD_RED);
      display.setCursor(centerX - (maxTempWidth / 2), yPos + 85);
      printCZ(maxTemp.c_str());

      display.setTextColor(GxEPD_BLACK);
      display.setFont(&OpenSans_Medium8);

      display.setTextColor(GxEPD_BLACK);
      display.setCursor(centerX - (totalOtherWidth / 2), yPos + 105);
      printCZ(totalOther.c_str());


      //display.setTextColor(GxEPD_BLACK);
      //display.setCursor(centerX - (windWidth / 2), yPos + 105);
      //printCZ(wind.c_str());

      //display.setTextColor(GxEPD_BLACK);
      //display.setCursor(centerX - (rainWidth / 2), yPos + 125);
      //printCZ(rain.c_str());

      display.setTextColor(GxEPD_BLACK);

      switch (weatherResult[i+1].icon)
      {
        case ICON_SUNNY:
        {
          display.drawBitmap(xPos, yPos, sunny, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_PARTLY:
        {
          display.drawBitmap(xPos, yPos, partly, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_CLOUDY:
        {
          display.drawBitmap(xPos, yPos, cloudy, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_SHOWERS:
        {
          display.drawBitmap(xPos, yPos, showers, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_RAIN:
        {
          display.drawBitmap(xPos, yPos, rainy, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_FOG:
        {
          display.drawBitmap(xPos, yPos, fog, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_STORM:
        {
          display.drawBitmap(xPos, yPos, storm, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_SNOW:
        {
          display.drawBitmap(xPos, yPos, snowy, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
        case ICON_UNKNOWN:
        {
          display.drawBitmap(xPos, yPos, unknown, iconSize, iconSize, GxEPD_BLACK);
          break;
        }
      }
    }
    else
    {
      display.drawBitmap(xPos, yPos, unknown, iconSize, iconSize, GxEPD_BLACK);
    }
  }

}

void drawEventsForDay()
{
  String s1, s2, s3;
  tm tstr = addDays(0);
  calendar.getEventsForDay(tstr, s1, s2, s3);
  int xPos = 0;
  int yPos = 235;
  if (s1.length() > 0 || s2.length() > 0 || s3.length() > 0)
  {
      display.setFont(&Inter_Medium10);
      display.setTextColor(GxEPD_BLACK);
      if (s1.length() > 0)
      {
        display.setCursor(xPos+28, yPos+12);
        printCZ(s1.c_str());
        display.drawBitmap(xPos, yPos-5, point, 24, 24, GxEPD_BLACK);
      }
      if (s2.length() > 0)
      {
        display.setCursor(xPos+28, yPos+43);
        printCZ(s2.c_str());
        display.drawBitmap(xPos, yPos+25, point, 24, 24, GxEPD_BLACK);
      }
      if (s3.length() > 0)
      {
        display.setCursor(xPos+28, yPos+72);
        printCZ(s3.c_str());
        display.drawBitmap(xPos, yPos+55, point, 24, 24, GxEPD_BLACK);
      }
  }
  else
  {
      display.setFont(&OpenSans_Light18);
      display.setTextColor(GxEPD_BLACK);
      display.drawBitmap(xPos, yPos+25, point, 24, 24, GxEPD_BLACK);
      display.setCursor(xPos+28, yPos+48);
      printCZ("DNES BEZ UDÁLOSTÍ");
  }  
}

void drawWeekLine()
{
  int top = 334;
  int bottom = 628;
  int rowHeight = (bottom - top) / 6;
  int textHeight = getTextHeightCZ(getDayShortName(0).c_str(), &LexendDeca_Regular12);
  display.setFont(&LexendDeca_Regular12);
  display.setTextColor(GxEPD_BLACK);
  for (int i = 1; i < 6; i++)
  {
    int y = top + i * rowHeight;
    display.drawFastHLine(10, y, 460, GxEPD_BLACK);
  }
  for ( int i = 1; i < 7; i++)
  {
    int y = top + i * rowHeight;
    display.setCursor(18, y-20);
    printCZ(getDayShortName(i).c_str());
  }
}

void drawWeekEvents()
{
  int top = 334;
  int bottom = 628;
  int rowHeight = (bottom - top) / 6;
  String s1, s2, s3;
  display.setTextColor(GxEPD_BLACK);
  for (int i = 1; i < 7; i++)
  {
    tm tstr = addDays(i);
    calendar.getEventsForDay(tstr, s1, s2, s3);
    int y = top + i * rowHeight;
    if (s1.length() > 0 || s2.length() > 0)
    {
      display.setFont(&OpenSans_Medium8);
      display.setCursor(65, y-30);
      printCZ(s1.c_str());
      display.setCursor(65, y-10);
      printCZ(s2.c_str());
    }
    else
    {
      display.setFont(&Roboto_Condensed_Regular12);
      display.setCursor(65, y-20);
      printCZ("Žádné události");
    }

  }
}

void drawLastRefresh()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return; // když není čas, nic nevykresluj
  }
  char timeStr[6]; // HH:MM + \0
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  String text = "Refresh: ";
  text += timeStr;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&Inter_Medium5);
  display.setCursor(10, display.height()-1);
  printCZ(text.c_str());
}


void drawAll()
{
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        /// BEGIN DRAW
        drawDate();
        drawNameDay();
        //display.drawFastHLine(10, 220, 460, GxEPD_BLACK);
        //display.drawFastHLine(10, 222, 460, GxEPD_RED);
        
        display.drawFastHLine(10, 330, 460, GxEPD_BLACK);
        display.drawFastHLine(10, 332, 460, GxEPD_RED);
        
        display.drawFastHLine(10, 630, 460, GxEPD_BLACK);
        display.drawFastHLine(10, 632, 460, GxEPD_RED);
        
        display.drawFastHLine(10, 788, 460, GxEPD_BLACK);
        display.drawFastHLine(10, 790, 460, GxEPD_RED);

        display.drawFastVLine(60, 340, 286, GxEPD_RED);
        
        drawRiseSun();
        drawWeather();
        drawWeatherForDay();
        drawEventsForDay();
        drawWeekLine();
        drawWeekEvents();
        drawLastRefresh();
        /// END DRAW
    } while (display.nextPage());
}


bool needRefresh = false;

// poslední zpracované okamžiky
int lastDay = -1;           // pro půlnoc
int lastNoonDay = -1;       // pro poledne (aby se spustilo 1× denně)

// scheduler kalendáře
unsigned long lastCalendarCheck = 0;
const unsigned long CALENDAR_INTERVAL = 1UL * 60UL * 1000UL; // 1 minut
//const unsigned long CALENDAR_INTERVAL = 5UL * 60UL * 1000UL; // 5 minut

// (volitelné) watchdog refresh – pojistka
unsigned long lastFullRefresh = 0;
const unsigned long FULL_REFRESH_INTERVAL = 6UL * 60UL * 60UL * 1000UL; // 6 h



void setup()
{
  Serial.begin(115200);

  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(CLEAR_PIN, INPUT_PULLUP);

  loadConfig();

  auto cause = esp_sleep_get_wakeup_cause();
  Serial.println("Wakeup cause: " + String(cause));
  
  if (digitalRead(CLEAR_PIN) == LOW) 
  {
    delay(3000);
    if (digitalRead(CLEAR_PIN) == LOW) 
    {
      Serial.println("CLEAR CONFIG");
      clearConfig();
    }
  }

  if (!hasWiFi()) 
  {
    Serial.println("NO WiFi → CONFIG");
    startConfig();
  }

  if (cause == ESP_SLEEP_WAKEUP_EXT1)
  {
    Serial.println("Wake by BUTTON");
    unsigned long t = millis();
    while (digitalRead(RESET_PIN) == LOW)
    {
      if (millis() - t > 3000)
      {
        Serial.println("LONG PRESS → CONFIG");
        startConfig();
      }
    }
    Serial.println("SHORT PRESS → RESTART");
    ESP.restart();
    return;
  }

  if (digitalRead(RESET_PIN) == LOW)
  {
    delay(3000);
    if (digitalRead(RESET_PIN) == LOW)
    {
      Serial.println("BOOT HOLD → CONFIG");
      startConfig();
    }
    Serial.println("BOOT CLICK → RESTART");
    ESP.restart();
    return;
  }

  Serial.println("NORMAL START");

  if (!connectWiFi())
  {
    ESP.restart();
  }

  syncTime();

  Serial.println("GET NAMEDAY BEGIN");
  nameDay = getNameDayCZ(timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900);
  Serial.println("GET NAMEDAY END");

  Serial.println("CAL SUN");
  CalcSun(lat, lon, sunRise, sunSet);

  Serial.println("GETTING WEATHER BEGIN");
  getWeatherWeekAfternoon(lat, lon, weatherResult);
  Serial.println("GETTING WEATHER END");

  Serial.println("INIT SPI DISPLAY");
  SPI.begin(14, -1, 13, CS_PIN);

  pinMode(CS_PIN, OUTPUT);
  pinMode(DC_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  pinMode(BUSY_PIN, INPUT);

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(RST_PIN, LOW);
  delay(20);
  digitalWrite(RST_PIN, HIGH);
  delay(20);

  Serial.println("DOWNLOAD CALENDAR BEGIN");
  if (calendar.fetchICS(ics))
  {
    Serial.println("ICS OK");
    calendar.storeWeek();
  }
  else
  {
    Serial.println("ICS FAILED!");
  }

  display.init(115200);

  Serial.println("INIT DISPLAY DONE");

  Serial.println("DRAWING...");
  display.setRotation(1);
  drawAll();
  display.hibernate();
  Serial.println("DRAW DONE");

  if (getLocalTime(&timeinfo))
  {
      lastDay = timeinfo.tm_mday;
      lastNoonDay = timeinfo.tm_mday;
  }
  else
  {
      // fallback – když selže čas
      lastDay = -1;
      lastNoonDay = -1;
  }  

}



bool isNewDay(const struct tm& t)
{
    if (lastDay == -1)  // první průchod po bootu
    {
        lastDay = t.tm_mday;
        return true;    // klidně inicializační refresh
    }

    if (t.tm_mday != lastDay)
    {
        lastDay = t.tm_mday;
        return true;
    }

    return false;
}


bool isNoonTrigger(const struct tm& t)
{
    // spustíme kdykoliv po 12:00, ale jen jednou za den
    if (t.tm_hour >= 12)
    {
        if (lastNoonDay != t.tm_mday)
        {
            lastNoonDay = t.tm_mday;
            return true;
        }
    }
    return false;
}


bool shouldCheckCalendar(unsigned long now)
{
    if (now - lastCalendarCheck >= CALENDAR_INTERVAL)
    {
        lastCalendarCheck = now;
        return true;
    }
    return false;
}


void loop()
{
    //struct tm timeinfo;

    // --- čas ---
    if (!getLocalTime(&timeinfo))
    {
        // NTP/WiFi problém → nic nespouštěj, jen čekej
        delay(1000);
        return;
    }

    unsigned long now = millis();

    // =========================
    // 🕛 1) ZMĚNA DNE
    // =========================
    if (isNewDay(timeinfo))
    {
        Serial.println("NEW DAY DETECT!");
        needRefresh = true;
    }

    // =========================
    // 🕛 2) POLEDNE
    // =========================
    if (isNoonTrigger(timeinfo))
    {
        Serial.println("NOON DETECT!");
        needRefresh = true;
    }

    // =========================
    // 📅 3) KALENDÁŘ
    // =========================
    if (shouldCheckCalendar(now))
    {
        Serial.println("CALENDAR!");
        if (calendar.fetchICS(ics))
        {
            if (calendar.isWeekChanged())
            {
                Serial.println("CALENDAR CHANGE DETECT!");
                calendar.storeWeek();
                needRefresh = true;
            }
        }
    }

    // =========================
    // 🛟 4) WATCHDOG REFRESH (volitelné)
    // =========================
    if (now - lastFullRefresh >= FULL_REFRESH_INTERVAL)
    {
        Serial.println("WATCHDOG RUN");
        lastFullRefresh = now;
        needRefresh = true;
    }

    // =========================
    // 🖥️ 5) VYKRESLENÍ
    // =========================
    if (needRefresh)
    {
        needRefresh = false;
        Serial.println("TOTAL REFRESHING");

        syncTime();

        Serial.println("GET NAMEDAY BEGIN");
        nameDay = getNameDayCZ(timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900);
        Serial.println("GET NAMEDAY END");

        Serial.println("CAL SUN");
        CalcSun(lat, lon, sunRise, sunSet);

        Serial.println("GETTING WEATHER BEGIN");
        getWeatherWeekAfternoon(lat, lon, weatherResult);
        Serial.println("GETTING WEATHER END");

        display.init(115200);

        Serial.println("DRAWING...");
        drawAll();
        Serial.println("DRAW DONE");

        display.hibernate();
        Serial.println("TOTAL REFRESHING DONE");
    }
}
















