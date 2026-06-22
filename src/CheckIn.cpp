#include "CheckIn.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
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

  // An https URL (e.g. a remote display reaching HA through a TLS ingress) needs
  // a secure client. setInsecure() skips certificate verification: the traffic
  // is still encrypted, but it doesn't pin a CA. Good enough for the webhook id;
  // can be hardened with setCACert() later. Plain http keeps the lighter client.
  bool isHttps = url.startsWith("https://") || url.startsWith("HTTPS://");
  WiFiClient plain;
  WiFiClientSecure secure;
  WiFiClient* client = &plain;
  if (isHttps) {
    secure.setInsecure();
    client = &secure;
  }

  HTTPClient http;
  if (!http.begin(*client, url)) {
    httpCode = -1;
    return false;
  }
  http.setTimeout(isHttps ? 12000 : 8000);  // TLS handshake is slower
  http.addHeader("Content-Type", "application/json");

  httpCode = http.POST(body);
  bool ok = false;
  if (httpCode == 200) {
    ok = parseBundle(http.getString(), out);
  }
  http.end();
  return ok;
}
