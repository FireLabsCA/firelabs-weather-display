#include "CheckIn.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

bool CheckIn::run(const String& url, const Telemetry& tel, WeatherBundle& out,
                  int& httpCode) {
  if (url.isEmpty()) {
    httpCode = 0;
    return false;
  }

  String body;
  {
    JsonDocument d;
    d["battery"] = tel.battery;
    d["voltage"] = tel.voltage;
    d["version"] = tel.version;
    d["wake"] = tel.wake;
    serializeJson(d, body);
  }

  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, url)) {
    httpCode = -1;
    return false;
  }
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");

  httpCode = http.POST(body);
  bool ok = false;
  if (httpCode == 200) {
    ok = parseBundle(http.getString(), out);
  }
  http.end();
  return ok;
}
