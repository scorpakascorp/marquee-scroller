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

#include "OpenWeatherMapClient.h"

#include "math.h"

OpenWeatherMapClient::OpenWeatherMapClient(String ApiKey, int CityIDs[], int cityCount, boolean isMetric) {
  updateCityIdList(CityIDs, cityCount);
  myApiKey = ApiKey;
  setMetric(isMetric);
}

void OpenWeatherMapClient::updateWeatherApiKey(String ApiKey) {
  myApiKey = ApiKey;
}

void OpenWeatherMapClient::updateWeather() {
  WiFiClient weatherClient;
  String apiGetData = "GET /data/2.5/group?id=" + myCityIDs + "&units=" + units + "&cnt=1&APPID=" + myApiKey + " HTTP/1.1";

  Serial.println("*OWMC: Getting Weather Data");
  Serial.println("*OWMC: request: " + apiGetData);
  weathers[0].cached = false;
  weathers[0].error = "";
  if (weatherClient.connect(servername, 80)) {  // starts client connection, checks for connection
    weatherClient.println(apiGetData);
    weatherClient.println("Host: " + String(servername));
    weatherClient.println("User-Agent: ArduinoWiFi/1.1");
    weatherClient.println("Connection: close");
    weatherClient.println();
  } else {
    Serial.println("*OWMC: Connection for weather data failed");  // error message if no
                                                                  // client connect
    Serial.println();
    weathers[0].error = "Connection for weather data failed";
    return;
  }

  while (weatherClient.connected() && !weatherClient.available())
    delay(1);  // waits for data

  Serial.println("*OWMC: Waiting for data...");

  // Check HTTP status
  char status[32] = {0};
  weatherClient.readBytesUntil('\r', status, sizeof(status));
  Serial.println("*OWMC: Response Header: " + String(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("*OWMC: Unexpected response: "));
    Serial.println(status);
    weathers[0].error = "Weather Data Error: " + String(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!weatherClient.find(endOfHeaders)) {
    Serial.println(F("*OWMC: Invalid response"));
    return;
  }

  const size_t capacity = 2 * JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) +
                          3 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) +
                          JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(10) + 270;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, weatherClient);
  if (error) {
    Serial.print(F("*OWMC: JsonDeserealization error "));
    Serial.println(error.c_str());
    return;
  }

  weatherClient.stop();  // stop client

  if (doc.size() <= 1) {
    Serial.println("*OWMC: Error Does not look like we got the data.  Size: " + String(doc.size()));
    weathers[0].cached = true;
    weathers[0].error = (const char*)doc["message"];
    Serial.println("*OWMC: Error: " + weathers[0].error);
    return;
  }
  int count = doc["cnt"];

  for (int inx = 0; inx < count; inx++) {
    weathers[inx].lat = String((float)doc["list"][inx]["coord"]["lat"]);
    weathers[inx].lon = String((float)doc["list"][inx]["coord"]["lon"]);
    weathers[inx].dt = String((long)doc["list"][inx]["dt"]);
    weathers[inx].name = (const char*)doc["list"][inx]["name"];
    weathers[inx].country = (const char*)doc["list"][inx]["sys"]["country"];
    weathers[inx].timeZone = String((int)doc["list"][inx]["sys"]["timezone"]);
    weathers[inx].temp = String((int)doc["list"][inx]["main"]["temp"]);
    weathers[inx].feels_like = String((float)doc["list"][inx]["main"]["feels_like"]);
    weathers[inx].humidity = String((int)doc["list"][inx]["main"]["humidity"]);
    weathers[inx].pressure = String((int)doc["list"][inx]["main"]["pressure"]);
    weathers[inx].high = String((int)doc["list"][inx]["main"]["temp_max"]);
    weathers[inx].low = String((int)doc["list"][inx]["main"]["temp_min"]);
    weathers[inx].condition = (const char*)doc["list"][inx]["weather"][0]["main"];
    weathers[inx].weatherId = String((int)doc["list"][inx]["weather"][0]["id"]);
    weathers[inx].description = (const char*)doc["list"][inx]["weather"][0]["description"];
    weathers[inx].icon = (const char*)doc["list"][inx]["weather"][0]["icon"];
    weathers[inx].wind = String((int)doc["list"][inx]["wind"]["speed"]);
    weathers[inx].direction = String((int)doc["list"][inx]["wind"]["deg"]);

    if (units == "metric") {
      float f = (weathers[inx].wind.toFloat());
      weathers[inx].wind = String(f);
    }

    if (units != "metric") {
      float p = (weathers[inx].pressure.toFloat() * 0.0295301);  // convert millibars to inches
      weathers[inx].pressure = String(p);
    } else {
      float p = (weathers[inx].pressure.toFloat() * 0.750062); // mbar to mm of hg
      weathers[inx].pressure = String(p);      
    }

    Serial.println("*OWMC: lat: " + weathers[inx].lat);
    Serial.println("*OWMC: lon: " + weathers[inx].lon);
    Serial.println("*OWMC: dt: " + weathers[inx].dt);
    Serial.println("*OWMC: city: " + weathers[inx].name);
    Serial.println("*OWMC: country: " + weathers[inx].country);
    Serial.println("*OWMC: temp: " + weathers[inx].temp);
    Serial.println("*OWMC: feels_like: " + weathers[inx].feels_like);
    Serial.println("*OWMC: humidity: " + weathers[inx].humidity);
    Serial.println("*OWMC: condition: " + weathers[inx].condition);
    Serial.println("*OWMC: wind: " + weathers[inx].wind);
    Serial.println("*OWMC: direction: " + weathers[inx].direction);
    Serial.println("*OWMC: weatherId: " + weathers[inx].weatherId);
    Serial.println("*OWMC: description: " + weathers[inx].description);
    Serial.println("*OWMC: icon: " + weathers[inx].icon);
    Serial.println("*OWMC: timezone: " + String(getTimeZone(inx)));
    Serial.println();
  }
}

