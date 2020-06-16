#ifndef MARQUEESCROLL_H
#define MARQUEESCROLL_H
#pragma once

#include <Adafruit_GFX.h>  // --> https://github.com/adafruit/Adafruit-GFX-Library
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <Max72xxPanel.h>  // --> https://github.com/markruys/arduino-Max72xxPanel
#include <SPI.h>
#include <WiFiManager.h>  // --> https://github.com/tzapu/WiFiManager
#include <Wire.h>

#include "BitcoinApiClient.h"
#include "NewsApiClient.h"
#include "OpenWeatherMapClient.h"
#include "Settings.h"
#include "TimeDB.h"
#include "NBUStatClient.h"

boolean athentication();
void centerPrint(String msg, boolean extraStuff = false);
void checkDisplay();
String decodeHtmlString(String msg);
void displayMessage(String message);
void displayWeatherData();
void enableDisplay(boolean enable);
void flashLED(int number, int delayTime);
int getMinutesFromLastDisplay();
int getMinutesFromLastRefresh();
String getPressureSymbol();
String getSpeedSymbol();
String getTempSymbol();
String getTempSymbolWeb();
String getTimeTillUpdate();
void getUpdatedData();
int8_t getWifiQuality();
void handleBitcoinConfigure();
void handleConfigure();
void handleDisplay();
void handleForgetWifi();
void handleSaveMain();
void handleNewsConfigure();
void handlePull();
void handleSaveBitcoin();
void handleSaveNews();
void handleSaveWideClock();
void handleSystemReset();
void handleWideClockConfigure();
String hourMinutes(boolean isRefresh);
void redirectHome();
void scrollMessage(String msg);
String secondsIndicator(boolean isRefresh);
void sendFooter();
void sendHeader();
bool writeConfigJson();
bool readConfigJson();

// declairing prototypes
void configModeCallback(WiFiManager *myWiFiManager);
int8_t getWifiQuality();

// LED Settings
const int offset = 1;
int refresh = 0;
String message = "hello";
int spacer = 1;          // dots between letters
int width = 5 + spacer;  // The font width is 5 pixels + spacer
Max72xxPanel matrix =
    Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
String Wide_Clock_Style = "1";  // 1="hh:mm Temp", 2="hh:mm:ss", 3="hh:mm"
float UtcOffset;                // time zone offsets that correspond with the CityID above
                                // (offset from GMT)
const String scrollSpacer = " --- ";

// Time
TimeDB TimeDBclient("");
String lastMinute = "xx";
int displayRefreshCount = 1;
long lastEpoch = 0;
long firstEpoch = 0;
long displayOffEpoch = 0;
boolean displayOn = true;

// News Client
NewsApiClient newsClient(NEWS_API_KEY, NEWS_SOURCE);
int newsIndex = 0;

// Weather Client
OpenWeatherMapClient weatherClient(WEATHER_API_KEY, CityIDs, 1, IS_METRIC);

// Bitcoin Client
BitcoinApiClient bitcoinClient;

// NBUStat Client
NBUStatClient NBUClient;

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;

// // Json settings
// const size_t capacity = 2048;
// DynamicJsonDocument doc(capacity);

static const char WEB_TOP[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel = "stylesheet" href = "https://www.w3schools.com/w3css/4/w3.css">
  <link rel = "stylesheet" href = "https://www.w3schools.com/lib/w3-theme-blue-grey.css">
  <link rel = "stylesheet" href = "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.8.1/css/all.min.css">
</head>
<body>
  <nav class="w3-sidebar w3-bar-block w3-card" style="margin-top:88px" id="mySidebar">
    <div class="w3-container w3-theme-d2">
      <span onclick="closeSidebar()" class="w3-button w3-display-topright w3-large">
        <i class="fas fa-times"></i>
      </span>
      <div class="w3-padding">Menu</div>
    </div>
)=====";

