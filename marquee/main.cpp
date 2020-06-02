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
  //LittleFS.remove(CONFIG);
  delay(10);

  // Initialize digital pin for LED
  pinMode(externalLight, OUTPUT);

  //New Line to clear from start garbage
  Serial.println();

  readCityIds();

  Serial.println("*setup(): Number of LED Displays: " + String(numberOfHorizontalDisplays));
  // initialize dispaly
  matrix.setIntensity(0); // Use a value between 0 and 15 for brightness
  //matrix.setFont(&TomThumb);
  //matrix.cp437(true);

  int maxPos = numberOfHorizontalDisplays * numberOfVerticalDisplays;
  for (int i = 0; i < maxPos; i++) {
    matrix.setRotation(i, ledRotation);
    matrix.setPosition(i, maxPos - i - 1, 0);
  }

  Serial.println("*setup(): Matrix created");
  matrix.fillScreen(LOW); // show black
  centerPrint("hello");

  tone(BUZZER_PIN, 415, 500);
  delay(500 * 1.3);
  tone(BUZZER_PIN, 466, 500);
  delay(500 * 1.3);
  tone(BUZZER_PIN, 370, 1000);
  delay(1000 * 1.3);
  noTone(BUZZER_PIN);

  for (int inx = 0; inx <= 15; inx++) {
    matrix.setIntensity(inx);
    delay(100);
  }
  for (int inx = 15; inx >= 0; inx--) {
    matrix.setIntensity(inx);
    delay(60);
  }
  delay(1000);
  matrix.setIntensity(displayIntensity);
  //noTone(BUZZER_PIN);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //Custom Station (client) Static IP Configuration - Set custom IP for your Network (IP, Gateway, Subnet mask)
  //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,99), IPAddress(192,168,0,1), IPAddress(255,255,255,0));

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {// new addition
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
    ArduinoOTA.onStart([]() {
      Serial.println("*setup() OTA: Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("*setup() OTA: Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("*setup() OTA: Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
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
    server.on("/locations", handleLocations);
    server.on("/savebitcoin", handleSaveBitcoin);
    server.on("/savewideclock", handleSaveWideClock);
    server.on("/savenews", handleSaveNews);
    server.on("/saveoctoprint", handleSaveOctoprint);
    server.on("/savepihole", handleSavePihole);
    server.on("/systemreset", handleSystemReset);
    server.on("/forgetwifi", handleForgetWifi);
    server.on("/configure", handleConfigure);
    server.on("/configurebitcoin", handleBitcoinConfigure);
    server.on("/configurewideclock", handleWideClockConfigure);
    server.on("/configurenews", handleNewsConfigure);
    server.on("/configureoctoprint", handleOctoprintConfigure);
    server.on("/configurepihole", handlePiholeConfigure);
    server.on("/display", handleDisplay);
    server.onNotFound(redirectHome);
    serverUpdater.setup(&server, "/update", www_username, www_password);
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
  //Get some Weather Data to serve
  if ((getMinutesFromLastRefresh() >= minutesBetweenDataRefresh) || lastEpoch == 0) {
    getUpdatedData();
  }
  checkDisplay(); // this will see if we need to turn it on or off for night mode.

  if (lastMinute != TimeDB.zeroPad(minute())) {
    lastMinute = TimeDB.zeroPad(minute());

    if (weatherClient.getError() != "") {
      scrollMessage(weatherClient.getError());
      return;
    }

    if (displayOn) {
      matrix.shutdown(false);
    }
    matrix.fillScreen(LOW); // show black
    if (OCTOPRINT_ENABLED) {
      if (displayOn && ((printerClient.isOperational() || printerClient.isPrinting()) || printerCount == 0)) {
        // This should only get called if the printer is actually running or if it has been 2 minutes since last check
        printerClient.getPrinterJobResults();
      }
      printerCount += 1;
      if (printerCount > 2) {
        printerCount = 0;
      }
    }

    displayRefreshCount --;
    // Check to see if we need to Scroll some Data
    if (displayRefreshCount <= 0) {
      displayRefreshCount = minutesBetweenScrolling;
      String temperature = weatherClient.getTempRounded(0);
      String description = weatherClient.getDescription(0);
      description.toUpperCase();
      String msg;
      msg += scrollSpacer;

      if (SHOW_DATE) {
        msg += TimeDB.getDayName() + ", ";
        msg += TimeDB.getMonthName() + " " + day();
        msg += scrollSpacer;
      }
      if (SHOW_CITY) {
        msg += weatherClient.getCity(0);
        msg += scrollSpacer;
      }
      msg += "Temp: " + temperature + getTempSymbol();
      msg += scrollSpacer;

      if (SHOW_FEELSLIKE) {
        String feels_like = weatherClient.getFeelsLikeRounded(0);
        msg += "Feels like: " + feels_like + getTempSymbol();
        msg += scrollSpacer;
      } 
        

      //show high/low temperature
      if (SHOW_HIGHLOW) {
        msg += "Hi:" + weatherClient.getHigh(0) + getTempSymbol() + " / Lo:" + weatherClient.getLow(0) + " " + getTempSymbol();
        msg += scrollSpacer;
      }
      
      if (SHOW_CONDITION) {
        msg += description + ". ";
      }
      if (SHOW_HUMIDITY) {
        msg += "Humid: " + weatherClient.getHumidityRounded(0) + "%  ";
      }
      if (SHOW_WIND) {
        msg += "Wind: " + weatherClient.getDirectionText(0) + " @ " + weatherClient.getWindRounded(0) + " " + getSpeedSymbol();
        msg += scrollSpacer;
      }
      //line to show barometric pressure
      if (SHOW_PRESSURE) {
        msg += "Pressure:" + weatherClient.getPressure(0) + getPressureSymbol();
        msg += scrollSpacer;
      }

      if (!marqueeMessage.isEmpty()) {
        msg += marqueeMessage;
        msg += scrollSpacer;
      }
      
      if (NEWS_ENABLED) {
        msg += NEWS_SOURCE + ": " + newsClient.getTitle(newsIndex);
        msg += scrollSpacer;
        newsIndex += 1;
        if (newsIndex > 9) {
          newsIndex = 0;
        }
      }
      if (OCTOPRINT_ENABLED && printerClient.isPrinting()) {
        msg += printerClient.getFileName() + " ";
        msg += "(" + printerClient.getProgressCompletion() + "%)";
        msg += scrollSpacer;
      }
      if (BitcoinCurrencyCode != "NONE" && BitcoinCurrencyCode != "") {
        msg += "Bitcoin: " + bitcoinClient.getRate() + " " + bitcoinClient.getCode();
        msg += scrollSpacer;;
      }
      if (USE_PIHOLE) {
        piholeClient.getPiHoleData(PiHoleServer, PiHolePort);
        piholeClient.getGraphData(PiHoleServer, PiHolePort);
        if (piholeClient.getPiHoleStatus() != "") {
          msg += "Pi-hole (" + piholeClient.getPiHoleStatus() + "): " + piholeClient.getAdsPercentageToday() + "%";
          msg += scrollSpacer; 
        }
      }

      scrollMessage(msg);
      Serial.println(msg);
      drawPiholeGraph();
    }
  }

  String currentTime = hourMinutes(false);

  if (numberOfHorizontalDisplays >= 8) {
    if (Wide_Clock_Style == "1") {
      // On Wide Display -- show the current temperature as well
      String currentTemp = weatherClient.getTempRounded(0);
      String timeSpacer = "";
      if (currentTemp.length() <= 2) timeSpacer = " ";
      currentTime += timeSpacer + currentTemp + getTempSymbol();
    }
    if (Wide_Clock_Style == "2") {
      currentTime += secondsIndicator(false) + TimeDB.zeroPad(second());
      matrix.fillScreen(LOW); // show black
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
    return hour() + secondsIndicator(isRefresh) + TimeDB.zeroPad(minute());
  } else {
    return hourFormat12() + secondsIndicator(isRefresh) + TimeDB.zeroPad(minute());
  }
}

String secondsIndicator(boolean isRefresh) {
  String rtnValue = ":";
  if (isRefresh == false && (flashOnSeconds && (second() % 2) == 0)) {
    rtnValue = " ";
  }
  return rtnValue;
}

boolean athentication() {
  if (IS_BASIC_AUTH) {
    return server.authenticate(www_username, www_password);
  }
  return true; // Authentication not required
}

void handlePull() {
  getUpdatedData(); // this will force a data pull for new weather
  displayWeatherData();
}

void handleSaveBitcoin() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  BitcoinCurrencyCode = server.arg("bitcoincurrency");
  writeConfig();
  bitcoinClient.updateBitcoinData(BitcoinCurrencyCode);  // does nothing if BitCoinCurrencyCode is "NONE" or empty
  redirectHome();
}

void handleSaveWideClock() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  if (numberOfHorizontalDisplays >= 8) {
    Wide_Clock_Style = server.arg("wideclockformat");
    writeConfig();
    matrix.fillScreen(LOW); // show black
  }
  redirectHome();
}

void handleSaveNews() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  NEWS_ENABLED = server.hasArg("displaynews");
  NEWS_API_KEY = server.arg("newsApiKey");
  NEWS_SOURCE = server.arg("newssource");
  matrix.fillScreen(LOW); // show black
  writeConfig();
  newsClient.updateNews();
  redirectHome();
}

void handleSaveOctoprint() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  OCTOPRINT_ENABLED = server.hasArg("displayoctoprint");
  OCTOPRINT_PROGRESS = server.hasArg("octoprintprogress");
  OctoPrintApiKey = server.arg("octoPrintApiKey");
  OctoPrintServer = server.arg("octoPrintAddress");
  OctoPrintPort = server.arg("octoPrintPort").toInt();
  OctoAuthUser = server.arg("octoUser");
  OctoAuthPass = server.arg("octoPass");
  matrix.fillScreen(LOW); // show black
  writeConfig();
  if (OCTOPRINT_ENABLED) {
    printerClient.getPrinterJobResults();
  }
  redirectHome();
}

void handleSavePihole() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  USE_PIHOLE = server.hasArg("displaypihole");
  PiHoleServer = server.arg("piholeAddress");
  PiHolePort = server.arg("piholePort").toInt();
  writeConfig();
  if (USE_PIHOLE) {
    piholeClient.getPiHoleData(PiHoleServer, PiHolePort);
    piholeClient.getGraphData(PiHoleServer, PiHolePort);
  }
  redirectHome();
}

