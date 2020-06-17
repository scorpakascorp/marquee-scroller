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

#include "MarqueeScroll.h"

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  // LittleFS.remove(CONFIG_JSON);
  delay(10);

  // Initialize digital pin for LED
  pinMode(externalLight, OUTPUT);

  // New Line to clear from start garbage
  Serial.println();

  readConfigJson();

  Serial.println("*setup(): Number of LED Displays: " + String(numberOfHorizontalDisplays));
  // initialize dispaly
  matrix.setIntensity(0);  // Use a value between 0 and 15 for brightness
  // matrix.setFont(&TomThumb);
  // matrix.cp437(true);

  int maxPos = numberOfHorizontalDisplays * numberOfVerticalDisplays;
  for (int i = 0; i < maxPos; i++) {
    matrix.setRotation(i, ledRotation);
    matrix.setPosition(i, maxPos - i - 1, 0);
  }

  Serial.println("*setup(): Matrix created");
  matrix.fillScreen(LOW);  // show black
  centerPrint("-*-");

  // tone(BUZZER_PIN, 415, 500);
  // delay(500 * 1.3);
  // tone(BUZZER_PIN, 466, 500);
  // delay(500 * 1.3);
  // tone(BUZZER_PIN, 370, 1000);
  // delay(1000 * 1.3);
  // noTone(BUZZER_PIN);

  for (int inx = 0; inx <= 15; inx++) {
    matrix.setIntensity(inx);
    delay(60);
  }
  for (int inx = 15; inx >= 0; inx--) {
    matrix.setIntensity(inx);
    delay(30);
  }
  delay(100);
  matrix.setIntensity(LED_BRIGHTNESS);
  // noTone(BUZZER_PIN);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it
  // around
  WiFiManager wifiManager;

  // Uncomment for testing wifi manager
  // wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  // Custom Station (client) Static IP Configuration - Set custom IP for your
  // Network (IP, Gateway, Subnet mask)
  // wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,99),
  // IPAddress(192,168,0,1), IPAddress(255,255,255,0));

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {  // new addition
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }

  // print the received signal strength:
  Serial.print("*setup(): Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");

  if (ENABLE_OTA) {
    ArduinoOTA.onStart([]() { Serial.println("*setup() OTA: Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("*setup() OTA: Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("*setup() OTA: Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    });
    ArduinoOTA.setHostname((const char *)hostname.c_str());
    if (OTA_Password != "") {
      ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
    }
    ArduinoOTA.begin();
  }

  if (WEBSERVER_ENABLED) {
    server.on("/", displayWeatherData);
    server.on("/pull", handlePull);
    server.on("/savemain", handleSaveMain);
    server.on("/savebitcoin", handleSaveBitcoin);
    server.on("/savewideclock", handleSaveWideClock);
    server.on("/savenews", handleSaveNews);
    server.on("/systemreset", handleSystemReset);
    server.on("/forgetwifi", handleForgetWifi);
    server.on("/configure", handleConfigure);
    server.on("/configurebitcoin", handleBitcoinConfigure);
    server.on("/configurewideclock", handleWideClockConfigure);
    server.on("/configurenews", handleNewsConfigure);
    server.on("/display", handleDisplay);
    server.onNotFound(redirectHome);
    serverUpdater.setup(&server, "/update", WEB_INTERFACE_USER, WEB_INTERFACE_PASS);

    // Serve static
    server.serveStatic(CONFIG_JSON, LittleFS, CONFIG_JSON);
    // Start the server
    server.begin();
    Serial.println("*setup(): Server started");
    // Print the IP address
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("*setup(): Use this URL : " + webAddress);
    scrollMessage(" v" + String(VERSION) + "  IP: " + WiFi.localIP().toString() + "  ");
  } else {
    Serial.println("*setup(): Web Interface is Disabled");
    scrollMessage("Web Interface is Disabled");
  }

  flashLED(1, 500);
}

void loop() {
  // Get some Weather Data to serve
  if ((getMinutesFromLastRefresh() >= minutesBetweenDataRefresh) ||
      lastEpoch == 0) {
    getUpdatedData();
  }
  checkDisplay();  // this will see if we need to turn it on or off for night
                   // mode.

  if (lastMinute != TimeDBclient.zeroPad(minute())) {
    lastMinute = TimeDBclient.zeroPad(minute());

    if (weatherClient.getError() != "") {
      scrollMessage(weatherClient.getError());
      return;
    }

    if (displayOn) {
      matrix.shutdown(false);
    }
    matrix.fillScreen(LOW);  // show black

    displayRefreshCount--;
    // Check to see if we need to Scroll some Data
    if (displayRefreshCount <= 0) {
      displayRefreshCount = minutesBetweenScrolling;
      String temperature = weatherClient.getTempRounded(0);
      String description = weatherClient.getDescription(0);
      description.toUpperCase();
      String msg;
      msg += scrollSpacer;

      if (WEATHER_DATE) {
        msg += TimeDBclient.getDayName() + ", ";
        msg += TimeDBclient.getMonthName() + " " + day();
        msg += scrollSpacer;
      }
      if (WEATHER_CITY) {
        msg += weatherClient.getCity(0);
        msg += scrollSpacer;
      }
      msg += "Temp: " + temperature + getTempSymbol();
      msg += scrollSpacer;

      if (WEATHER_FEELSLIKE) {
        String feels_like = weatherClient.getFeelsLikeRounded(0);
        msg += "Feels like: " + feels_like + getTempSymbol();
        msg += scrollSpacer;
      }

      // show high/low temperature
      if (WEATHER_HIGHLOW) {
        msg += "Hi:" + weatherClient.getHigh(0) + getTempSymbol() + " / Lo:" + weatherClient.getLow(0) + " " + getTempSymbol();
        msg += scrollSpacer;
      }

      if (WEATHER_CONDITION) {
        msg += description + ". ";
      }
      if (WEATHER_HUMIDITY) {
        msg += "Humid: " + weatherClient.getHumidityRounded(0) + "%  ";
      }
      if (WEATHER_WIND) {
        msg += "Wind: " + weatherClient.getDirectionText(0) + " @ " + weatherClient.getWindRounded(0) + " " + getSpeedSymbol();
        msg += scrollSpacer;
      }
      // line to show barometric pressure
      if (WEATHER_PRESSURE) {
        msg += "Press: " + weatherClient.getPressure(0) + getPressureSymbol();
        msg += scrollSpacer;
      }

      if (!USER_MESSAGE.isEmpty()) {
        msg += USER_MESSAGE;
        msg += scrollSpacer;
      }

      if (NBU_CODE != "NONE" && NBU_CODE != "") {
        msg += NBUClient.getCode() + ":" + NBUClient.getRate();
        msg += scrollSpacer;
        ;
      }

      if (NEWS_ENABLED) {
        msg += NEWS_SOURCE + ": " + newsClient.getTitle(newsIndex);
        msg += scrollSpacer;
        newsIndex += 1;
        if (newsIndex > 9) {
          newsIndex = 0;
        }
      }

      if (BC_CODE != "NONE" && BC_CODE != "") {
        msg += "Bitcoin: " + bitcoinClient.getRate() + " " + bitcoinClient.getCode();
        msg += scrollSpacer;
        ;
      }
      scrollMessage(msg);
      Serial.println(msg);
    }
  }

  String currentTime = hourMinutes(false);

  if (numberOfHorizontalDisplays >= 8) {
    if (Wide_Clock_Style == "1") {
      // On Wide Display -- show the current temperature as well
      String currentTemp = weatherClient.getTempRounded(0);
      String timeSpacer = "";
      if (currentTemp.length() <= 2)
        timeSpacer = " ";
      currentTime += timeSpacer + currentTemp + getTempSymbol();
    }
    if (Wide_Clock_Style == "2") {
      currentTime += secondsIndicator(false) + TimeDBclient.zeroPad(second());
      matrix.fillScreen(LOW);  // show black
    }
    if (Wide_Clock_Style == "3") {
      // No change this is normal clock display
    }
  }
  matrix.fillScreen(LOW);
  centerPrint(currentTime, true);

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
  if (ENABLE_OTA) {
    ArduinoOTA.handle();
  }
}

String hourMinutes(boolean isRefresh) {
  if (IS_24HOUR) {
    return hour() + secondsIndicator(isRefresh) + TimeDBclient.zeroPad(minute());
  } else {
    return hourFormat12() + secondsIndicator(isRefresh) + TimeDBclient.zeroPad(minute());
  }
}

String secondsIndicator(boolean isRefresh) {
  String rtnValue = ":";
  if (isRefresh == false && (IS_DOTS_BLINKING && (second() % 2) == 0)) {
    rtnValue = " ";
  }
  return rtnValue;
}

boolean athentication() {
  if (WEB_INTERFACE_AUTH_ENABLED) {
    return server.authenticate(WEB_INTERFACE_USER.c_str(), WEB_INTERFACE_PASS.c_str());
  }
  return true;  // Authentication not required
}

void handlePull() {
  getUpdatedData();  // this will force a data pull for new weather
  displayWeatherData();
}

void handleSaveBitcoin() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  BC_CODE = server.arg("bitcoinCurrency");
  NBU_CODE = server.arg("NBUCurrency");
  writeConfigJson();
  bitcoinClient.updateBitcoinData(BC_CODE);  // does nothing if BitCoinCurrencyCode is "NONE" or empty
  NBUClient.updateNBUStatData(NBU_CODE);
}

void handleSaveWideClock() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  if (numberOfHorizontalDisplays >= 8) {
    Wide_Clock_Style = server.arg("wideclockformat");
    writeConfigJson();
    matrix.fillScreen(LOW);  // show black
  }
  redirectHome();
}

void handleSaveNews() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  NEWS_ENABLED = server.hasArg("displaynews");
  NEWS_API_KEY = server.arg("NEWS_API_KEY");
  NEWS_SOURCE = server.arg("newssource");
  matrix.fillScreen(LOW);  // show black
  writeConfigJson();
  newsClient.updateNews();
  redirectHome();
}

