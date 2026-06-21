#pragma once
#include <Arduino.h>

// The weather bundle the device gets back from the HA webhook on each check-in.
// The field names here are the contract; the integration fills them (SPEC.md
// "Data transport"). Optional numeric fields carry a `has` flag so the render
// can show "--" when HA didn't map a source for them.

struct WxValue {
  bool has = false;
  float v = 0;
};

struct WxCurrent {
  WxValue temp, feels, humidity, wind, gust, precip, uv, high, low, pop;
  String condition;
};

struct WxForecast {
  String time;   // e.g. "3PM"
  WxValue temp;
  String cond;
};

struct WxSettings {
  uint16_t sleepMin = 30;
  uint8_t quietStart = 21;
  uint8_t quietEnd = 6;
  bool forceWake = false;
};

struct WxOta {
  String version;
  String url;
};

static const uint8_t WX_FORECAST_MAX = 5;

struct WeatherBundle {
  String updated;                          // ISO datetime from HA
  WxCurrent current;
  WxForecast forecast[WX_FORECAST_MAX];
  uint8_t forecastCount = 0;
  WxSettings settings;
  WxOta ota;
};

// Parse the JSON response body into the bundle. Returns false on malformed JSON.
bool parseBundle(const String& json, WeatherBundle& out);
