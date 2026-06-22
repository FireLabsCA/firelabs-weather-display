#pragma once
#include <Arduino.h>
#include "Weather.h"

// E-paper screens: setup/status plus the weather layout.
namespace Display {
void begin(bool fromSleep = false);  // fromSleep skips the full panel re-init
void showSetup(const String& apSsid);                       // phase 1: join the AP
void showConfigPrompt(const String& host, const String& ip);// phase 2: configure in browser
void showMessage(const String& title, const String& line);  // generic status line
void showWeather(const WeatherBundle& wx, int batteryPct);  // the dashboard
}
