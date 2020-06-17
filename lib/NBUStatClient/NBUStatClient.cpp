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

#include "NBUStatClient.h"

NBUStatClient::NBUStatClient() {
  // Constructor
}

void NBUStatClient::updateNBUStatData(String currencyCode) {
  if (currencyCode == "" || currencyCode == "NONE") {
    nbuData.code = "";
    nbuData.rate = 0;
    nbuData.description = "";
    return;  // nothing to do here
  }
  WiFiClientSecure client;
  HTTPClient http;

  String apiGetData = "https://" + String(servername) + "/NBUStatService/v1/statdirectory/exchangenew?json&valcode=" + currencyCode;

  Serial.println("Getting NBUStat Data");
  Serial.println(apiGetData);
  client.setInsecure();
  http.begin(client, apiGetData);
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  // int httpCode = http.GET();

  String result = "";

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + 60;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // if (httpCode > 0) {  // checks for connection
  //   Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  //   if (httpCode == HTTP_CODE_OK) {
  //     // get length of document (is -1 when Server sends no Content-Length
  //     // header)
  //     int len = http.getSize();
  //     // create buffer for read
  //     char buff[128] = {0};
  //     // get tcp stream
  //     WiFiClient* stream = http.getStreamPtr();
  //     // read all data from server
  //     Serial.println("Start reading...");
  //     while (http.connected() && (len > 0 || len == -1)) {
  //       // get available data size
  //       size_t size = stream->available();
  //       if (size) {
  //         // read up to 128 byte
  //         int c = stream->readBytes(
  //             buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
  //         for (int i = 0; i < c; i++) {
  //           result += buff[i];
  //         }

  //         if (len > 0)
  //           len -= c;
  //       }
  //       delay(1);
  //     }
  //   }
  //   http.end();
  // } else {
  //   Serial.println("connection for NBUStat data failed: " +
  //                  String(apiGetData));  // error message if no client connect
  //   Serial.println();
  //   return;
  // }
  // // Clean dirty results
  // result.remove(0, result.indexOf("["));
  // result.remove(result.lastIndexOf("]") + 1);

  // char jsonArray[result.length() + 1];
  // result.toCharArray(jsonArray, sizeof(jsonArray));
  // // jsonArray[result.length() + 1] = '\0';
  // DynamicJsonDocument doc(1024);
  // DeserializationError error = deserializeJson(doc, jsonArray);
  // if (error) {
  //   Serial.print(F("NBUStatClient: JsonDeserealization error "));
  //   Serial.println(error.c_str());
  //   return;
  // }

  serializeJsonPretty(doc, Serial);
  Serial.println();

  nbuData.code = (const char*)doc[0]["cc"];
  nbuData.rate = doc[0]["rate"];
  nbuData.description = (const char*)doc[0]["txt"];

  Serial.println("cc: " + nbuData.code);
  Serial.println("rate: " + String(nbuData.rate));
  Serial.println("txt: " + nbuData.description);

  Serial.println();
}

String NBUStatClient::getCode() {
  return nbuData.code;
}

String NBUStatClient::getRate() {
  return String(nbuData.rate);
}

String NBUStatClient::getDescription() {
  return nbuData.description;
}
