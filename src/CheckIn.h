#pragma once
#include <Arduino.h>
#include "Weather.h"

// One request per wake: POST telemetry to the HA webhook, get the weather bundle
// back in the response (SPEC.md "Data transport"). No broker, no HA token on the
// device; the webhook URL is the only secret it holds.
namespace CheckIn {
struct Telemetry {
  int battery;        // percent
  float voltage;      // volts
  const char* version;
  const char* wake;   // "boot", "timer", "button"
};

// Returns true and fills `out` on a 200 with a parseable bundle. `httpCode` gets
// the HTTP status (or a negative HTTPClient error) for diagnostics either way.
bool run(const String& url, const Telemetry& tel, WeatherBundle& out, int& httpCode);
}