String OpenWeatherMapClient::roundValue(String value) {
  float f = value.toFloat();
  int rounded = (int)(f + 0.5f);
  return String(rounded);
}

void OpenWeatherMapClient::updateCityIdList(int CityIDs[], int cityCount) {
  myCityIDs = "";
  for (int inx = 0; inx < cityCount; inx++) {
    if (CityIDs[inx] > 0) {
      if (myCityIDs != "") {
        myCityIDs = myCityIDs + ",";
      }
      myCityIDs = myCityIDs + String(CityIDs[inx]);
    }
  }
}

void OpenWeatherMapClient::setMetric(boolean isMetric) {
  units = (isMetric) ? "metric" : "imperial";
}

String OpenWeatherMapClient::getLat(int index) {
  return weathers[index].lat;
}

String OpenWeatherMapClient::getLon(int index) {
  return weathers[index].lon;
}

String OpenWeatherMapClient::getDt(int index) {
  return weathers[index].dt;
}

String OpenWeatherMapClient::getCity(int index) {
  return weathers[index].name;
}

String OpenWeatherMapClient::getCountry(int index) {
  return weathers[index].country;
}

String OpenWeatherMapClient::getTemp(int index) {
  return weathers[index].temp;
}

String OpenWeatherMapClient::getTempRounded(int index) {
  return roundValue(getTemp(index));
}

String OpenWeatherMapClient::getFeelsLike(int index) {
  return weathers[index].feels_like;
}

String OpenWeatherMapClient::getFeelsLikeRounded(int index) {
  return roundValue(getFeelsLike(index));
}

String OpenWeatherMapClient::getHumidity(int index) {
  return weathers[index].humidity;
}

String OpenWeatherMapClient::getHumidityRounded(int index) {
  return roundValue(getHumidity(index));
}

String OpenWeatherMapClient::getCondition(int index) {
  return weathers[index].condition;
}

String OpenWeatherMapClient::getWind(int index) {
  return weathers[index].wind;
}

String OpenWeatherMapClient::getDirection(int index) {
  return weathers[index].direction;
}

String OpenWeatherMapClient::getDirectionRounded(int index) {
  return roundValue(getDirection(index));
}