static const char WEB_ACTIONS[] PROGMEM = R"=====(
  <a class="w3-bar-item w3-button" href="/"><i class="fas fa-home"></i> Home</a>
  <a class="w3-bar-item w3-button" href="/configure"><i class="fas fa-cog"></i> Configure</a>
  <a class="w3-bar-item w3-button" href="/configurenews"><i class="far fa-newspaper"></i> News</a>
  <a class="w3-bar-item w3-button" href="/configurebitcoin"><i class="fab fa-bitcoin"></i> Bitcoin</a>
  <a class="w3-bar-item w3-button" href="/pull"><i class="fas fa-cloud-download-alt"></i> Refresh Data</a>
  <a class="w3-bar-item w3-button" href="/systemreset" onclick="return confirm(\"Do you want to reset to default weather settings?\")"><i class="fas fa-undo"></i> Reset Settings</a>
  <a class="w3-bar-item w3-button" href="/forgetwifi" onclick="return confirm(\"Do you want to forget to WiFi connection?\")"><i class="fas fa-wifi"></i> Forget WiFi</a>
  <a class="w3-bar-item w3-button" href="/update"><i class="fas fa-wrench"></i> Firmware Update</a>
  <a class="w3-bar-item w3-button" href="https://github.com/scorpakascorp/marquee-scroller" target="_blank"><i class="fas fa-question-circle"></i> About</a>
)=====";

static const char CHANGE_FORM1[] PROGMEM = R"=====(
  <form class="w3-container" action="/savemain" method="get"><h2>Configure:</h2>
  <label>TimeZone DB API Key (get from <a href="https://timezonedb.com/register" target="_BLANK">here</a>)</label>
  <input class="w3-input w3-border w3-margin-bottom" type="text" name="TIMEDB_API_KEY" value="%TIMEDB_API_KEY%" maxlength="60">
  <label>OpenWeatherMap API Key (get from <a href="https://openweathermap.org/" target="_BLANK">here</a>)</label>
  <input class="w3-input w3-border w3-margin-bottom" type="text" name="WEATHER_API_KEY" value="%WEATHERKEY%" maxlength="70">
  <p><label>%CITYNAME1% (<a href="http://openweathermap.org/find" target="_BLANK"><i class="fas fa-search"></i> Search for City ID</a>)</label>
  <input class="w3-input w3-border w3-margin-bottom" type="text" name="city1" value="%CITY1%" onkeypress="return isNumberKey(event)"></p>
  <p><input name="metric" class="w3-check w3-margin-top" type="checkbox" %CHECKED%> Use Metric (Celsius)</p>
  <p><input name="showfeelslike" class="w3-check w3-margin-top" type="checkbox" %FEELSLIKE_CHECKED%> Display (feels like)</p>
  <p><input name="showdate" class="w3-check w3-margin-top" type="checkbox" %DATE_CHECKED%> Display Date</p>
  <p><input name="showcity" class="w3-check w3-margin-top" type="checkbox" %CITY_CHECKED%> Display City Name</p>
  <p><input name="showhighlow" class="w3-check w3-margin-top" type="checkbox" %HIGHLOW_CHECKED%> Display Daily High/Low Temperatures</p>
  <p><input name="showcondition" class="w3-check w3-margin-top" type="checkbox" %CONDITION_CHECKED%> Display Weather Condition</p>
  <p><input name="showhumidity" class="w3-check w3-margin-top" type="checkbox" %HUMIDITY_CHECKED%> Display Humidity</p>
  <p><input name="showwind" class="w3-check w3-margin-top" type="checkbox" %WIND_CHECKED%> Display Wind</p>
  <p><input name="showpressure" class="w3-check w3-margin-top" type="checkbox" %PRESSURE_CHECKED%> Display Barometric Pressure</p>
  <p><input name="IS_24HOUR" class="w3-check w3-margin-top" type="checkbox" %IS_24HOUR_CHECKED%> Use 24 Hour Clock (military time)</p>
)=====";

static const char CHANGE_FORM2[] PROGMEM = R"=====(
  <p><input name="IS_PM" class="w3-check w3-margin-top" type="checkbox" %IS_PM_CHECKED%> Show PM indicator (only 12h format)</p>
  <p><input name="flashseconds" class="w3-check w3-margin-top" type="checkbox" %FLASHSECONDS%> Flash : in the time</p>
  <p><label>Marquee Message (up to 60 chars)</label><input class="w3-input w3-border w3-margin-bottom" type="text" name="marqueeMsg" value="%USER_MESSAGE%" maxlength="60"></p>
  <p><label>Start Time </label><input name="startTime" type="time" value="%TIME_TO_DISPLAY_ON%"></p>
  <p><label>End Time </label><input name="endTime" type="time" value="%TIME_TO_DISPLAY_OFF%"></p>
  <p>Display Brightness <input class="w3-border w3-margin-bottom" name="LED_BRIGHTNESS" type="number" min="0" max="15" value="%LED_BRIGHTNESS%"></p>
  <p>Display Scroll Speed <select class="w3-option w3-padding" name="SCROLLING_SPEED">%SCROLLING_SPEED%</select></p>
  <p>Minutes Between Refresh Data <select class="w3-option w3-padding" name="refresh">%OPTIONS%</select></p>
  <p>Minutes Between Scrolling Data <input class="w3-border w3-margin-bottom" name="refreshDisplay" type="number" min="1" max="10" value="%REFRESH_DISPLAY%"></p>
)=====";