void handleSaveMain() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  TIMEDB_API_KEY = server.arg("TIMEDB_API_KEY");
  WEATHER_API_KEY = server.arg("WEATHER_API_KEY");
  CityIDs[0] = server.arg("city1").toInt();
  IS_DOTS_BLINKING = server.hasArg("flashseconds");
  IS_24HOUR = server.hasArg("IS_24HOUR");
  IS_PM = server.hasArg("IS_PM");
  WEATHER_FEELSLIKE = server.hasArg("showfeelslike");
  WEATHER_DATE = server.hasArg("showdate");
  WEATHER_CITY = server.hasArg("showcity");
  WEATHER_CONDITION = server.hasArg("showcondition");
  WEATHER_HUMIDITY = server.hasArg("showhumidity");
  WEATHER_WIND = server.hasArg("showwind");
  WEATHER_PRESSURE = server.hasArg("showpressure");
  WEATHER_HIGHLOW = server.hasArg("showhighlow");
  IS_METRIC = server.hasArg("metric");
  USER_MESSAGE = decodeHtmlString(server.arg("marqueeMsg"));
  TIME_TO_DISPLAY_ON = decodeHtmlString(server.arg("startTime"));
  TIME_TO_DISPLAY_OFF = decodeHtmlString(server.arg("endTime"));
  LED_BRIGHTNESS = server.arg("LED_BRIGHTNESS").toInt();
  minutesBetweenDataRefresh = server.arg("refresh").toInt();
  minutesBetweenScrolling = server.arg("refreshDisplay").toInt();
  SCROLLING_SPEED = server.arg("SCROLLING_SPEED").toInt();
  WEB_INTERFACE_AUTH_ENABLED = server.hasArg("isBasicAuth");
  WEB_INTERFACE_USER = server.arg("userid");
  WEB_INTERFACE_PASS = server.arg("stationpassword");
  weatherClient.setMetric(IS_METRIC);
  matrix.fillScreen(LOW);  // show black
  writeConfigJson();
  getUpdatedData();  // this will force a data pull for new weather
  redirectHome();
}