void handleLocations() {
  if (!athentication()) {
    return server.requestAuthentication();
  }
  TIMEDBKEY = server.arg("TimeZoneDB");
  APIKEY = server.arg("openWeatherMapApiKey");
  CityIDs[0] = server.arg("city1").toInt();
  flashOnSeconds = server.hasArg("flashseconds");
  IS_24HOUR = server.hasArg("is24hour");
  IS_PM = server.hasArg("isPM");
  SHOW_FEELSLIKE = server.hasArg("showfeelslike");
  SHOW_DATE = server.hasArg("showdate");
  SHOW_CITY = server.hasArg("showcity");
  SHOW_CONDITION = server.hasArg("showcondition");
  SHOW_HUMIDITY = server.hasArg("showhumidity");
  SHOW_WIND = server.hasArg("showwind");
  SHOW_PRESSURE = server.hasArg("showpressure");
  SHOW_HIGHLOW = server.hasArg("showhighlow");
  IS_METRIC = server.hasArg("metric");
  marqueeMessage = decodeHtmlString(server.arg("marqueeMsg"));
  timeDisplayTurnsOn = decodeHtmlString(server.arg("startTime"));
  timeDisplayTurnsOff = decodeHtmlString(server.arg("endTime"));
  displayIntensity = server.arg("ledintensity").toInt();
  minutesBetweenDataRefresh = server.arg("refresh").toInt();
  minutesBetweenScrolling = server.arg("refreshDisplay").toInt();
  displayScrollSpeed = server.arg("scrollspeed").toInt();
  IS_BASIC_AUTH = server.hasArg("isBasicAuth");
  String temp = server.arg("userid");
  temp.toCharArray(www_username, sizeof(temp));
  temp = server.arg("stationpassword");
  temp.toCharArray(www_password, sizeof(temp));
  weatherClient.setMetric(IS_METRIC);
  matrix.fillScreen(LOW); // show black
  writeConfig();
  getUpdatedData(); // this will force a data pull for new weather
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
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
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
  bitcoinOptions.replace(BitcoinCurrencyCode + "'", BitcoinCurrencyCode + "' selected");
  form.replace("%BITCOINOPTIONS%", bitcoinOptions);
  server.sendContent(form); //Send another Chunk of form

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
    String clockOptions = "<option value='1'>HH:MM Temperature</option><option value='2'>HH:MM:SS</option><option value='3'>HH:MM</option>";
    clockOptions.replace(Wide_Clock_Style + "'", Wide_Clock_Style + "' selected");
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
  String isNewsDisplayedChecked = "";
  if (NEWS_ENABLED) {
    isNewsDisplayedChecked = "checked='checked'";
  }
  form.replace("%NEWSCHECKED%", isNewsDisplayedChecked);
  form.replace("%NEWSKEY%", NEWS_API_KEY);
  form.replace("%NEWSSOURCE%", NEWS_SOURCE);
  server.sendContent(form); //Send news form

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleOctoprintConfigure() {
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

  String form = FPSTR(OCTO_FORM);
  String isOctoPrintDisplayedChecked = "";
  if (OCTOPRINT_ENABLED) {
    isOctoPrintDisplayedChecked = "checked='checked'";
  }
  form.replace("%OCTOCHECKED%", isOctoPrintDisplayedChecked);
  String isOctoPrintProgressChecked = "";
  if (OCTOPRINT_PROGRESS) {
    isOctoPrintProgressChecked = "checked='checked'";
  }
  form.replace("%OCTOPROGRESSCHECKED%", isOctoPrintProgressChecked);
  form.replace("%OCTOKEY%", OctoPrintApiKey);
  form.replace("%OCTOADDRESS%", OctoPrintServer);
  form.replace("%OCTOPORT%", String(OctoPrintPort));
  form.replace("%OCTOUSER%", OctoAuthUser);
  form.replace("%OCTOPASS%", OctoAuthPass);
  server.sendContent(form);

  sendFooter();

  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handlePiholeConfigure() {
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

  server.sendContent(FPSTR(PIHOLE_TEST));

  String form = FPSTR(PIHOLE_FORM);
  String isPiholeDisplayedChecked = "";
  if (USE_PIHOLE) {
    isPiholeDisplayedChecked = "checked='checked'";
  }
  form.replace("%PIHOLECHECKED%", isPiholeDisplayedChecked);
  form.replace("%PIHOLEADDRESS%", PiHoleServer);
  form.replace("%PIHOLEPORT%", String(PiHolePort));

  server.sendContent(form);
  form = "";
          
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
  form.replace("%TIMEDBKEY%", TIMEDBKEY);
  form.replace("%WEATHERKEY%", APIKEY);

  String cityName = "";
  if (weatherClient.getCity(0) != "") {
    cityName = weatherClient.getCity(0) + ", " + weatherClient.getCountry(0);
  }
  form.replace("%CITYNAME1%", cityName);
  form.replace("%CITY1%", String(CityIDs[0]));
  
  String isFeelsLikeChecked = "";
  if (SHOW_FEELSLIKE) {
    isFeelsLikeChecked = "checked='checked'";
  }
  form.replace("%FEELSLIKE_CHECKED%", isFeelsLikeChecked);
  
  String isDateChecked = "";
  if (SHOW_DATE) {
    isDateChecked = "checked='checked'";
  }
  form.replace("%DATE_CHECKED%", isDateChecked);
  
  String isCityChecked = "";
  if (SHOW_CITY) {
    isCityChecked = "checked='checked'";
  }
  form.replace("%CITY_CHECKED%", isCityChecked);
  
  String isConditionChecked = "";
  if (SHOW_CONDITION) {
    isConditionChecked = "checked='checked'";
  }
  form.replace("%CONDITION_CHECKED%", isConditionChecked);
  
  String isHumidityChecked = "";
  if (SHOW_HUMIDITY) {
    isHumidityChecked = "checked='checked'";
  }
  form.replace("%HUMIDITY_CHECKED%", isHumidityChecked);
  
  String isWindChecked = "";
  if (SHOW_WIND) {
    isWindChecked = "checked='checked'";
  }
  form.replace("%WIND_CHECKED%", isWindChecked);
  
  String isPressureChecked = "";
  if (SHOW_PRESSURE) {
    isPressureChecked = "checked='checked'";
  }
  form.replace("%PRESSURE_CHECKED%", isPressureChecked);

  String isHighlowChecked = "";
  if (SHOW_HIGHLOW) {
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
  if (flashOnSeconds) {
    isFlashSecondsChecked = "checked='checked'";
  }
  form.replace("%FLASHSECONDS%", isFlashSecondsChecked);
  form.replace("%MSG%", marqueeMessage);
  form.replace("%STARTTIME%", timeDisplayTurnsOn);
  form.replace("%ENDTIME%", timeDisplayTurnsOff);
  form.replace("%INTENSITYOPTIONS%", String(displayIntensity));
  String dSpeed = String(displayScrollSpeed);
  String scrollOptions = "<option value='35'>Slow</option><option value='25'>Normal</option><option value='15'>Fast</option><option value='10'>Very Fast</option>";
  scrollOptions.replace(dSpeed + "'", dSpeed + "' selected" );
  form.replace("%SCROLLOPTIONS%", scrollOptions);
  String minutes = String(minutesBetweenDataRefresh);
  String options = "<option>5</option><option>10</option><option>15</option><option>20</option><option>30</option><option>60</option>";
  options.replace(">" + minutes + "<", " selected>" + minutes + "<");
  form.replace("%OPTIONS%", options);
  form.replace("%REFRESH_DISPLAY%", String(minutesBetweenScrolling));

  server.sendContent(form); //Send another chunk of the form

  form = FPSTR(CHANGE_FORM3);
  String isUseSecurityChecked = "";
  if (IS_BASIC_AUTH) {
    isUseSecurityChecked = "checked='checked'";
  }
  form.replace("%IS_BASICAUTH_CHECKED%", isUseSecurityChecked);
  form.replace("%USERID%", String(www_username));
  form.replace("%STATIONPASSWORD%", String(www_password));

  server.sendContent(form); // Send the second chunk of Data

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
void getUpdatedData() //client function to send/receive GET request data.
{
  digitalWrite(externalLight, LOW);
  matrix.fillScreen(LOW); // show black

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
  //Update the Time
  matrix.drawPixel(0, 4, HIGH);
  matrix.drawPixel(0, 3, HIGH);
  matrix.drawPixel(0, 2, HIGH);
  Serial.println("*GetUpdatedData(): Matrix Width:" + matrix.width());
  matrix.write();
  TimeDB.updateConfig(TIMEDBKEY, weatherClient.getLat(0), weatherClient.getLon(0));
  time_t currentTime = TimeDB.getTime();
  if(currentTime > 5000 || firstEpoch == 0) {
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
    bitcoinClient.updateBitcoinData(BitcoinCurrencyCode);  // does nothing if BitCoinCurrencyCode is "NONE" or empty
  }

  Serial.println("*GetUpdatedData(): Version: " + String(VERSION));
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
  delay(1000);
}

void sendHeader() {
  String html = "<!DOCTYPE HTML>";
  html += "<html><head><title>Marquee Scroller</title><link rel='icon' href='data:;base64,='>";
  html += "<meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/lib/w3-theme-blue-grey.css'>";
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.8.1/css/all.min.css'>";
  html += "</head><body>";
  server.sendContent(html);
  html = "<nav class='w3-sidebar w3-bar-block w3-card' style='margin-top:88px' id='mySidebar'>";
  html += "<div class='w3-container w3-theme-d2'>";
  html += "<span onclick='closeSidebar()' class='w3-button w3-display-topright w3-large'><i class='fas fa-times'></i></span>";
  html += "<div class='w3-left'><img src='http://openweathermap.org/img/w/" + weatherClient.getIcon(0) + ".png' alt='" + weatherClient.getDescription(0) + "'></div>";
  html += "<div class='w3-padding'>Menu</div></div>";
  server.sendContent(html);

  server.sendContent(FPSTR(WEB_ACTIONS1));
  Serial.println("Displays: " + String(numberOfHorizontalDisplays));
  if (numberOfHorizontalDisplays >= 8) {
    server.sendContent("<a class='w3-bar-item w3-button' href='/configurewideclock'><i class='far fa-clock'></i> Wide Clock</a>");
  }
  server.sendContent(FPSTR(WEB_ACTIONS2));
  if (displayOn) {
    server.sendContent("<i class='fas fa-eye-slash'></i> Turn Display OFF");
  } else {
    server.sendContent("<i class='fas fa-eye'></i> Turn Display ON");
  }
  server.sendContent(FPSTR(WEB_ACTION3));

  html = "</nav>";
  html += "<header class='w3-top w3-bar w3-theme'><button class='w3-bar-item w3-button w3-xxxlarge w3-hover-theme' onclick='openSidebar()'><i class='fas fa-bars'></i></button><h2 class='w3-bar-item'>Weather Marquee</h2></header>";
  html += "<script>";
  html += "function openSidebar(){document.getElementById('mySidebar').style.display='block'}function closeSidebar(){document.getElementById('mySidebar').style.display='none'}closeSidebar();";
  html += "</script>";
  html += "<br><div class='w3-container w3-large' style='margin-top:88px'>";
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
  html += "<i class='far fa-paper-plane'></i> Version: " + String(VERSION) + "<br>";
  html += "<i class='far fa-clock'></i> Next Update: " + getTimeTillUpdate() + "<br>";
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

  // if ((temperature.indexOf(".") != -1) && (temperature.length() >= (temperature.indexOf(".") + 2))) {
  //   temperature.remove(temperature.indexOf(".") + 2);
  // }

  Serial.println("debug-temperature: " + temperature); // debug
  Serial.println("debug-feels_like: " + feels_like); // debug

  String time = TimeDB.getDayName() + ", " + TimeDB.getMonthName() + " " + day() + ", " + hourFormat12() + ":" + TimeDB.zeroPad(minute()) + " " + TimeDB.getAmPm();

  Serial.println(weatherClient.getCity(0));
  Serial.println(weatherClient.getCondition(0));
  Serial.println(weatherClient.getDescription(0));
  Serial.println(temperature);
  Serial.println(time);

  if (TIMEDBKEY == "") {
    html += "<p>Please <a href='/configure'>Configure TimeZoneDB</a> with API key.</p>";
  }

  if (weatherClient.getCity(0) == "") {
    html += "<p>Please <a href='/configure'>Configure Weather</a> API</p>";
    if (weatherClient.getError() != "") {
      html += "<p>Weather Error: <strong>" + weatherClient.getError() + "</strong></p>";
    }
  } else {
    html += "<div class='w3-cell-row' style='width:100%; text-align:center'><h2>" + weatherClient.getCity(0) + ", " + weatherClient.getCountry(0) + "</h2></div><div class='w3-cell-row'>";
    html += "<div class='w3-cell w3-container' style='width:50%; text-align:right'>";
    html += "<img src='http://openweathermap.org/img/w/" + weatherClient.getIcon(0) + ".png' alt='" + weatherClient.getDescription(0) + "'><br>";
    html += weatherClient.getHumidity(0) + "% Humidity<br>";
    html += weatherClient.getDirectionText(0) + " / " + weatherClient.getWind(0) + " <span class='w3-tiny'>" + getSpeedSymbol() + "</span> Wind<br>";
    html += weatherClient.getPressure(0) + " Pressure<br>";
    html += "</div>";
    html += "<div class='w3-cell w3-container' style='width:50%'><p>";
    html += weatherClient.getCondition(0) + " (" + weatherClient.getDescription(0) + ")<br>";
    html += "Temperature: " + temperature + " " + getTempSymbolWeb() + "<br>";
    html += "Feels like: " + feels_like + " " + getTempSymbolWeb() + "<br>";
    html += weatherClient.getHigh(0) + "/" + weatherClient.getLow(0) + " " + getTempSymbolWeb() + "<br>";
    html += time + "<br>";
    html += "<a href='https://www.google.com/maps/@" + weatherClient.getLat(0) + "," + weatherClient.getLon(0) + ",10000m/data=!3m1!1e3' target='_BLANK'><i class='fas fa-map-marker' style='color:red'></i> Map It!</a><br>";
    html += "</p></div></div><hr>";
  }


  server.sendContent(String(html)); // spit out what we got
  html = ""; // fresh start


  if (OCTOPRINT_ENABLED) {
    html = "<div class='w3-cell-row'>OctoPrint Status: ";
    if (printerClient.isPrinting()) {
      html += printerClient.getState() + " " + printerClient.getFileName() + " (" + printerClient.getProgressCompletion() + "%)";
    } else if (printerClient.isOperational()) {
      html += printerClient.getState();
    } else if (printerClient.getError() != "") {
      html += printerClient.getError();
    } else {
      html += "Not Connected";
    }
    html += "</div><br><hr>";
    server.sendContent(String(html));
    html = "";
  }

  if (BitcoinCurrencyCode != "NONE" && BitcoinCurrencyCode != "") {
    html = "<div class='w3-cell-row'>Bitcoin value: " + bitcoinClient.getRate() + " " + bitcoinClient.getCode() + "</div><br><hr>";
    server.sendContent(String(html));
    html = "";
  }

  if (USE_PIHOLE) {
    if (piholeClient.getError() == "") {
      html = "<div class='w3-cell-row'><b>Pi-hole</b><br>"
             "Total Queries (" + piholeClient.getUniqueClients() + " clients): <b>" + piholeClient.getDnsQueriesToday() + "</b><br>"
             "Queries Blocked: <b>" + piholeClient.getAdsBlockedToday() + "</b><br>"
             "Percent Blocked: <b>" + piholeClient.getAdsPercentageToday() + "%</b><br>"
             "Domains on Blocklist: <b>" + piholeClient.getDomainsBeingBlocked() + "</b><br>"
             "Status: <b>" + piholeClient.getPiHoleStatus() + "</b><br>"
             "</div><br><hr>";
    } else {
      html = "<div class='w3-cell-row'>Pi-hole Error";
      html += "Please <a href='/configurepihole' title='Configure'>Configure</a> for Pi-hole <a href='/configurepihole' title='Configure'><i class='fas fa-cog'></i></a><br>";
      html += "Status: Error Getting Data<br>";
      html += "Reason: " + piholeClient.getError() + "<br></div><br><hr>";
    }
    server.sendContent(html);
    html = "";
  }

  if (NEWS_ENABLED) {
    html = "<div class='w3-cell-row' style='width:100%'><h2>News (" + NEWS_SOURCE + ")</h2></div>";
    if (newsClient.getTitle(0) == "") {
      html += "<p>Please <a href='/configurenews'>Configure News</a> API</p>";
      server.sendContent(html);
      html = "";
    } else {
      for (int inx = 0; inx < 10; inx++) {
        html = "<div class='w3-cell-row'><a href='" + newsClient.getUrl(inx) + "' target='_BLANK'>" + newsClient.getTitle(inx) + "</a></div>";
        html += newsClient.getDescription(inx) + "<br/><br/>";
        server.sendContent(html);
        html = "";
      }
    }
  }

  sendFooter();
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println("Wifi Manager");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("To setup Wifi Configuration");
  scrollMessage("Please Connect to AP: " + String(myWiFiManager->getConfigPortalSSID()));
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
  return (IS_METRIC ? degSymbol+"C" : degSymbol+"F");
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

String getPressureSymbol()
{
  String rtnValue = "";
  if (IS_METRIC)
  {
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
      // The display has been off longer than the minutes between refresh -- need to get fresh data
      lastEpoch = 0; // this should force a data pull of the weather
      displayOffEpoch = 0;  // reset
    }
    matrix.shutdown(false);
    matrix.fillScreen(LOW); // show black
    Serial.println("Display was turned ON: " + now());
  } else {
    matrix.shutdown(true);
    Serial.println("Display was turned OFF: " + now());
    displayOffEpoch = lastEpoch;
  }
}

// Toggle on and off the display if user defined times
void checkDisplay() {
  if (timeDisplayTurnsOn == "" || timeDisplayTurnsOff == "") {
    return; // nothing to do
  }
  String currentTime = TimeDB.zeroPad(hour()) + ":" + TimeDB.zeroPad(minute());

  if (currentTime == timeDisplayTurnsOn && !displayOn) {
    Serial.println("Time to turn display on: " + currentTime);
    flashLED(1, 500);
    enableDisplay(true);
  }

  if (currentTime == timeDisplayTurnsOff && displayOn) {
    Serial.println("Time to turn display off: " + currentTime);
    flashLED(2, 500);
    enableDisplay(false);
  }
}

String writeConfigJson() {

}

String writeConfig() {
  // Save decoded message to LittleFS file for playback on power up.
  File f = LittleFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("TIMEDBKEY=" + TIMEDBKEY);
    f.println("APIKEY=" + APIKEY);
    f.println("CityID=" + String(CityIDs[0]));
    f.println("marqueeMessage=" + marqueeMessage);
    f.println("newsSource=" + NEWS_SOURCE);
    f.println("timeDisplayTurnsOn=" + timeDisplayTurnsOn);
    f.println("timeDisplayTurnsOff=" + timeDisplayTurnsOff);
    f.println("ledIntensity=" + String(displayIntensity));
    f.println("scrollSpeed=" + String(displayScrollSpeed));
    f.println("isNews=" + String(NEWS_ENABLED));
    f.println("newsApiKey=" + NEWS_API_KEY);
    f.println("isFlash=" + String(flashOnSeconds));
    f.println("is24hour=" + String(IS_24HOUR));
    f.println("isPM=" + String(IS_PM));
    f.println("wideclockformat=" + Wide_Clock_Style);
    f.println("isMetric=" + String(IS_METRIC));
    f.println("refreshRate=" + String(minutesBetweenDataRefresh));
    f.println("minutesBetweenScrolling=" + String(minutesBetweenScrolling));
    f.println("isOctoPrint=" + String(OCTOPRINT_ENABLED));
    f.println("isOctoProgress=" + String(OCTOPRINT_PROGRESS));
    f.println("octoKey=" + OctoPrintApiKey);
    f.println("octoServer=" + OctoPrintServer);
    f.println("octoPort=" + String(OctoPrintPort));
    f.println("octoUser=" + OctoAuthUser);
    f.println("octoPass=" + OctoAuthPass);
    f.println("www_username=" + String(www_username));
    f.println("www_password=" + String(www_password));
    f.println("IS_BASIC_AUTH=" + String(IS_BASIC_AUTH));
    f.println("BitcoinCurrencyCode=" + BitcoinCurrencyCode);
    f.println("SHOW_CITY=" + String(SHOW_CITY));
    f.println("SHOW_CONDITION=" + String(SHOW_CONDITION));
    f.println("SHOW_HUMIDITY=" + String(SHOW_HUMIDITY));
    f.println("SHOW_WIND=" + String(SHOW_WIND));
    f.println("SHOW_PRESSURE=" + String(SHOW_PRESSURE));
    f.println("SHOW_HIGHLOW=" + String(SHOW_HIGHLOW));
    f.println("SHOW_FEELSLIKE=" + String(SHOW_FEELSLIKE));
    f.println("SHOW_DATE=" + String(SHOW_DATE));
    f.println("USE_PIHOLE=" + String(USE_PIHOLE));
    f.println("PiHoleServer=" + PiHoleServer);
    f.println("PiHolePort=" + String(PiHolePort));
  }
  f.close();
  readCityIds();
  weatherClient.updateCityIdList(CityIDs, 1);
  String cityIds = weatherClient.getMyCityIDs();
  return cityIds;
}

void readCityIds() {
  if (LittleFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeConfig();
    return;
  }
  File fr = LittleFS.open(CONFIG, "r");
  String line;
  while (fr.available()) {
    line = fr.readStringUntil('\n');
    if (line.indexOf("TIMEDBKEY=") >= 0) {
      TIMEDBKEY = line.substring(line.lastIndexOf("TIMEDBKEY=") + 10);
      TIMEDBKEY.trim();
      Serial.println("TIMEDBKEY: " + TIMEDBKEY);
    }
    if (line.indexOf("APIKEY=") >= 0) {
      APIKEY = line.substring(line.lastIndexOf("APIKEY=") + 7);
      APIKEY.trim();
      Serial.println("APIKEY: " + APIKEY);
    }
    if (line.indexOf("CityID=") >= 0) {
      CityIDs[0] = line.substring(line.lastIndexOf("CityID=") + 7).toInt();
      Serial.println("CityID: " + String(CityIDs[0]));
    }
    if (line.indexOf("newsSource=") >= 0) {
      NEWS_SOURCE = line.substring(line.lastIndexOf("newsSource=") + 11);
      NEWS_SOURCE.trim();
      Serial.println("newsSource=" + NEWS_SOURCE);
    }
    if (line.indexOf("isNews=") >= 0) {
      NEWS_ENABLED = line.substring(line.lastIndexOf("isNews=") + 7).toInt();
      Serial.println("NEWS_ENABLED=" + String(NEWS_ENABLED));
    }
    if (line.indexOf("newsApiKey=") >= 0) {
      NEWS_API_KEY = line.substring(line.lastIndexOf("newsApiKey=") + 11);
      NEWS_API_KEY.trim();
      Serial.println("NEWS_API_KEY: " + NEWS_API_KEY);
    }
    if (line.indexOf("isFlash=") >= 0) {
      flashOnSeconds = line.substring(line.lastIndexOf("isFlash=") + 8).toInt();
      Serial.println("flashOnSeconds=" + String(flashOnSeconds));
    }
    if (line.indexOf("is24hour=") >= 0) {
      IS_24HOUR = line.substring(line.lastIndexOf("is24hour=") + 9).toInt();
      Serial.println("IS_24HOUR=" + String(IS_24HOUR));
    }
    if (line.indexOf("isPM=") >= 0) {
      IS_PM = line.substring(line.lastIndexOf("isPM=") + 5).toInt();
      Serial.println("IS_PM=" + String(IS_PM));
    }
    if (line.indexOf("wideclockformat=") >= 0) {
      Wide_Clock_Style = line.substring(line.lastIndexOf("wideclockformat=") + 16);
      Wide_Clock_Style.trim();
      Serial.println("Wide_Clock_Style=" + Wide_Clock_Style);
    }
    if (line.indexOf("isMetric=") >= 0) {
      IS_METRIC = line.substring(line.lastIndexOf("isMetric=") + 9).toInt();
      Serial.println("IS_METRIC=" + String(IS_METRIC));
    }
    if (line.indexOf("refreshRate=") >= 0) {
      minutesBetweenDataRefresh = line.substring(line.lastIndexOf("refreshRate=") + 12).toInt();
      if (minutesBetweenDataRefresh == 0) {
        minutesBetweenDataRefresh = 15; // can't be zero
      }
      Serial.println("minutesBetweenDataRefresh=" + String(minutesBetweenDataRefresh));
    }
    if (line.indexOf("minutesBetweenScrolling=") >= 0) {
      displayRefreshCount = 1;
      minutesBetweenScrolling = line.substring(line.lastIndexOf("minutesBetweenScrolling=") + 24).toInt();
      Serial.println("minutesBetweenScrolling=" + String(minutesBetweenScrolling));
    }
    if (line.indexOf("marqueeMessage=") >= 0) {
      marqueeMessage = line.substring(line.lastIndexOf("marqueeMessage=") + 15);
      marqueeMessage.trim();
      Serial.println("marqueeMessage=" + marqueeMessage);
    }
    if (line.indexOf("timeDisplayTurnsOn=") >= 0) {
      timeDisplayTurnsOn = line.substring(line.lastIndexOf("timeDisplayTurnsOn=") + 19);
      timeDisplayTurnsOn.trim();
      Serial.println("timeDisplayTurnsOn=" + timeDisplayTurnsOn);
    }
    if (line.indexOf("timeDisplayTurnsOff=") >= 0) {
      timeDisplayTurnsOff = line.substring(line.lastIndexOf("timeDisplayTurnsOff=") + 20);
      timeDisplayTurnsOff.trim();
      Serial.println("timeDisplayTurnsOff=" + timeDisplayTurnsOff);
    }
    if (line.indexOf("ledIntensity=") >= 0) {
      displayIntensity = line.substring(line.lastIndexOf("ledIntensity=") + 13).toInt();
      Serial.println("displayIntensity=" + String(displayIntensity));
    }
    if (line.indexOf("scrollSpeed=") >= 0) {
      displayScrollSpeed = line.substring(line.lastIndexOf("scrollSpeed=") + 12).toInt();
      Serial.println("displayScrollSpeed=" + String(displayScrollSpeed));
    }
    if (line.indexOf("isOctoPrint=") >= 0) {
      OCTOPRINT_ENABLED = line.substring(line.lastIndexOf("isOctoPrint=") + 12).toInt();
      Serial.println("OCTOPRINT_ENABLED=" + String(OCTOPRINT_ENABLED));
    }
    if (line.indexOf("isOctoProgress=") >= 0) {
      OCTOPRINT_PROGRESS = line.substring(line.lastIndexOf("isOctoProgress=") + 15).toInt();
      Serial.println("OCTOPRINT_PROGRESS=" + String(OCTOPRINT_PROGRESS));
    }
    if (line.indexOf("octoKey=") >= 0) {
      OctoPrintApiKey = line.substring(line.lastIndexOf("octoKey=") + 8);
      OctoPrintApiKey.trim();
      Serial.println("OctoPrintApiKey=" + OctoPrintApiKey);
    }
    if (line.indexOf("octoServer=") >= 0) {
      OctoPrintServer = line.substring(line.lastIndexOf("octoServer=") + 11);
      OctoPrintServer.trim();
      Serial.println("OctoPrintServer=" + OctoPrintServer);
    }
    if (line.indexOf("octoPort=") >= 0) {
      OctoPrintPort = line.substring(line.lastIndexOf("octoPort=") + 9).toInt();
      Serial.println("OctoPrintPort=" + String(OctoPrintPort));
    }
    if (line.indexOf("octoUser=") >= 0) {
      OctoAuthUser = line.substring(line.lastIndexOf("octoUser=") + 9);
      OctoAuthUser.trim();
      Serial.println("OctoAuthUser=" + OctoAuthUser);
    }
    if (line.indexOf("octoPass=") >= 0) {
      OctoAuthPass = line.substring(line.lastIndexOf("octoPass=") + 9);
      OctoAuthPass.trim();
      Serial.println("OctoAuthPass=" + OctoAuthPass);
    }
    if (line.indexOf("www_username=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_username=") + 13);
      temp.trim();
      temp.toCharArray(www_username, sizeof(temp));
      Serial.println("www_username=" + String(www_username));
    }
    if (line.indexOf("www_password=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_password=") + 13);
      temp.trim();
      temp.toCharArray(www_password, sizeof(temp));
      Serial.println("www_password=" + String(www_password));
    }
    if (line.indexOf("IS_BASIC_AUTH=") >= 0) {
      IS_BASIC_AUTH = line.substring(line.lastIndexOf("IS_BASIC_AUTH=") + 14).toInt();
      Serial.println("IS_BASIC_AUTH=" + String(IS_BASIC_AUTH));
    }
    if (line.indexOf("BitcoinCurrencyCode=") >= 0) {
      BitcoinCurrencyCode = line.substring(line.lastIndexOf("BitcoinCurrencyCode=") + 20);
      BitcoinCurrencyCode.trim();
      Serial.println("BitcoinCurrencyCode=" + BitcoinCurrencyCode);
    }
    if (line.indexOf("SHOW_CITY=") >= 0) {
      SHOW_CITY = line.substring(line.lastIndexOf("SHOW_CITY=") + 10).toInt();
      Serial.println("SHOW_CITY=" + String(SHOW_CITY));
    }
    if (line.indexOf("SHOW_CONDITION=") >= 0) {
      SHOW_CONDITION = line.substring(line.lastIndexOf("SHOW_CONDITION=") + 15).toInt();
      Serial.println("SHOW_CONDITION=" + String(SHOW_CONDITION));
    }
    if (line.indexOf("SHOW_HUMIDITY=") >= 0) {
      SHOW_HUMIDITY = line.substring(line.lastIndexOf("SHOW_HUMIDITY=") + 14).toInt();
      Serial.println("SHOW_HUMIDITY=" + String(SHOW_HUMIDITY));
    }
    if (line.indexOf("SHOW_WIND=") >= 0) {
      SHOW_WIND = line.substring(line.lastIndexOf("SHOW_WIND=") + 10).toInt();
      Serial.println("SHOW_WIND=" + String(SHOW_WIND));
    }
    if (line.indexOf("SHOW_PRESSURE=") >= 0) {
      SHOW_PRESSURE = line.substring(line.lastIndexOf("SHOW_PRESSURE=") + 14).toInt();
      Serial.println("SHOW_PRESSURE=" + String(SHOW_PRESSURE));
    }

    if (line.indexOf("SHOW_HIGHLOW=") >= 0) {
      SHOW_HIGHLOW = line.substring(line.lastIndexOf("SHOW_HIGHLOW=") + 13).toInt();
      Serial.println("SHOW_HIGHLOW=" + String(SHOW_HIGHLOW));
    }
    
    if (line.indexOf("SHOW_FEELSLIKE=") >= 0) {
      SHOW_FEELSLIKE = line.substring(line.lastIndexOf("SHOW_FEELSLIKE=") + 15).toInt();
      Serial.println("SHOW_FEELSLIKE=" + String(SHOW_FEELSLIKE));
    }
    
    if (line.indexOf("SHOW_DATE=") >= 0) {
      SHOW_DATE = line.substring(line.lastIndexOf("SHOW_DATE=") + 10).toInt();
      Serial.println("SHOW_DATE=" + String(SHOW_DATE));
    }

    if (line.indexOf("USE_PIHOLE=") >= 0) {
      USE_PIHOLE = line.substring(line.lastIndexOf("USE_PIHOLE=") + 11).toInt();
      Serial.println("USE_PIHOLE=" + String(USE_PIHOLE));
    }
    if (line.indexOf("PiHoleServer=") >= 0) {
      PiHoleServer = line.substring(line.lastIndexOf("PiHoleServer=") + 13);
      PiHoleServer.trim();
      Serial.println("PiHoleServer=" + PiHoleServer);
    }
    if (line.indexOf("PiHolePort=") >= 0) {
      PiHolePort = line.substring(line.lastIndexOf("PiHolePort=") + 11).toInt();
      Serial.println("PiHolePort=" + String(PiHolePort));
    }
  }
  fr.close();
  matrix.setIntensity(displayIntensity);
  newsClient.updateNewsClient(NEWS_API_KEY, NEWS_SOURCE);
  weatherClient.updateWeatherApiKey(APIKEY);
  weatherClient.setMetric(IS_METRIC);
  weatherClient.updateCityIdList(CityIDs, 1);
  printerClient.updateOctoPrintClient(OctoPrintApiKey, OctoPrintServer, OctoPrintPort, OctoAuthUser, OctoAuthPass);
}

void scrollMessage(String msg) {
  msg += " "; // add a space at the end
  for ( int i = 0 ; i < width * msg.length() + matrix.width() - 1 - spacer; i++ ) {
    if (WEBSERVER_ENABLED) {
      server.handleClient();
    }
    if (ENABLE_OTA) {
      ArduinoOTA.handle();
    }
    if (refresh == 1) i = 0;
    refresh = 0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < msg.length() ) {
        matrix.drawChar(x, y, msg[letter], HIGH, LOW, 1);
      }

      letter--;
      x -= width;
    }

    matrix.write(); // Send bitmap to display
    delay(displayScrollSpeed);
  }
  matrix.setCursor(0, 0);
}

