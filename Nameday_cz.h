#ifndef NAMEDAY_CZ_H
#define NAMEDAY_CZ_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

String getNameDayCZ(int day, int month, int year) 
{
  HTTPClient http;
  //String url = "https://svatkyapi.cz/api/day/";
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", year, month, day);
  String url = "https://svatkyapi.cz/api/day/" + String(dateStr);  
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != 200) 
  {
    http.end();
    // chyba API
    Serial.println("GetNameDay > Chyba API");
    return " ";
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) 
  {
    // JSON chyba
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
  // neznamy / nezjisteno
  Serial.println("GetNameDay > Neznama chyba");
  return " ";
}

#endif