void handleSystemReset() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  Serial.println("Reset System Configuration");
  if (LittleFS.remove(CONFIG)) {
    redirectHome();
    ESP.restart();
  }
}

void handleForgetWifi() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it
  // around
  redirectHome();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

void handleBitcoinConfigure() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  String form = FPSTR(BITCOIN_FORM);

  String bitcoinOptions = FPSTR(CURRENCY_OPTIONS);
  bitcoinOptions.replace(BC_CODE + "\"", BC_CODE + "\" selected");
  form.replace("%BITCOINOPTIONS%", bitcoinOptions);

  String NBUOptions = FPSTR(NBU_OPTIONS);
  NBUOptions.replace(NBU_CODE + "\"", NBU_CODE + "\" selected");
  form.replace("%NBUOPTIONS%", NBUOptions);

  server.sendContent(form);  // Send another Chunk of form

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleWideClockConfigure() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  if (numberOfHorizontalDisplays >= 8) {
    // Wide display options
    String form = FPSTR(WIDECLOCK_FORM);
    String clockOptions =
        "<option value='1'>HH:MM Temperature</option><option "
        "value='2'>HH:MM:SS</option><option value='3'>HH:MM</option>";
    clockOptions.replace(Wide_Clock_Style + "\"", Wide_Clock_Style + "\" selected");
    form.replace("%WIDECLOCKOPTIONS%", clockOptions);
    server.sendContent(form);
  }

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleNewsConfigure() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  String form = FPSTR(NEWS_FORM1);

  // String NEWS_ENABLED = "";
  // if (NEWS_ENABLED) {
  //   NEWS_ENABLED = "checked='checked'";
  // }

  String strNewsEnabled = NEWS_ENABLED ? "checked='checked'" : "";

  form.replace("%NEWS_ENABLED%", strNewsEnabled);
  form.replace("%NEWS_API_KEY%", NEWS_API_KEY);
  form.replace("%NEWS_SOURCE%", NEWS_SOURCE);
  server.sendContent(form);  // Send news form

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleConfigure() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendHeader();

  String form = FPSTR(CHANGE_FORM1);
  form.replace("%TIMEDB_API_KEY%", TIMEDB_API_KEY);
  form.replace("%WEATHERKEY%", WEATHER_API_KEY);

  String cityName = "";
  if (weatherClient.getCity(0) != "") {
    cityName = weatherClient.getCity(0) + ", " + weatherClient.getCountry(0);
  }
  form.replace("%CITYNAME1%", cityName);
  form.replace("%CITY1%", String(CityIDs[0]));

  String isFeelsLikeChecked = "";
  if (WEATHER_FEELSLIKE) {
    isFeelsLikeChecked = "checked='checked'";
  }
  form.replace("%FEELSLIKE_CHECKED%", isFeelsLikeChecked);

  String isDateChecked = "";
  if (WEATHER_DATE) {
    isDateChecked = "checked='checked'";
  }
  form.replace("%DATE_CHECKED%", isDateChecked);

  String isCityChecked = "";
  if (WEATHER_CITY) {
    isCityChecked = "checked='checked'";
  }
  form.replace("%CITY_CHECKED%", isCityChecked);

  String isConditionChecked = "";
  if (WEATHER_CONDITION) {
    isConditionChecked = "checked='checked'";
  }
  form.replace("%CONDITION_CHECKED%", isConditionChecked);

  String isHumidityChecked = "";
  if (WEATHER_HUMIDITY) {
    isHumidityChecked = "checked='checked'";
  }
  form.replace("%HUMIDITY_CHECKED%", isHumidityChecked);

  String isWindChecked = "";
  if (WEATHER_WIND) {
    isWindChecked = "checked='checked'";
  }
  form.replace("%WIND_CHECKED%", isWindChecked);

  String isPressureChecked = "";
  if (WEATHER_PRESSURE) {
    isPressureChecked = "checked='checked'";
  }
  form.replace("%PRESSURE_CHECKED%", isPressureChecked);

  String isHighlowChecked = "";
  if (WEATHER_HIGHLOW) {
    isHighlowChecked = "checked='checked'";
  }
  form.replace("%HIGHLOW_CHECKED%", isHighlowChecked);

  String is24hourChecked = "";
  if (IS_24HOUR) {
    is24hourChecked = "checked='checked'";
  }
  form.replace("%IS_24HOUR_CHECKED%", is24hourChecked);

  String checked = "";
  if (IS_METRIC) {
    checked = "checked='checked'";
  }
  form.replace("%CHECKED%", checked);
  server.sendContent(form);

  form = FPSTR(CHANGE_FORM2);
  String isPmChecked = "";
  if (IS_PM) {
    isPmChecked = "checked='checked'";
  }
  form.replace("%IS_PM_CHECKED%", isPmChecked);
  String isFlashSecondsChecked = "";
  if (IS_DOTS_BLINKING) {
    isFlashSecondsChecked = "checked='checked'";
  }
  form.replace("%FLASHSECONDS%", isFlashSecondsChecked);
  form.replace("%USER_MESSAGE%", USER_MESSAGE);
  form.replace("%TIME_TO_DISPLAY_ON%", TIME_TO_DISPLAY_ON);
  form.replace("%TIME_TO_DISPLAY_OFF%", TIME_TO_DISPLAY_OFF);
  form.replace("%LED_BRIGHTNESS%", String(LED_BRIGHTNESS));
  String dSpeed = String(SCROLLING_SPEED);
  String scrollOptions =
      "<option value='35'>Slow</option><option "
      "value='25'>Normal</option><option value='15'>Fast</option><option "
      "value='10'>Very Fast</option>";
  scrollOptions.replace(dSpeed + "\"", dSpeed + "\" selected");
  form.replace("%SCROLLING_SPEED%", scrollOptions);
  String minutes = String(minutesBetweenDataRefresh);
  String options =
      "<option>5</option><option>10</option><option>15</option><option>20</"
      "option><option>30</option><option>60</option>";
  options.replace(">" + minutes + "<", " selected>" + minutes + "<");
  form.replace("%OPTIONS%", options);
  form.replace("%REFRESH_DISPLAY%", String(minutesBetweenScrolling));

  server.sendContent(form);  // Send another chunk of the form

  form = FPSTR(CHANGE_FORM3);
  String isUseSecurityChecked = "";
  if (WEB_INTERFACE_AUTH_ENABLED) {
    isUseSecurityChecked = "checked='checked'";
  }
  form.replace("%IS_BASICAUTH_CHECKED%", isUseSecurityChecked);
  form.replace("%USERID%", String(WEB_INTERFACE_USER));
  form.replace("%STATIONPASSWORD%", String(WEB_INTERFACE_PASS));

  server.sendContent(form);  // Send the second chunk of Data

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleDisplay() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  enableDisplay(!displayOn);
  String state = "OFF";
  if (displayOn) {
    state = "ON";
  }
  displayMessage("Display is now " + state);
}