void drawPiholeGraph() {
  if (!USE_PIHOLE || piholeClient.getBlockedCount() == 0) {
    return;
  }
  int count = piholeClient.getBlockedCount();
  int high = 0;
  int row = matrix.width() - 1;
  int yval = 0;

  int totalRows = count - matrix.width();
  
  if (totalRows < 0) {
    totalRows = 0;
  }

  // get the high value for the sample that will be on the screen
  for (int inx = count; inx >= totalRows; inx--) {
    if (piholeClient.getBlockedAds()[inx] > high) {
      high = (int)piholeClient.getBlockedAds()[inx];
    }
  }

  int currentVal = 0;
  for (int inx = (count-1); inx >= totalRows; inx--) {
    currentVal = (int)piholeClient.getBlockedAds()[inx];
    yval = map(currentVal, 0, high, 7, 0);
    //Serial.println("Value: " + String(currentVal));
    //Serial.println("x: " + String(row) + " y:" + String(yval) + " h:" + String(8-yval));
    matrix.drawFastVLine(row, yval, 8-yval, HIGH);
    if (row == 0) {
      break;
    }
    row--;
  }
  matrix.write();
  for (int wait = 0; wait < 500; wait++) {
    if (WEBSERVER_ENABLED) {
      server.handleClient();
    }
    if (ENABLE_OTA) {
      ArduinoOTA.handle();
    }
    delay(20);
  }
}

// void centerPrint(String msg) {
//   centerPrint(msg, false);
// }

void centerPrint(String msg, boolean extraStuff) {
  int x = (matrix.width() - (msg.length() * width)) / 2;

  // Print the static portions of the display before the main Message
  if (extraStuff) {
    if (!IS_24HOUR && IS_PM && isPM()) {
      matrix.drawPixel(matrix.width() - 1, 6, HIGH);
    }

    if (OCTOPRINT_ENABLED && OCTOPRINT_PROGRESS && printerClient.isPrinting()) {
      int numberOfLightPixels = (printerClient.getProgressCompletion().toFloat() / float(100)) * (matrix.width() - 1);
      matrix.drawFastHLine(0, 7, numberOfLightPixels, HIGH);
    }
    
  }
  
  matrix.setCursor(x, 0);
  matrix.print(msg);
  //Serial.println("Debug - scrolled message: " + msg);
  matrix.write();
}

String decodeHtmlString(String msg) {
  String decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser
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
