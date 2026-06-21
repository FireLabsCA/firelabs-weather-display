#include "Weather.h"
#include <ArduinoJson.h>

static void num(JsonVariantConst v, WxValue& out) {
  if (!v.isNull()) {
    out.v = v.as<float>();
    out.has = true;
  }
}

bool parseBundle(const String& json, WeatherBundle& out) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  out = WeatherBundle{};

  out.updated = (const char*)(doc["updated"] | "");

  JsonObjectConst c = doc["current"];
  num(c["temp"], out.current.temp);
  num(c["feels_like"], out.current.feels);
  num(c["humidity"], out.current.humidity);
  num(c["wind"], out.current.wind);
  num(c["gust"], out.current.gust);
  num(c["precip"], out.current.precip);
  num(c["uv"], out.current.uv);
  num(c["high"], out.current.high);
  num(c["low"], out.current.low);
  num(c["pop"], out.current.pop);
  out.current.condition = (const char*)(c["condition"] | "");

  uint8_t n = 0;
  for (JsonObjectConst o : doc["forecast"].as<JsonArrayConst>()) {
    if (n >= WX_FORECAST_MAX) break;
    out.forecast[n].time = (const char*)(o["time"] | "");
    num(o["temp"], out.forecast[n].temp);
    out.forecast[n].cond = (const char*)(o["cond"] | "");
    n++;
  }
  out.forecastCount = n;

  JsonObjectConst s = doc["settings"];
  out.settings.sleepMin = s["sleep_min"] | 30;
  out.settings.quietStart = s["quiet_start"] | 21;
  out.settings.quietEnd = s["quiet_end"] | 6;
  out.settings.forceWake = s["force_wake"] | false;

  out.ota.version = (const char*)(doc["ota"]["version"] | "");
  out.ota.url = (const char*)(doc["ota"]["url"] | "");
  return true;
}