//***********************************************************************
void getUpdatedData()  // client function to send/receive GET request data.
{
  digitalWrite(externalLight, LOW);
  matrix.fillScreen(LOW);  // show black

  if (displayOn) {
    // only pull the weather data if display is on
    if (firstEpoch != 0) {
      centerPrint(hourMinutes(true), true);
    } else {
      centerPrint("...");
    }
    matrix.drawPixel(0, 7, HIGH);
    matrix.drawPixel(0, 6, HIGH);
    matrix.drawPixel(0, 5, HIGH);
    matrix.write();

    weatherClient.updateWeather();
    if (weatherClient.getError() != "") {
      scrollMessage(weatherClient.getError());
    }
  }

  Serial.println("*GetUpdatedData(): Updating Time...");
  // Update the Time
  matrix.drawPixel(0, 4, HIGH);
  matrix.drawPixel(0, 3, HIGH);
  matrix.drawPixel(0, 2, HIGH);
  //Serial.println("*GetUpdatedData(): Matrix Width:" + matrix.width());
  matrix.write();
  TimeDBclient.updateConfig(TIMEDB_API_KEY, weatherClient.getLat(0), weatherClient.getLon(0));
  time_t currentTime = TimeDBclient.getTime();
  if (currentTime > 5000 || firstEpoch == 0) {
    setTime(currentTime);
  } else {
    Serial.println("*GetUpdatedData(): Time update unsuccessful!");
  }
  lastEpoch = now();
  if (firstEpoch == 0) {
    firstEpoch = now();
    Serial.println("*GetUpdatedData(): firstEpoch is: " + String(firstEpoch));
  }

  if (NEWS_ENABLED && displayOn) {
    matrix.drawPixel(0, 2, HIGH);
    matrix.drawPixel(0, 1, HIGH);
    matrix.drawPixel(0, 0, HIGH);
    matrix.write();
    Serial.println("*GetUpdatedData(): Getting News Data for " + NEWS_SOURCE);
    newsClient.updateNews();
  }

  if (displayOn) {
    bitcoinClient.updateBitcoinData(BC_CODE);
    NBUClient.updateNBUStatData(NBU_CODE);
  }

  Serial.println("Firmware Version: " + String(VERSION));
  Serial.println();
  digitalWrite(externalLight, HIGH);
}

