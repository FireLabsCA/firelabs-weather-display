#pragma once
#include <Arduino.h>

// Device settings only. WiFi credentials and the friendly name live in
// FirelabsCore; this holds the weather-display-specific config.
class Config {
public:
  String webhookUrl;          // HA webhook the device checks in with
  uint16_t sleepMin   = 30;
  uint8_t  quietStart = 21;
  uint8_t  quietEnd   = 6;

  bool load();
  bool save() const;
  void clear();
  bool isConfigured() const { return webhookUrl.length() > 0; }

private:
  static const char* kPath;
};