static const char CHANGE_FORM3[] PROGMEM = R"=====(
  <hr><p><input name="isBasicAuth" class="w3-check w3-margin-top" type="checkbox" %IS_BASICAUTH_CHECKED%> Use Security Credentials for Configuration Changes</p>
  <p><label>Marquee User ID (for this web interface)</label><input class="w3-input w3-border w3-margin-bottom" type="text" name="userid" value="%USERID%" maxlength="20"></p>
  <p><label>Marquee Password </label><input class="w3-input w3-border w3-margin-bottom" type="password" name="stationpassword" value="%STATIONPASSWORD%"></p>
  <p><button class="w3-button w3-block w3-green w3-section w3-padding" type="submit">Save</button></p></form>
  <script>function isNumberKey(e){
  var h = e.which ? e.which : event.keyCode;
  return !(h > 31 && (h < 48 || h > 57))
}</script>
)=====";

static const char BITCOIN_FORM[] PROGMEM = R"=====(
  <form class="w3-container" action="/savebitcoin" method="get"><h2>Bitcoin Configuration:</h2>
  <p>Select Bitcoin Currency <select class="w3-option w3-padding" name="bitcoincurrency">%BITCOINOPTIONS%</select></p>
  <button class="w3-button w3-block w3-grey w3-section w3-padding" type="submit">Save</button></form>
)=====";

static const char CURRENCY_OPTIONS[] PROGMEM = R"=====(
    <option value="NONE">NONE</option>
    <option value="USD">United States Dollar</option>
    <option value="AUD">Australian Dollar</option>
    <option value="BRL">Brazilian Real</option>
    <option value="CAD">Canadian Dollar</option>
    <option value="CNY">Chinese Yuan</option>
    <option value="EUR">Euro</option>
    <option value="GBP">British Pound Sterling</option>
    <option value="XAU">Gold (troy ounce)</option>
)=====";

static const char WIDECLOCK_FORM[] PROGMEM = R"=====(
  <form class="w3-container" action="/savewideclock" method="get"><h2>Wide Clock Configuration:</h2>
  <p>Wide Clock Display Format <select class="w3-option w3-padding" name="wideclockformat">%WIDECLOCKOPTIONS%</select></p>
  <button class="w3-button w3-block w3-grey w3-section w3-padding" type="submit">Save</button></form>
)=====";

static const char NEWS_FORM1[] PROGMEM = R"=====(
  <form class="w3-container" action="/savenews" method="get"><h2>News Configuration:</h2>
  <p><input name="displaynews" class="w3-check w3-margin-top" type="checkbox" %NEWS_ENABLED%> Display News Headlines</p>
  <label>News API Key (get from <a href="https://newsapi.org/" target="_BLANK">here</a>)</label>
  <input class="w3-input w3-border w3-margin-bottom" type="text" name="NEWS_API_KEY" value="%NEWS_API_KEY%" maxlength="60">
  <p>Select News Source <select class="w3-option w3-padding" name="newssource" id="newssource"></select></p>
  <script>var s="%NEWS_SOURCE%";
var tt;
var xmlhttp = new XMLHttpRequest();
xmlhttp.open("GET", "https://raw.githubusercontent.com/Qrome/marquee-scroller/master/sources.json", !0);
xmlhttp.onreadystatechange = function() {
  if (xmlhttp.readyState == 4) {
    if (xmlhttp.status == 200) {
      var obj = JSON.parse(xmlhttp.responseText);
      obj.sources.forEach(t)
    }
  }
};
xmlhttp.send();
function t(it) {
  if (it != null) {
    if (s == it.id) {
      se = " selected"
    } else {
      se = ""
    }
    tt += "<option" + se + ">" + it.id + "</option>";
    document.getElementById("newssource").innerHTML = tt
  }
}</script>
  <button class="w3-button w3-block w3-grey w3-section w3-padding" type="submit">Save</button></form>
)=====";

const int TIMEOUT = 500; // 500 = 1/2 second
int timeoutCount = 0;

// Change the externalLight to the pin you wish to use if other than the Built-in LED
int externalLight = LED_BUILTIN;  // LED_BUILTIN is is the built in LED on the Wemos

#endif
