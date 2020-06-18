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

  BearSSL::WiFiClientSecure newSecure;
  newSecure.setFingerprint(fingerprint);
  newSecure.setInsecure();
  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  if (https.begin(newSecure, host, httpsPort,
                  "/NBUStatService/v1/statdirectory/exchangenew?json&valcode=" + currencyCode)) {  // HTTPS
    Serial.print("[HTTPS] GET...\n");
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String content = https.getString();
        Serial.println(content);
        const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + 260;
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, content);
        if (error) {
          Serial.print(F("*NBU: deserializeJson() failed: "));
          Serial.println(error.c_str());
        } else {
          nbuData.code = (const char*)doc[0]["cc"];
          nbuData.rate = doc[0]["rate"];
          nbuData.description = (const char*)doc[0]["txt"];
          Serial.println("*NBU: cc: " + nbuData.code);
          Serial.println("*NBU: rate: " + String(nbuData.rate));
          Serial.println("*NBU: txt: " + nbuData.description);
          Serial.println();
        }
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    // https.end();
    // newSecure.stopAll();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }

  // // Use WiFiClientSecure class to create TLS connection
  // WiFiClientSecure client;
  // client.setFingerprint(fingerprint);
  // client.setInsecure();
  // HTTPClient http;
  // String httpAddress = "https://" + String(host) + ":" + httpsPort + "/NBUStatService/v1/statdirectory/exchangenew?json&valcode=" + currencyCode;
  // Serial.println("*NBU: getting NBU Data: " + httpAddress);

  // if (http.begin(client, httpAddress)) {  // HTTP
  //   int httpCode = http.GET();
  //   if (httpCode > 0) {
  //     if (httpCode == HTTP_CODE_OK) {
  //       String content = http.getString();
  //       const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + 260;
  //       DynamicJsonDocument doc(capacity);
  //       DeserializationError error = deserializeJson(doc, content);
  //       if (error) {
  //         Serial.print(F("*NBU: deserializeJson() failed: "));
  //         Serial.println(error.c_str());
  //         return;
  //       }
  //       // serializeJsonPretty(doc, Serial);
  //       // Serial.println();

  //       nbuData.code = (const char*)doc[0]["cc"];
  //       nbuData.rate = doc[0]["rate"];
  //       nbuData.description = (const char*)doc[0]["txt"];
  //     }
  //   } else {
  //     Serial.printf("*NBU: GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  //   }

  //   http.end();
  //   client.stop();
  // } else {
  //   Serial.println("*NBU: unable to connect");
  // }

  // Serial.println("*NBU: cc: " + nbuData.code);
  // Serial.println("*NBU: rate: " + String(nbuData.rate));
  // Serial.println("*NBU: txt: " + nbuData.description);

  // Serial.println();
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
