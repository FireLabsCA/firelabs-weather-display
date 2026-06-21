#include "Config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* Config::kPath = "/config.json";

bool Config::load() {
  LittleFS.begin(true);
  File f = LittleFS.open(kPath, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  webhookUrl = doc["webhook"]     | "";
  sleepMin   = doc["sleep_min"]   | 30;
  quietStart = doc["quiet_start"] | 21;
  quietEnd   = doc["quiet_end"]   | 6;
  return true;
}

bool Config::save() const {
  LittleFS.begin(true);
  JsonDocument doc;
  doc["webhook"]     = webhookUrl;
  doc["sleep_min"]   = sleepMin;
  doc["quiet_start"] = quietStart;
  doc["quiet_end"]   = quietEnd;
  File f = LittleFS.open(kPath, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

void Config::clear() {
  LittleFS.begin(true);
  LittleFS.remove(kPath);
}
