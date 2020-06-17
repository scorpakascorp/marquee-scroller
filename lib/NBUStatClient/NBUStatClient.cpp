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

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (http.begin(client, "https://" + String(host) + "/NBUStatService/v1/statdirectory/exchangenew?json&valcode=" + currencyCode)) {  // HTTP
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + 60;
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, http.getString());
        if (error) {
          Serial.print(F("*NBU: deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
        // serializeJsonPretty(doc, Serial);
        // Serial.println();

        nbuData.code = (const char*)doc[0]["cc"];
        nbuData.rate = doc[0]["rate"];
        nbuData.description = (const char*)doc[0]["txt"];
      }
    } else {
      Serial.printf("*NBU: GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("*NBU: unable to connect");
  }

  Serial.println("*NBU: cc: " + nbuData.code);
  Serial.println("*NBU: rate: " + String(nbuData.rate));
  Serial.println("*NBU: txt: " + nbuData.description);

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
