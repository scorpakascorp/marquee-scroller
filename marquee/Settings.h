/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/******************************************************************************
 * This is designed for the Wemos D1 ESP8266
 * Wemos D1 Mini:  https://amzn.to/2qLyKJd
 * MAX7219 Dot Matrix Module 4-in-1 Display For Arduino
 * Matrix Display:  https://amzn.to/2HtnQlD
 ******************************************************************************/
/******************************************************************************
 * NOTE: The settings here are the default settings for the first loading.
 * After loading you will manage changes to the settings via the Web Interface.
 * If you want to change settings again in the settings.h, you will need to
 * erase the file system on the Wemos or use the “Reset Settings” option in
 * the Web Interface.
 ******************************************************************************/
#ifndef SETTINGS_H
#define SETTINGS_H

//******************************
// Start Settings
//******************************
#define VERSION "2.18"

#define HOSTNAME "CLOCK-"
//#define CONFIG "/conf.txt"
#define CONFIG_JSON "/conf.json"
#define BUZZER_PIN D2

// Display Settings
// CLK -> D5 (SCK)
// CS  -> D6
// DIN -> D7 (MOSI)
const int pinCS = D6;                      // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf
                                           // http://arduino.cc/en/Reference/SPI )
const int numberOfHorizontalDisplays = 8;  // default 4 for standard
                                           // 4 x 1 display Max size of 16
const int numberOfVerticalDisplays = 1;    // default 1 for a single row height
const int WEBSERVER_PORT = 80;             // The port you can access
                                           // this device on over HTTP
const boolean WEBSERVER_ENABLED = true;    // Device will provide a web
                                           // interface via http://[ip]:[port]/
/* set ledRotation for LED Display panels (3 is default)
0: no rotation
1: 90 degrees clockwise
2: 180 degrees
3: 90 degrees counter clockwise (default)
*/
const int ledRotation = 3;

String TIMEDB_API_KEY = "";  // Your API Key from
                             // https://timezonedb.com/register
// Default City Location (use http://openweathermap.org/find to find city ID)
int CityIDs[] = {709930};  // Only USE ONE for weather marquee
String USER_MESSAGE = "";
boolean IS_METRIC = true;                    // false = Imperial and true = Metric
boolean IS_24HOUR = true;                    // 23:00 millitary 24 hour clock
boolean IS_PM = false;                       // Show PM indicator on Clock when in AM/PM mode
boolean WEB_INTERFACE_AUTH_ENABLED = false;  // Use Basic Authorization for
                                             // Configuration security
                                             // on Web Interface
String WEB_INTERFACE_USER = "admin";         // User account for the Web Interface
String WEB_INTERFACE_PASS = "password";      // Password for the Web Interface
int PULL_DATA_INTERVAL = 15;                 // Time in minutes between
                                             // data refresh (default 15 minutes)
int SCROLLING_INTERVAL = 1;                  // Time in minutes between scrolling data
                                             // (default 1 minutes and max is 10)
int SCROLLING_SPEED = 25;                    // In milliseconds -- Configurable by the web UI
                                             // (slow = 35, normal = 25, fast = 15, very fast = 5)
boolean IS_DOTS_BLINKING = true;             // when true the : character in the time will
                                             // flash on and off as a seconds indicator

boolean NEWS_ENABLED = true;
String NEWS_API_KEY = "";    // Get your News API Key from https://newsapi.org
String NEWS_SOURCE = "rte";  // https://newsapi.org/sources to get full list of
                             // news sources available

// (some) Default Weather Settings
String WEATHER_API_KEY = "";  // Your API Key from http://openweathermap.org/
boolean WEATHER_FEELSLIKE = true;
boolean WEATHER_DATE = true;
boolean WEATHER_CITY = false;
boolean WEATHER_CONDITION = true;
boolean WEATHER_HUMIDITY = true;
boolean WEATHER_WIND = true;
boolean WEATHER_WINDDIR = true;
boolean WEATHER_PRESSURE = false;
boolean WEATHER_HIGHLOW = false;

String TIME_TO_DISPLAY_ON = "06:30";   // 24 Hour Format HH:MM
                                       // Leave blank for always on. (ie 05:30)
String TIME_TO_DISPLAY_OFF = "23:00";  // 24 Hour Format HH:MM
                                       // Leave blank for always on.
                                       // Both must be set to work.

// Bitcoin Client - NONE or empty is off
String BC_CODE = "NONE";  // Change to USD, GBD, EUR, or NONE -- this can be
                          // managed in the Web Interface

String NBU_CODE = "NONE";

boolean ENABLE_OTA = true;  // this will allow you to load firmware to the
                            // device over WiFi (see OTA for ESP8266)
String OTA_Password = "";   // Set an OTA password here -- leave blank if you
                            // don't want to be prompted for password

int LED_BRIGHTNESS = 1;  //(This can be set from 0 - 15)

#endif
