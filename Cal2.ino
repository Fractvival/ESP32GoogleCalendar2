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
#include "Fonts/fontinc.h"

// ===== ICS =====
float lat = 0;
float lon = 0;
String ics = "";

#define CLEAR_PIN 4 // smaze komplet nastaveni
#define RESET_PIN 2 // reset desky ESP32

#define CS_PIN   5
#define DC_PIN   12
#define RST_PIN  16
#define BUSY_PIN 21

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(
  GxEPD2_750c_Z08(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN)
);

const unsigned char sunIcon[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x0c, 0x18, 0x30, 0x0e, 
	0x00, 0x70, 0x07, 0x7e, 0xe0, 0x02, 0xff, 0x40, 0x01, 0xe7, 0x80, 0x03, 0x81, 0xc0, 0x03, 0x81, 
	0xc0, 0x7b, 0x00, 0xde, 0x7b, 0x00, 0xde, 0x03, 0x81, 0xc0, 0x03, 0x81, 0xc0, 0x01, 0xe7, 0x80, 
	0x02, 0xff, 0x40, 0x07, 0x7e, 0xe0, 0x0e, 0x00, 0x70, 0x1c, 0x18, 0x30, 0x08, 0x18, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00
};

const unsigned char moonIcon[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x0f, 0xfe, 0x00, 0x03, 0x9f, 0x80, 0x01, 
	0xc3, 0x80, 0x01, 0xc1, 0xc0, 0x00, 0xe0, 0xe0, 0x00, 0x60, 0xe0, 0x00, 0x70, 0x60, 0x00, 0x70, 
	0x70, 0x00, 0x70, 0x70, 0x00, 0x70, 0x70, 0x00, 0x70, 0x70, 0x00, 0x60, 0x60, 0x00, 0x60, 0xe0, 
	0x00, 0xe0, 0xe0, 0x01, 0xc1, 0xc0, 0x01, 0xc3, 0x80, 0x03, 0x9f, 0x00, 0x0f, 0xfe, 0x00, 0x03, 
	0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char isMoon[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x03, 0xc0, 0x00, 0x07, 0xc0, 0x00, 0x0f, 
	0xc0, 0x00, 0x1c, 0xe0, 0x00, 0x1c, 0xe0, 0x00, 0x38, 0x70, 0x00, 0x38, 0x70, 0x00, 0x30, 0x38, 
	0x00, 0x30, 0x3c, 0x00, 0x30, 0x1f, 0x00, 0x30, 0x0f, 0xc0, 0x38, 0x03, 0xfc, 0x38, 0x00, 0xfc, 
	0x1c, 0x00, 0x38, 0x1e, 0x00, 0x38, 0x0f, 0x00, 0xf0, 0x07, 0xc3, 0xe0, 0x03, 0xff, 0xc0, 0x00, 
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char noMoon[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x03, 0xc0, 0x00, 0x47, 0xc0, 0x00, 0xe7, 
	0xc0, 0x00, 0xf0, 0xe0, 0x00, 0x78, 0xe0, 0x00, 0x3c, 0x70, 0x00, 0x3e, 0x70, 0x00, 0x3f, 0x38, 
	0x00, 0x37, 0x9c, 0x00, 0x33, 0xcf, 0x00, 0x31, 0xe7, 0xc0, 0x38, 0xf3, 0xfc, 0x38, 0x78, 0xfc, 
	0x1c, 0x3c, 0x38, 0x1e, 0x1e, 0x38, 0x0f, 0x0f, 0x30, 0x07, 0xc7, 0x80, 0x03, 0xff, 0xc0, 0x00, 
	0xff, 0xe0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x60
};

const unsigned char sunny[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x0c, 0x18, 0x30, 0x0e, 
	0x00, 0x70, 0x07, 0x7e, 0xe0, 0x02, 0xff, 0x40, 0x01, 0xe7, 0x80, 0x03, 0x81, 0xc0, 0x03, 0x81, 
	0xc0, 0x7b, 0x00, 0xde, 0x7b, 0x00, 0xde, 0x03, 0x81, 0xc0, 0x03, 0x81, 0xc0, 0x01, 0xe7, 0x80, 
	0x02, 0xff, 0x40, 0x07, 0x7e, 0xe0, 0x0e, 0x00, 0x70, 0x1c, 0x18, 0x30, 0x08, 0x18, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00
};

const unsigned char partly[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x10, 0x0c, 0x18, 0x38, 0x0e, 
	0x00, 0x70, 0x07, 0x7e, 0xe0, 0x02, 0xff, 0x40, 0x01, 0xe7, 0x80, 0x03, 0x81, 0xc0, 0x03, 0x81, 
	0xc0, 0x1f, 0x80, 0xde, 0x3f, 0xc0, 0xde, 0x79, 0xe1, 0xc0, 0x70, 0xf9, 0xc0, 0x60, 0x7f, 0x80, 
	0x60, 0x1f, 0x40, 0x70, 0x0e, 0xe0, 0x78, 0x1c, 0x70, 0x3f, 0xfc, 0x30, 0x1f, 0xf8, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char cloudy[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x01, 
	0xff, 0x80, 0x03, 0xc7, 0xc0, 0x03, 0x81, 0xc0, 0x07, 0x00, 0xe0, 0x0f, 0x00, 0xe0, 0x3e, 0x00, 
	0x60, 0x38, 0x00, 0x78, 0x70, 0x00, 0x7c, 0x60, 0x00, 0x1e, 0x60, 0x00, 0x0e, 0x60, 0x00, 0x06, 
	0x70, 0x00, 0x0e, 0x78, 0x00, 0x1e, 0x3f, 0xff, 0xfc, 0x0f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char rain[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 0xe7, 0x80, 0x07, 
	0x81, 0xc0, 0x1f, 0x81, 0xc0, 0x1c, 0x00, 0xf0, 0x38, 0x00, 0xf8, 0x30, 0x00, 0x3c, 0x30, 0x00, 
	0x1c, 0x30, 0x00, 0x0c, 0x38, 0x00, 0x1c, 0x1c, 0x00, 0x3c, 0x1f, 0xff, 0xf8, 0x07, 0xff, 0xf0, 
	0x00, 0x00, 0x00, 0x06, 0x18, 0x60, 0x07, 0x1c, 0x70, 0x07, 0x1c, 0x70, 0x03, 0x8e, 0x38, 0x03, 
	0x8e, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char showers[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 
	0x39, 0x80, 0x77, 0x39, 0xc0, 0x73, 0x9d, 0xc0, 0x3b, 0x9c, 0xe0, 0x39, 0xce, 0xe0, 0x1d, 0xce, 
	0x70, 0x1c, 0xe7, 0x70, 0x0e, 0xe7, 0x38, 0x0e, 0x73, 0xb8, 0x07, 0x73, 0x9c, 0x07, 0x39, 0xdc, 
	0x03, 0xb9, 0xce, 0x03, 0x9c, 0xee, 0x01, 0x9c, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char snow[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
	0x18, 0x60, 0x06, 0x18, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0xc3, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x18, 0x60, 0x06, 0x18, 0x60, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char storm[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 0xe7, 0x80, 0x07, 
	0x81, 0xc0, 0x1f, 0x81, 0xc0, 0x1c, 0x00, 0xf0, 0x38, 0x00, 0xf8, 0x30, 0x00, 0x3c, 0x30, 0x00, 
	0x1c, 0x30, 0x00, 0x0c, 0x38, 0x00, 0x1c, 0x1c, 0x00, 0x3c, 0x1f, 0xff, 0xf8, 0x07, 0xff, 0xf0, 
	0x00, 0x00, 0x00, 0x01, 0xc3, 0x80, 0x01, 0xc7, 0x00, 0x01, 0xe7, 0xc0, 0x03, 0xe1, 0x80, 0x00, 
	0xc3, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

const unsigned char fog[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 0xe7, 0x80, 0x07, 
	0x81, 0xc0, 0x1f, 0x81, 0xc0, 0x1c, 0x00, 0xf0, 0x38, 0x00, 0xf8, 0x30, 0x00, 0x3c, 0x30, 0x00, 
	0x1c, 0x30, 0x00, 0x0c, 0x38, 0x00, 0x1c, 0x1c, 0x00, 0x3c, 0x1f, 0xff, 0xf8, 0x07, 0xff, 0xf0, 
	0x00, 0x00, 0x00, 0x07, 0xff, 0x60, 0x07, 0xff, 0x60, 0x00, 0x00, 0x00, 0x03, 0x7f, 0xc0, 0x03, 
	0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char unknown[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 
	0xff, 0x80, 0x03, 0xc3, 0xc0, 0x01, 0xc3, 0xc0, 0x00, 0x03, 0xc0, 0x00, 0x03, 0x80, 0x00, 0x07, 
	0x80, 0x00, 0x0f, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char windpower0[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x66, 0x00, 0x00, 0xc3, 0x00, 0x00, 
	0xc3, 0x00, 0x00, 0xdb, 0x00, 0x00, 0xdb, 0x00, 0x00, 0xc3, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x66, 
	0x60, 0x00, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xfc, 
	0x00, 0x00, 0x1c, 0x3f, 0xfc, 0x0c, 0x3f, 0xfc, 0x1c, 0x00, 0x0e, 0x3c, 0x00, 0xee, 0x38, 0x00, 
	0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00
};

const unsigned char windpower1[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x38, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x78, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 
	0x60, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xfc, 
	0x00, 0x00, 0x1c, 0x3f, 0xfc, 0x0c, 0x3f, 0xfc, 0x1c, 0x00, 0x0e, 0x3c, 0x00, 0xee, 0x38, 0x00, 
	0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00
};

const unsigned char windpower2[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x63, 0x00, 0x00, 0x03, 0x00, 0x00, 
	0x03, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x38, 0x00, 0x00, 0x70, 
	0x60, 0x00, 0x7f, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xfc, 
	0x00, 0x00, 0x1c, 0x3f, 0xfc, 0x0c, 0x3f, 0xfc, 0x1c, 0x00, 0x0e, 0x3c, 0x00, 0xee, 0x38, 0x00, 
	0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00
};

const unsigned char windpower3[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x46, 0x00, 0x00, 0x06, 0x00, 0x00, 
	0x06, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x07, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x03, 
	0x60, 0x00, 0x7e, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xfc, 
	0x00, 0x00, 0x1c, 0x3f, 0xfc, 0x0c, 0x3f, 0xfc, 0x1c, 0x00, 0x0e, 0x3c, 0x00, 0xee, 0x38, 0x00, 
	0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00
};

const unsigned char drop0[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x7e, 0x00, 0x00, 0xc3, 0x00, 0x01, 0x81, 0x80, 0x03, 0x99, 0xc0, 0x07, 0x3c, 0xe0, 0x07, 0x3c, 
	0xe0, 0x0f, 0x24, 0xf0, 0x0f, 0x24, 0xf0, 0x0f, 0x24, 0xf0, 0x0f, 0x3c, 0xf0, 0x0f, 0x3c, 0xf0, 
	0x07, 0x99, 0xe0, 0x07, 0x81, 0xe0, 0x03, 0xc3, 0xc0, 0x01, 0xff, 0x80, 0x00, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


const unsigned char drop1[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x7e, 0x00, 0x00, 0xe3, 0x00, 0x01, 0xc3, 0x80, 0x03, 0x03, 0xc0, 0x07, 0x03, 0xe0, 0x07, 0x63, 
	0xe0, 0x0f, 0xe3, 0xf0, 0x0f, 0xe3, 0xf0, 0x0f, 0xe3, 0xf0, 0x0f, 0xe3, 0xf0, 0x0f, 0xe3, 0xf0, 
	0x07, 0xe3, 0xe0, 0x07, 0x00, 0xe0, 0x03, 0x00, 0xc0, 0x01, 0xff, 0x80, 0x00, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char drop2[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x7e, 0x00, 0x00, 0x83, 0x00, 0x01, 0x01, 0x80, 0x03, 0x38, 0xc0, 0x07, 0xf8, 0xe0, 0x07, 0xf8, 
	0xe0, 0x0f, 0xf1, 0xf0, 0x0f, 0xe1, 0xf0, 0x0f, 0xc3, 0xf0, 0x0f, 0x87, 0xf0, 0x0f, 0x8f, 0xf0, 
	0x07, 0x1f, 0xe0, 0x07, 0x00, 0x60, 0x03, 0x00, 0x40, 0x01, 0xff, 0x80, 0x00, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char drop3[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x7e, 0x00, 0x00, 0x83, 0x00, 0x01, 0x01, 0x80, 0x03, 0xf8, 0xc0, 0x07, 0xf8, 0xe0, 0x07, 0xf8, 
	0xe0, 0x0f, 0xc3, 0xf0, 0x0f, 0xc1, 0xf0, 0x0f, 0xf0, 0xf0, 0x0f, 0xf8, 0xf0, 0x0f, 0xf8, 0xf0, 
	0x07, 0x70, 0xe0, 0x07, 0x01, 0xe0, 0x03, 0x03, 0xc0, 0x01, 0xff, 0x80, 0x00, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char air[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x07, 0xc0, 0x00, 
	0x0e, 0xe0, 0x00, 0x0c, 0x60, 0x00, 0x00, 0x60, 0x3f, 0xff, 0xc0, 0x3f, 0xff, 0xc0, 0x00, 0x00, 
	0x00, 0x3f, 0xff, 0xf0, 0x3f, 0xff, 0xf8, 0x00, 0x00, 0x0c, 0x3f, 0xf8, 0x0c, 0x3f, 0xfc, 0x0c, 
	0x00, 0x0c, 0x18, 0x00, 0x6c, 0x10, 0x00, 0x7c, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char drop[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x40, 0x00, 
	0x6e, 0xe0, 0x00, 0xc7, 0xf0, 0x01, 0xc3, 0xb0, 0x03, 0x83, 0x18, 0x03, 0x03, 0x18, 0x06, 0x03, 
	0x38, 0x06, 0x01, 0xf0, 0x0c, 0x00, 0xf0, 0x0c, 0x00, 0x30, 0x0d, 0x80, 0x30, 0x0d, 0x80, 0x30, 
	0x07, 0xc0, 0x60, 0x06, 0xf8, 0x60, 0x03, 0x38, 0xc0, 0x03, 0xc3, 0xc0, 0x00, 0xff, 0x00, 0x00, 
	0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char thermo[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x66, 0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 
	0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 0x00, 0x00, 0xe7, 0x00, 0x01, 0xc3, 0x80, 0x01, 0x81, 0x80, 
	0x01, 0x81, 0x80, 0x01, 0xc3, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


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
  float rain_mm;  
  bool valid;
};


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


#define WIND_NONE    0
#define WIND_LOW     1
#define WIND_MEDIUM  2
#define WIND_HIGH    3

#define RAIN_NONE    0
#define RAIN_LOW     1
#define RAIN_MEDIUM  2
#define RAIN_HIGH    3


#define ICONWIDTH 24

ICSCalendar ical;

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


// ===== NVS =====
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
  int y=timeinfo.tm_year+1900;
  int m=timeinfo.tm_mon+1;
  int d=timeinfo.tm_mday;
  int h=timeinfo.tm_hour;
  int min=timeinfo.tm_min;
  Serial.print("Date: ");
  Serial.print(d); Serial.print(".");
  Serial.print(m); Serial.print(".");
  Serial.println(y);
  Serial.print("Time: ");
  Serial.print(h); Serial.print(":");
  Serial.println(min);
}


void downloadICS()
{
  Serial.println("Downloading ICS...");
  if (ical.fetchICS(ics)) 
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


bool getWeatherForDay(float lat, float lon, tm timeinfo, DayResult &out) 
{
  out.valid = false;
  char targetDate[11];
  sprintf(targetDate, "%04d-%02d-%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday);
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



void drawDayNameAndNumber()
{
  DayInfo d = getDayInfo(timeinfo);
  display.setFont(&LexendPeta_Medium);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 80);
  printCZ(String(d.dayFull).c_str());

  display.setFont(&IBMPlexSerif_Medium);
  display.setCursor(320, 150);
  display.print(String(d.dayNumber).c_str());

  display.setFont(&Inter_24pt_Bold);
  display.setCursor(25, 150);
  printCZ(String(d.monthName).c_str());

}



void drawAll()
{
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        /// BEGIN DRAW

        drawDayNameAndNumber();

        /// END DRAW
    } while (display.nextPage());
}



/*
void drawRow(const Rect& rect, const struct tm& date, const char* event1, const char* event2)
{
  Rect partRect[4];
  const char* daysCZ[] = {
    "Ne", "Po", "Út", "St", "Čt", "Pá", "So"
  };  
  int leftW = (rect.w * 15) / 100;
  partRect[0] = { rect.x, rect.y, leftW, rect.h };
  Rect topRect;
  Rect bottomRect;
  int split = (partRect[0].h * 60) / 100;
  topRect.x = partRect[0].x;
  topRect.y = partRect[0].y;
  topRect.w = partRect[0].w;
  topRect.h = split;
  bottomRect.x = partRect[0].x;
  bottomRect.y = partRect[0].y + split;
  bottomRect.w = partRect[0].w;
  bottomRect.h = partRect[0].h - split;
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&CZUbuntuBold12);
  char dayStr[4];
  snprintf(dayStr, sizeof(dayStr), "%d", date.tm_mday);  
  int dayHeightFont = getTextHeightCZ("A", &CZUbuntuBold12);
  int dayWidthFont = getTextWidthCZ(dayStr, &CZUbuntuBold12);
  drawTextCentered(topRect, dayStr, dayWidthFont, dayHeightFont);  
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&Exo_SemiBold9);
  int daynameHeightFont = getTextHeightCZ("A", &Exo_SemiBold9);
  int daynameWidthFont = getTextWidthCZ(daysCZ[date.tm_wday], &Exo_SemiBold9);
  drawTextCentered(bottomRect, daysCZ[date.tm_wday], daynameWidthFont, daynameHeightFont);
  Rect right;
  right.x = rect.x + leftW;
  right.y = rect.y;
  right.w = rect.w - leftW;
  right.h = rect.h;
  int topH = ICONWIDTH + 2;
  int remainingH = right.h - topH;
  int midH = remainingH / 2;
  int botH = remainingH - midH;
  partRect[1] = 
  {
    right.x,
    right.y,
    right.w,
    topH
  };
  partRect[2] = 
  {
    right.x,
    right.y + topH,
    right.w,
    midH
  };
  partRect[3] = 
  {
    right.x,
    right.y + topH + midH,
    right.w,
    botH
  };
  display.drawRect(rect.x, rect.y, rect.w, rect.h, GxEPD_RED);
  display.drawRect(partRect[0].x, partRect[0].y,
                   partRect[0].w, partRect[0].h,
                   GxEPD_RED);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&Karla_SemiBold9);

  int fontHeight = getTextHeightCZ("A", &Karla_SemiBold9);

  bool hasEvent1 = (event1 != nullptr) && (event1[0] != '\0');
  bool hasEvent2 = (event2 != nullptr) && (event2[0] != '\0');

  if (hasEvent1 && hasEvent2)
  {
    display.setCursor(
        partRect[2].x + 2,
        partRect[2].y + fontHeight
    );
    printCZ(event1);

    display.setCursor(
        partRect[3].x + 2,
        partRect[3].y + fontHeight
    );
    printCZ(event2);
  }

  else if (hasEvent1 || hasEvent2)
  {
    const char* event = hasEvent1 ? event1 : event2;

    int combinedY = partRect[2].y;
    int combinedHeight = partRect[2].h + partRect[3].h;

    //int yCentered = combinedY + (combinedHeight / 2) + (fontHeight / 2);
    int yCentered = combinedY + (combinedHeight - fontHeight) / 2 + fontHeight - 2;

    display.setCursor(
        partRect[2].x + 2,
        yCentered
    );
    printCZ(event);
  }

  uint16_t colorGx = GxEPD_BLACK;
  DayResult dayRes;
  if (getWeatherForDay(lat, lon, date, dayRes))
  {
    switch (dayRes.icon) 
    {
      case ICON_SUNNY:
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, sunny, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_PARTLY: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, partly, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_CLOUDY: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, cloudy, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_RAIN: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, rain, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_SHOWERS: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, showers, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_SNOW: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, snow, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_STORM: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, storm, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_FOG: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, fog, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
      case ICON_UNKNOWN: 
      {
        display.drawBitmap(partRect[1].x+1, partRect[1].y+1, unknown, ICONWIDTH, ICONWIDTH, colorGx);    
        break;
      }
    }

    display.setFont(&Roboto_Condensed_Regular9);

    display.drawBitmap(partRect[1].x + ICONWIDTH + 2, partRect[1].y+1, air, ICONWIDTH, ICONWIDTH, colorGx);
    display.setTextColor(GxEPD_BLACK);
    int windHeightFont = getTextHeightCZ("A", &MarkaziText_Regular12);
    String windText = String(dayRes.wind_ms, 1);// + " m/s";
    int windWidthFont = getTextWidthCZ(windText.c_str(), &MarkaziText_Regular12);
    display.setCursor(
        partRect[1].x + (ICONWIDTH*2) + 2,
        partRect[1].y + ((partRect[1].h/2)+(windHeightFont/2))
    );
    printCZ(windText.c_str());

    display.drawBitmap(partRect[1].x + (ICONWIDTH*2) + windWidthFont + 2, partRect[1].y+1, drop, ICONWIDTH, ICONWIDTH, colorGx);
    display.setTextColor(GxEPD_BLACK);
    int dropHeightFont = getTextHeightCZ("A", &MarkaziText_Regular12);
    String dropText = String(dayRes.rain_mm, 1);// + " mm/den";
    int dropWidthFont = getTextWidthCZ(dropText.c_str(), &MarkaziText_Regular12);
    display.setCursor(
        partRect[1].x + (ICONWIDTH*3) + windWidthFont + 2,
        partRect[1].y + ((partRect[1].h/2)+(dropHeightFont/2))
    );
    printCZ(dropText.c_str());

    display.drawBitmap(partRect[1].x + (ICONWIDTH*3) + windWidthFont + dropWidthFont + 2, partRect[1].y+1, thermo, ICONWIDTH, ICONWIDTH, colorGx);
    display.setTextColor(GxEPD_BLACK);
    int thermoHeightFont = getTextHeightCZ("A", &MarkaziText_Regular12);
    String thermoText = String(dayRes.tMin, 1) + "/" + String(dayRes.tMax, 1) + "C";
    int thermoWidthFont = getTextWidthCZ(thermoText.c_str(), &MarkaziText_Regular12);
    display.setCursor(
        partRect[1].x + (ICONWIDTH*4) + windWidthFont + dropWidthFont + 2,
        partRect[1].y + ((partRect[1].h/2)+(thermoHeightFont/2))
    );
    printCZ(thermoText.c_str());
  }
}
*/

/*
void drawAll(struct tm timeinfo)
{
  const char* namemonth[12] = 
  {
    "LEDEN","ÚNOR","BŘEZEN","DUBEN","KVĚTEN","ČERVEN","ČERVENEC","SRPEN","ZÁŘÍ","ŘÍJEN","LISTOPAD","PROSINEC"
  };
  
  display.setFullWindow();
  
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_RED);
    display.setFont(&Boldonse_Regular15);
    int widthMonthText = getTextWidthCZ(namemonth[timeinfo.tm_mon],&Boldonse_Regular15);
    int xPosMonthText = (display.width() - widthMonthText) / 2;
    int yPosMonthText = 0+((display.height()/100)*11);
    display.setCursor(xPosMonthText+1, yPosMonthText+1);
    printCZ(namemonth[timeinfo.tm_mon]);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(xPosMonthText, yPosMonthText);
    printCZ(namemonth[timeinfo.tm_mon]);
    display.setFont(&Roboto_Medium20);
    int16_t _x1, _y1;
    uint16_t _w, _h;
    display.getTextBounds(String(timeinfo.tm_mday), 0, 0, &_x1, &_y1, &_w, &_h);        
    int xPosDayText = (display.width() - _w) / 2;
    int yPosDayText = yPosMonthText+_h+((display.height()/100)*4);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(xPosDayText+1, yPosDayText+1);
    display.print(String(timeinfo.tm_mday));
    display.setTextColor(GxEPD_RED);
    display.setCursor(xPosDayText, yPosDayText);
    display.print(String(timeinfo.tm_mday));
    int yPosIcons = yPosDayText-_h;
    display.drawBitmap(0, yPosIcons, sunIcon, ICONWIDTH, ICONWIDTH, GxEPD_BLACK);
    if (MoonCalc::getMoonStatus(lat, lon))
    {
      display.drawBitmap(display.width()-24, yPosIcons, isMoon, ICONWIDTH, ICONWIDTH, GxEPD_BLACK);    
    }
    else
    {
      display.drawBitmap(display.width()-24, yPosIcons, noMoon, ICONWIDTH, ICONWIDTH, GxEPD_BLACK);    
    }
    int y=timeinfo.tm_year+1900;
    int m=timeinfo.tm_mon+1;
    int d=timeinfo.tm_mday;
    int hour = timeinfo.tm_hour;
    int min = timeinfo.tm_min;
    display.setFont(&Roboto_Medium);
    String srStr, ssStr;
    MoonCalc::CalcSun(lat, lon, srStr, ssStr);
    int16_t _x2, _y2;
    uint16_t _w2, _h2;
    display.getTextBounds(srStr, 0, 0, &_x2, &_y2, &_w2, &_h2);
    int spacing = ICONWIDTH * 30 / 100;
    int xPosSunText = ICONWIDTH + spacing;
    display.setTextColor(GxEPD_RED);
    display.setCursor(xPosSunText, yPosIcons + _h2);
    display.print(srStr);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(xPosSunText, yPosIcons + (_h2 * 2) + (_h2 * 40 / 100));
    display.print(ssStr);
    String riseStr, setStr;
    MoonCalc::compute(lat, lon, riseStr, setStr);
    bool isUp = MoonCalc::getMoonStatus(lat, lon);
    if (!isUp)
    {
      time_t now = time(nullptr) - 12 * 3600;
      struct tm t;
      localtime_r(&now, &t);
      int y = t.tm_year + 1900;
      int m = t.tm_mon + 1;
      int d = t.tm_mday;
      double prev = MoonCalc::moonAltitude(
          MoonCalc::julianDate(y,m,d,0), lat, lon);
      double todayRise = -1;
      for(double h=0.05; h<=24; h+=0.05)
      {
        double alt = MoonCalc::moonAltitude(
            MoonCalc::julianDate(y,m,d,h), lat, lon);
        if(prev < -0.3 && alt >= -0.3)
        {
          double h0 = h - 0.05;
          double frac = prev / (prev - alt);
          todayRise = h0 + frac * 0.05;
          break;
        }
        prev = alt;
      }
      if (todayRise >= 0)
      {
        int tz = t.tm_isdst ? 2 : 1;
        double rise = fmod(todayRise + tz, 24);
        riseStr = MoonCalc::toHHMM(rise);
      }
    }
    int xPosMoonText = display.width() - ICONWIDTH - spacing;
    display.setTextColor(GxEPD_RED);
    display.setCursor(xPosMoonText-_w2, yPosIcons + _h2);
    display.print(riseStr);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(xPosMoonText-_w2, yPosIcons + (_h2 * 2) + (_h2 * 40 / 100));
    display.print(setStr);
    int yPosNamedayText = yPosIcons + (_h2 * 2) + (_h2 * 40 / 100) + ((display.height()*5)/100);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&CZSpaceMonoRegular12);
    String namedayStr = String(getNameDayCZ(timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900));
    int widthNamedayText = getTextWidthCZ(namedayStr.c_str(),&CZSpaceMonoRegular12);
    int heightNamedayText = getTextHeightCZ(namedayStr.c_str(),&CZSpaceMonoRegular12);
    int xPosNamedayText = (display.width() - widthNamedayText) / 2;
    display.setCursor(xPosNamedayText, yPosNamedayText);
    printCZ(namedayStr.c_str());
    int yPosHeadLine = yPosNamedayText + ((display.height()*5)/100);
    display.drawLine(0, yPosHeadLine - 3, display.width(), yPosHeadLine -3, GxEPD_BLACK);
    display.drawLine(0, yPosHeadLine, display.width(), yPosHeadLine, GxEPD_BLACK);
    String lastSyncText = "";
    lastSyncText += String(twoDigits(hour));
    lastSyncText += ":";
    lastSyncText += String(twoDigits(min));
    lastSyncText += " - ";
    lastSyncText += String(d);
    lastSyncText += ".";
    lastSyncText += String(m);
    lastSyncText += ".";
    lastSyncText += String(y);
    display.setFont(&CZProstoOneRegular5);
    int heightSyncText = getTextHeightCZ(lastSyncText.c_str(),&CZProstoOneRegular5);
    display.setCursor(0, display.height()-1);
    printCZ(lastSyncText.c_str());
    int yPosBottomLine = display.height() - 1 - heightSyncText - 5;
    display.drawLine(0, yPosBottomLine + 3, display.width(), yPosBottomLine + 3, GxEPD_BLACK);
    display.drawLine(0, yPosBottomLine, display.width(), yPosBottomLine, GxEPD_BLACK);
    int areaHeight = yPosBottomLine - yPosHeadLine;
    display.setFont(&CZPTSansRegular9);
    int _thFont = getTextHeightCZ("A", &CZPTSansRegular9);
    int rowMin = ICONWIDTH + 2 + (_thFont * 2) + 4;
    int rows = areaHeight / rowMin;
    if (rows < 1) rows = 1;
    int baseH = areaHeight / rows;
    int remainder = areaHeight % rows;
    Rect rectRow[20] = {0};
    int yy = yPosHeadLine;
    time_t now;
    time(&now);
    struct tm day;
    localtime_r(&now, &day);    
    String e1, e2;
    delay(1000);
    for (int i = 0; i < rows; i++)
    {
        int h = baseH + (i < remainder ? 1 : 0);
        rectRow[i].x = 0;
        rectRow[i].y = yy;
        rectRow[i].w = display.width();
        rectRow[i].h = h;
        struct tm tmp = day;
        addDays(tmp, i);
        ical.getEventsForDay(tmp, e1, e2);
        drawRow(rectRow[i], tmp, e1.c_str(), e2.c_str());
        display.drawLine(0, yy + h, display.width(), yy + h, GxEPD_BLACK);
        yy += h;
    }
  }
  while(display.nextPage());
}
*/


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
  downloadICS();

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

  display.init(115200);

  Serial.println("INIT DISPLAY DONE");

  Serial.println("DRAWING...");
  display.setRotation(1);
  drawAll();
  display.hibernate();
  Serial.println("DRAW DONE");

}


void loop()
{
  delay(1000);
}


/*
unsigned long lastRefresh = 0;
const unsigned long interval = 20000; // 20 sekund

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastRefresh >= interval)
  {
    lastRefresh = currentMillis;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      drawAll(timeinfo);  // tvoje funkce
    }
  }

  // sem můžeš dát další kód (BLE, WiFi, atd.)
}
*/





