String OpenWeatherMapClient::getDirectionText(int index) {
  int num = getDirectionRounded(index).toInt();
  int val = floor((num / 22.5) + 0.5);
  String arr[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                  "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  return arr[(val % 16)];
}

String OpenWeatherMapClient::getWindRounded(int index) {
  return roundValue(getWind(index));
}

String OpenWeatherMapClient::getWeatherId(int index) {
  return weathers[index].weatherId;
}

String OpenWeatherMapClient::getDescription(int index) {
  return weathers[index].description;
}

String OpenWeatherMapClient::getPressure(int index) {
  // return weathers[index].pressure / 1.333;
  return weathers[index].pressure;
}

String OpenWeatherMapClient::getPressureRounded(int index) {
  return roundValue(getPressure(index));
}

String OpenWeatherMapClient::getHigh(int index) {
  return weathers[index].high;
}

String OpenWeatherMapClient::getLow(int index) {
  return weathers[index].low;
}

String OpenWeatherMapClient::getIcon(int index) {
  return weathers[index].icon;
}

boolean OpenWeatherMapClient::getCached() {
  return weathers[0].cached;
}

String OpenWeatherMapClient::getMyCityIDs() {
  return myCityIDs;
}

String OpenWeatherMapClient::getError() {
  return weathers[0].error;
}

String OpenWeatherMapClient::getWeekDay(int index, float offset) {
  String rtnValue = "";
  long epoc = weathers[index].dt.toInt();
  long day = 0;
  if (epoc != 0) {
    day = (((epoc + (3600 * (int)offset)) / 86400) + 4) % 7;
    switch (day) {
      case 0:
        rtnValue = "Sunday";
        break;
      case 1:
        rtnValue = "Monday";
        break;
      case 2:
        rtnValue = "Tuesday";
        break;
      case 3:
        rtnValue = "Wednesday";
        break;
      case 4:
        rtnValue = "Thursday";
        break;
      case 5:
        rtnValue = "Friday";
        break;
      case 6:
        rtnValue = "Saturday";
        break;
      default:
        break;
    }
  }
  return rtnValue;
}

int OpenWeatherMapClient::getTimeZone(int index) {
  int rtnValue = weathers[index].timeZone.toInt();
  if (rtnValue != 0) {
    rtnValue = rtnValue / 3600;
  }
  return rtnValue;
}

String OpenWeatherMapClient::getWeatherIcon(int index) {
  int id = getWeatherId(index).toInt();
  String W = ")";
  switch (id) {
    case 800:
      W = "B";
      break;
    case 801:
      W = "Y";
      break;
    case 802:
      W = "H";
      break;
    case 803:
      W = "H";
      break;
    case 804:
      W = "Y";
      break;

    case 200:
      W = "0";
      break;
    case 201:
      W = "0";
      break;
    case 202:
      W = "0";
      break;
    case 210:
      W = "0";
      break;
    case 211:
      W = "0";
      break;
    case 212:
      W = "0";
      break;
    case 221:
      W = "0";
      break;
    case 230:
      W = "0";
      break;
    case 231:
      W = "0";
      break;
    case 232:
      W = "0";
      break;

    case 300:
      W = "R";
      break;
    case 301:
      W = "R";
      break;
    case 302:
      W = "R";
      break;
    case 310:
      W = "R";
      break;
    case 311:
      W = "R";
      break;
    case 312:
      W = "R";
      break;
    case 313:
      W = "R";
      break;
    case 314:
      W = "R";
      break;
    case 321:
      W = "R";
      break;

    case 500:
      W = "R";
      break;
    case 501:
      W = "R";
      break;
    case 502:
      W = "R";
      break;
    case 503:
      W = "R";
      break;
    case 504:
      W = "R";
      break;
    case 511:
      W = "R";
      break;
    case 520:
      W = "R";
      break;
    case 521:
      W = "R";
      break;
    case 522:
      W = "R";
      break;
    case 531:
      W = "R";
      break;

    case 600:
      W = "W";
      break;
    case 601:
      W = "W";
      break;
    case 602:
      W = "W";
      break;
    case 611:
      W = "W";
      break;
    case 612:
      W = "W";
      break;
    case 615:
      W = "W";
      break;
    case 616:
      W = "W";
      break;
    case 620:
      W = "W";
      break;
    case 621:
      W = "W";
      break;
    case 622:
      W = "W";
      break;

    case 701:
      W = "M";
      break;
    case 711:
      W = "M";
      break;
    case 721:
      W = "M";
      break;
    case 731:
      W = "M";
      break;
    case 741:
      W = "M";
      break;
    case 751:
      W = "M";
      break;
    case 761:
      W = "M";
      break;
    case 762:
      W = "M";
      break;
    case 771:
      W = "M";
      break;
    case 781:
      W = "M";
      break;

    default:
      break;
  }
  return W;
}
