#pragma once
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

class MoonCalc 
{

public:

  static void initTime()
  {
    configTime(3600, 3600, "cz.pool.ntp.org", "pool.ntp.org");
    struct tm t;
    while (!getLocalTime(&t)) delay(200);
  }


  static double julianDate(int y, int m, int d, double hour)
  {
    if (m <= 2) { y--; m += 12; }
    int A = y / 100;
    int B = 2 - A + A / 4;
    return floor(365.25 * (y + 4716))
         + floor(30.6001 * (m + 1))
         + d + B - 1524.5 + hour / 24.0;
  }


  static void moonPosition(double jd, double &ra, double &dec)
  {
    double d = jd - 2451545.0;
    double L = fmod(218.3164477 + 13.17639648 * d, 360.0);
    double M = fmod(134.9633964 + 13.06499295 * d, 360.0);
    double F = fmod(93.2720950  + 13.22935024 * d, 360.0);
    double lon = L
      + 6.289 * sin(M * DEG_TO_RAD)
      + 1.274 * sin((2*L - M) * DEG_TO_RAD)
      + 0.658 * sin(2*L * DEG_TO_RAD)
      + 0.214 * sin(2*M * DEG_TO_RAD)
      - 0.186 * sin(L * DEG_TO_RAD)
      - 0.114 * sin(2*F * DEG_TO_RAD);
    double lat = 5.128 * sin(F * DEG_TO_RAD)
               + 0.280 * sin((M + F) * DEG_TO_RAD)
               + 0.277 * sin((M - F) * DEG_TO_RAD)
               + 0.173 * sin((2*L - F) * DEG_TO_RAD);
    double eps = 23.439291 - 0.00001314 * d;
    double x = cos(lon*DEG_TO_RAD)*cos(lat*DEG_TO_RAD);
    double y = sin(lon*DEG_TO_RAD)*cos(lat*DEG_TO_RAD);
    double z = sin(lat*DEG_TO_RAD);
    double y2 = y*cos(eps*DEG_TO_RAD) - z*sin(eps*DEG_TO_RAD);
    double z2 = y*sin(eps*DEG_TO_RAD) + z*cos(eps*DEG_TO_RAD);
    ra  = atan2(y2, x);
    dec = asin(z2);
  }


  static double moonAltitude(double jd, double lat, double lon)
  {
    double ra, dec;
    moonPosition(jd, ra, dec);
    double D = jd - 2451545.0;
    double T = D / 36525.0;
    double GMST = fmod(280.46061837
      + 360.98564736629 * D
      + 0.000387933 * T*T
      - (T*T*T)/38710000.0, 360);
    double LST = (GMST + lon) * DEG_TO_RAD;
    double HA = LST - ra;
    double alt = asin(
      sin(lat*DEG_TO_RAD)*sin(dec) +
      cos(lat*DEG_TO_RAD)*cos(dec)*cos(HA)
    );
    return alt * RAD_TO_DEG;
  }


  static bool getMoonStatus(double lat, double lon)
  {
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    int y = t.tm_year + 1900;
    int m = t.tm_mon + 1;
    int d = t.tm_mday;
    double hour = t.tm_hour + t.tm_min / 60.0 + t.tm_sec / 3600.0;
    double jd = julianDate(y, m, d, hour);
    double alt = moonAltitude(jd, lat, lon);
    const double HORIZON = 0.0;
    return (alt > HORIZON);
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


  static bool compute(double lat, double lon, String &riseStr, String &setStr)
  {
    time_t now = time(nullptr);
    struct tm t;
    if (!getLocalTime(&t)) return false;
    int y = t.tm_year + 1900;
    int m = t.tm_mon + 1;
    int d = t.tm_mday;
    int tz = t.tm_isdst ? 2 : 1;
    const double HORIZON = -0.3;
    struct tm t0 = t;
    t0.tm_hour = 0;
    t0.tm_min = 0;
    t0.tm_sec = 0;
    time_t dayStart = mktime(&t0);
    double prev = moonAltitude(julianDate(y,m,d,0), lat, lon);
    double bestRise = -1;
    double bestSet  = -1;
    double bestRiseDiff = 1e9;
    double bestSetDiff  = 1e9;
    for(double h=0.05; h<=48; h+=0.05)
    {
      int dayOffset = (int)(h / 24);
      double hourInDay = fmod(h, 24);
      double jd = julianDate(y, m, d + dayOffset, hourInDay);
      double alt = moonAltitude(jd, lat, lon);
      if(prev < HORIZON && alt >= HORIZON)
      {
        double h0 = h - 0.05;
        double a0 = prev;
        double a1 = alt;
        double frac = a0 / (a0 - a1);
        double eventH = h0 + frac * 0.05;
        // 🔥 OPRAVA – správný čas události
        time_t eventTime = dayStart + (time_t)(eventH * 3600);
        double diff = difftime(eventTime, now);
        if(diff >= 0 && diff < bestRiseDiff)
        {
          bestRiseDiff = diff;
          bestRise = eventH;
        }
      }
      if(prev > HORIZON && alt <= HORIZON)
      {
        double h0 = h - 0.05;
        double a0 = prev;
        double a1 = alt;
        double frac = a0 / (a0 - a1);
        double eventH = h0 + frac * 0.05;
        // 🔥 OPRAVA – správný čas události
        time_t eventTime = dayStart + (time_t)(eventH * 3600);
        double diff = difftime(eventTime, now);
        if(diff >= 0 && diff < bestSetDiff)
        {
          bestSetDiff = diff;
          bestSet = eventH;
        }
      }
      prev = alt;
    }
    if (bestRise < 0 || bestSet < 0) return false;
    double rise = bestRise + tz;
    double set  = bestSet  + tz;
    rise = fmod(rise, 24);
    set  = fmod(set, 24);
    riseStr = toHHMM(rise);
    setStr  = toHHMM(set);
    return true;
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
};