void displayMessage(String message) {
  digitalWrite(externalLight, LOW);

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendHeader();
  server.sendContent(message);
  sendFooter();
  server.sendContent("");
  server.client().stop();

  digitalWrite(externalLight, HIGH);
}

void redirectHome() {
  // Send them back to the Root Directory
  server.sendHeader("Location", String("/"), true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");
  server.client().stop();
  delay(100);
}

void sendHeader() {
  server.sendContent(FPSTR(WEB_TOP));
  server.sendContent(FPSTR(WEB_ACTIONS));
  String html = "";

  if (numberOfHorizontalDisplays >= 8) {
    html +=
        "<a class='w3-bar-item w3-button' href='/configurewideclock'>"
        "<i class='far fa-clock'></i> Wide Clock</a>";
  }

  String strDisplayOnOff = displayOn ? "fa-eye-slash'></i> Turn Display OFF" : "fa-eye'></i> Turn Display ON";
  html += "<a class='w3-bar-item w3-button' href='/display'><i class='fas " + strDisplayOnOff + "</a>";

  html +=
      "</nav><header class='w3-top w3-bar w3-theme'>"
      "<button class='w3-bar-item w3-button w3-xxxlarge w3-hover-theme' onclick='openSidebar()'>"
      "<i class='fas fa-bars'></i></button><h2 class='w3-bar-item'>Weather Marquee</h2></header>"
      "<script>"
      "function openSidebar(){document.getElementById('mySidebar').style.display='block'}"
      "function closeSidebar(){document.getElementById('mySidebar').style.display='none'}"
      "closeSidebar();"
      "</script>"
      "<br><div class='w3-container w3-large' style='margin-top:88px'>";
  server.sendContent(html);
}

void sendFooter() {
  int8_t rssi = getWifiQuality();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("%");
  String html = "<br><br><br>";
  html += "</div>";
  html += "<footer class='w3-container w3-bottom w3-theme w3-margin-top'>";
  html +=
      "<i class='far fa-paper-plane'></i> Version: " + String(VERSION) + "<br>";
  html += "<i class='far fa-clock'></i> Next Update: " + getTimeTillUpdate() +
          "<br>";
  html += "<i class='fas fa-rss'></i> Signal Strength: ";
  html += String(rssi) + "%";
  html += "</footer>";
  html += "</body></html>";
  server.sendContent(html);
}

void displayWeatherData() {
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  sendHeader();

  String temperature = weatherClient.getTemp(0);
  String feels_like = weatherClient.getFeelsLike(0);

  String time = TimeDBclient.getDayName() + ", " + TimeDBclient.getMonthName() +
                " " + day() + ", " + hourFormat12() + ":" +
                TimeDBclient.zeroPad(minute()) + " " + TimeDBclient.getAmPm();

  if (TIMEDB_API_KEY == "") {
    html += "<p>Please <a href='/configure'>Configure TimeZone</a> API</p>";
  }

  if (weatherClient.getCity(0) == "") {
    html += "<p>Please <a href='/configure'>Configure Weather</a> API</p>";
    if (weatherClient.getError() != "") {
      html += "<p>Weather Error: <strong>" + weatherClient.getError() +
              "</strong></p>";
    }
  } else {
    html +=
        "<div class='w3-cell-row' style='width:100%; text-align:center'><h2>" +
        weatherClient.getCity(0) + ", " + weatherClient.getCountry(0) +
        "</h2></div><div class='w3-cell-row'>";
    html +=
        "<div class='w3-cell w3-container' style='width:50%; "
        "text-align:right'>";
    html += "<img src='http://openweathermap.org/img/w/" +
            weatherClient.getIcon(0) + ".png' alt='" +
            weatherClient.getDescription(0) + "'><br>";
    html += weatherClient.getHumidity(0) + "% Humidity<br>";
    html += weatherClient.getDirectionText(0) + " / " +
            weatherClient.getWind(0) + " <span class='w3-tiny'>" +
            getSpeedSymbol() + "</span> Wind<br>";
    html += weatherClient.getPressure(0) + " <span class='w3-tiny'>" +
            getPressureSymbol() + "</span> Pressure<br>";
    html += "</div>";
    html += "<div class='w3-cell w3-container' style='width:50%'><p>";
    html += weatherClient.getCondition(0) + " (" +
            weatherClient.getDescription(0) + ")<br>";
    html += "Temperature: " + temperature + " " + getTempSymbolWeb() + "<br>";
    html += "Feels like: " + feels_like + " " + getTempSymbolWeb() + "<br>";
    html += weatherClient.getHigh(0) + "/" + weatherClient.getLow(0) + " " +
            getTempSymbolWeb() + "<br>";
    html += time + "<br>";
    html += "<a href='https://www.google.com/maps/@" + weatherClient.getLat(0) +
            "," + weatherClient.getLon(0) +
            ",10000m/data=!3m1!1e3' target='_BLANK'><i class='fas "
            "fa-map-marker' style='color:red'></i> Map It!</a><br>";
    html += "</p></div></div><hr>";
  }

  server.sendContent(String(html));  // spit out what we got
  html = "";                         // fresh start

  if (BC_CODE != "NONE" && BC_CODE != "") {
    html +=
        "<div class='w3-cell-row'>Bitcoin value: " + bitcoinClient.getRate() +
        " " + bitcoinClient.getCode() + "</div><br><hr>";
  }
  if (NBU_CODE != "NONE" && NBU_CODE != "") {
    html +=
        "<div class='w3-cell-row'>" + NBUClient.getCode() + " rate: " + NBUClient.getRate() + " UAH</div><br><hr>";
  }

  if (NEWS_ENABLED) {
    html += "<div class='w3-cell-row' style='width:100%'><h2>News (" +
            NEWS_SOURCE + ")</h2></div>";
    if (newsClient.getTitle(0) == "") {
      html += "<p>Please <a href='/configurenews'>Configure News</a> API</p>";
    } else {
      for (int inx = 0; inx < 10; inx++) {
        html += "<div class='w3-cell-row'><a href='" + newsClient.getUrl(inx) +
                "' target='_BLANK'>" + newsClient.getTitle(inx) + "</a></div>";
        html += newsClient.getDescription(inx) + "<br/><br/>";
      }
    }
  }
  server.sendContent(String(html));
  sendFooter();
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println("Wifi Manager");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("To setup Wifi Configuration");
  scrollMessage("Please Connect to AP: " +
                String(myWiFiManager->getConfigPortalSSID()));
  centerPrint("wifi");
}

void flashLED(int number, int delayTime) {
  for (int inx = 0; inx < number; inx++) {
    tone(BUZZER_PIN, 440, delayTime);
    delay(delayTime);
    digitalWrite(externalLight, LOW);
    delay(delayTime);
    digitalWrite(externalLight, HIGH);
    delay(delayTime);
  }
  noTone(BUZZER_PIN);
}

String getTempSymbol() {
  String degSymbol = String((char)247);
  return (IS_METRIC ? degSymbol + "C" : degSymbol + "F");
}

String getTempSymbolWeb() {
  return (IS_METRIC ? "°C" : "°F");
}

String getSpeedSymbol() {
  String rtnValue = "m/h";
  if (IS_METRIC) {
    rtnValue = "km/h";
  }
  return rtnValue;
}

String getPressureSymbol() {
  String rtnValue = "";
  if (IS_METRIC) {
    rtnValue = "mb";
  }
  return rtnValue;
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

String getTimeTillUpdate() {
  String rtnValue = "";

  long timeToUpdate = (((minutesBetweenDataRefresh * 60) + lastEpoch) - now());

  int hours = numberOfHours(timeToUpdate);
  int minutes = numberOfMinutes(timeToUpdate);
  int seconds = numberOfSeconds(timeToUpdate);

  rtnValue += String(hours) + ":";
  if (minutes < 10) {
    rtnValue += "0";
  }
  rtnValue += String(minutes) + ":";
  if (seconds < 10) {
    rtnValue += "0";
  }
  rtnValue += String(seconds);

  return rtnValue;
}

int getMinutesFromLastRefresh() {
  int minutes = (now() - lastEpoch) / 60;
  return minutes;
}

int getMinutesFromLastDisplay() {
  int minutes = (now() - displayOffEpoch) / 60;
  return minutes;
}

void enableDisplay(boolean enable) {
  displayOn = enable;
  if (enable) {
    if (getMinutesFromLastDisplay() >= minutesBetweenDataRefresh) {
      // The display has been off longer than the minutes between refresh --
      // need to get fresh data
      lastEpoch = 0;        // this should force a data pull of the weather
      displayOffEpoch = 0;  // reset
    }
    matrix.shutdown(false);
    matrix.fillScreen(LOW);  // show black
    Serial.println("Display was turned ON: " + now());
  } else {
    matrix.shutdown(true);
    Serial.println("Display was turned OFF: " + now());
    displayOffEpoch = lastEpoch;
  }
}

// Toggle on and off the display if user defined times
void checkDisplay() {
  if (TIME_TO_DISPLAY_ON == "" || TIME_TO_DISPLAY_OFF == "") {
    return;  // nothing to do
  }
  String currentTime =
      TimeDBclient.zeroPad(hour()) + ":" + TimeDBclient.zeroPad(minute());

  if (currentTime == TIME_TO_DISPLAY_ON && !displayOn) {
    Serial.println("Time to turn display on: " + currentTime);
    flashLED(1, 500);
    enableDisplay(true);
  }

  if (currentTime == TIME_TO_DISPLAY_OFF && displayOn) {
    Serial.println("Time to turn display off: " + currentTime);
    flashLED(2, 500);
    enableDisplay(false);
  }
}

bool writeConfigJson() {
  const size_t capacity = 2048;
  DynamicJsonDocument doc(capacity);

  doc["TIMEDB_API_KEY"] = TIMEDB_API_KEY;
  doc["USER_MESSAGE"] = USER_MESSAGE;
  doc["TIME_TO_DISPLAY_ON"] = TIME_TO_DISPLAY_ON;
  doc["TIME_TO_DISPLAY_OFF"] = TIME_TO_DISPLAY_OFF;
  doc["ledIntensity"] = LED_BRIGHTNESS;
  doc["scrollSpeed"] = SCROLLING_SPEED;

  doc["NEWS"]["ENABLED"] = NEWS_ENABLED;
  doc["NEWS"]["API_KEY"] = NEWS_API_KEY;
  doc["NEWS"]["SOURCE"] = NEWS_SOURCE;

  doc["IS_DOTS_BLINKING"] = IS_DOTS_BLINKING;
  doc["IS_24HOUR"] = IS_24HOUR;
  doc["IS_PM"] = IS_PM;
  doc["wideclockformat"] = Wide_Clock_Style;
  doc["isMetric"] = IS_METRIC;
  doc["refreshRate"] = minutesBetweenDataRefresh;
  doc["minutesBetweenScrolling"] = minutesBetweenScrolling;

  doc["WEB_INTERFACE"]["AUTH_ENABLED"] = WEB_INTERFACE_AUTH_ENABLED;
  doc["WEB_INTERFACE"]["USER"] = WEB_INTERFACE_USER;
  doc["WEB_INTERFACE"]["PASS"] = WEB_INTERFACE_PASS;

  doc["BC_CODE"] = BC_CODE;
  doc["NBU_CODE"] = NBU_CODE;

  doc["WEATHER"]["API_KEY"] = WEATHER_API_KEY;
  doc["WEATHER"]["CITY_ID"] = CityIDs[0];
  doc["WEATHER"]["CITY_NAME"] = WEATHER_CITY;
  doc["WEATHER"]["CONDITION"] = WEATHER_CONDITION;
  doc["WEATHER"]["HUMIDITY"] = WEATHER_HUMIDITY;
  doc["WEATHER"]["WIND"] = WEATHER_WIND;
  doc["WEATHER"]["PRESSURE"] = WEATHER_PRESSURE;
  doc["WEATHER"]["HIGHLOW"] = WEATHER_HIGHLOW;
  doc["WEATHER"]["FEELSLIKE"] = WEATHER_FEELSLIKE;
  doc["WEATHER"]["DATE"] = WEATHER_DATE;

  File configFile = LittleFS.open(CONFIG_JSON, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJsonPretty(doc, configFile);
  configFile.close();
  // serializeJsonPretty(doc, Serial);
  // Serial.println();
  newsClient.updateNewsClient(NEWS_API_KEY, NEWS_SOURCE);
  weatherClient.updateWeatherApiKey(WEATHER_API_KEY);
  weatherClient.setMetric(IS_METRIC);
  weatherClient.updateCityIdList(CityIDs, 1);
  return true;
}

bool readConfigJson() {
  if (LittleFS.exists(CONFIG_JSON) == false) {
    Serial.println("Settings File does not yet exists.");
    writeConfigJson();
  }
  File f = LittleFS.open(CONFIG_JSON, "r");

  const size_t capacity = 2048;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, f);
  if (error)
    Serial.println(F("Failed to read JSON file"));

  // serializeJsonPretty(doc, Serial);
  // Serial.println();
  f.close();

  TIMEDB_API_KEY = (const char *)doc["TIMEDB_API_KEY"];
  USER_MESSAGE = (const char *)doc["USER_MESSAGE"];
  NEWS_SOURCE = (const char *)doc["NEWS_SOURCE"];
  TIME_TO_DISPLAY_ON = (const char *)doc["TIME_TO_DISPLAY_ON"];
  TIME_TO_DISPLAY_OFF = (const char *)doc["TIME_TO_DISPLAY_OFF"];
  LED_BRIGHTNESS = doc["ledIntensity"];
  SCROLLING_SPEED = doc["scrollSpeed"];

  NEWS_ENABLED = doc["NEWS"]["ENABLED"];
  NEWS_API_KEY = (const char *)doc["NEWS"]["API_KEY"];
  NEWS_SOURCE = (const char *)doc["NEWS"]["SOURCE"];

  IS_DOTS_BLINKING = doc["IS_DOTS_BLINKING"];
  IS_24HOUR = doc["IS_24HOUR"];
  IS_PM = doc["IS_PM"];
  Wide_Clock_Style = (const char *)doc["wideclockformat"];
  IS_METRIC = doc["isMetric"];
  minutesBetweenDataRefresh = doc["refreshRate"];
  minutesBetweenScrolling = doc["minutesBetweenScrolling"];

  WEB_INTERFACE_AUTH_ENABLED = doc["WEB_INTERFACE"]["AUTH_ENABLED"];
  WEB_INTERFACE_USER = (const char *)doc["WEB_INTERFACE"]["USER"];
  WEB_INTERFACE_PASS = (const char *)doc["WEB_INTERFACE"]["PASS"];

  BC_CODE = (const char *)doc["BC_CODE"];
  NBU_CODE = (const char *)doc["NBU_CODE"];

  WEATHER_API_KEY = (const char *)doc["WEATHER"]["API_KEY"];
  CityIDs[0] = doc["WEATHER"]["CITY_ID"];
  WEATHER_CITY = doc["WEATHER"]["CITY_NAME"];
  WEATHER_CONDITION = doc["WEATHER"]["CONDITION"];
  WEATHER_HUMIDITY = doc["WEATHER"]["HUMIDITY"];
  WEATHER_WIND = doc["WEATHER"]["WIND"];
  WEATHER_PRESSURE = doc["WEATHER"]["PRESSURE"];
  WEATHER_HIGHLOW = doc["WEATHER"]["HIGHLOW"];
  WEATHER_FEELSLIKE = doc["WEATHER"]["FEELSLIKE"];
  WEATHER_DATE = doc["WEATHER"]["DATE"];

  matrix.setIntensity(LED_BRIGHTNESS);
  newsClient.updateNewsClient(NEWS_API_KEY, NEWS_SOURCE);
  weatherClient.updateWeatherApiKey(WEATHER_API_KEY);
  weatherClient.setMetric(IS_METRIC);
  weatherClient.updateCityIdList(CityIDs, 1);
  return true;
}

void scrollMessage(String msg) {
  msg += " ";  // add a space at the end
  for (int i = 0; i < width * msg.length() + matrix.width() - 1 - spacer; i++) {
    if (WEBSERVER_ENABLED) {
      server.handleClient();
    }
    if (ENABLE_OTA) {
      ArduinoOTA.handle();
    }
    if (refresh == 1)
      i = 0;
    refresh = 0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2;  // center the text vertically

    while (x + width - spacer >= 0 && letter >= 0) {
      if (letter < msg.length()) {
        matrix.drawChar(x, y, msg[letter], HIGH, LOW, 1);
      }

      letter--;
      x -= width;
    }

    matrix.write();  // Send bitmap to display
    delay(SCROLLING_SPEED);
  }
  matrix.setCursor(0, 0);
}

void centerPrint(String msg, boolean extraStuff) {
  int x = (matrix.width() - (msg.length() * width)) / 2;

  // Print the static portions of the display before the main Message
  if (extraStuff) {
    if (!IS_24HOUR && IS_PM && isPM()) {
      matrix.drawPixel(matrix.width() - 1, 6, HIGH);
    }
  }

  matrix.setCursor(x, 0);
  matrix.print(msg);
  matrix.write();
}

String decodeHtmlString(String msg) {
  String decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client
  // browser
  decodedMsg.replace("+", " ");
  decodedMsg.replace("%21", "!");
  decodedMsg.replace("%22", "");
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");
  decodedMsg.replace("%2C", ",");
  decodedMsg.replace("%2F", "/");
  decodedMsg.replace("%3A", ":");
  decodedMsg.replace("%3B", ";");
  decodedMsg.replace("%3C", "<");
  decodedMsg.replace("%3D", "=");
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");
  decodedMsg.replace("%40", "@");
  decodedMsg.toUpperCase();
  decodedMsg.trim();
  return decodedMsg;
}
