#include "WebUi.h"
#include "Config.h"
#include "Page.h"
#include "Branding.h"
#include "FirelabsCore.h"
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static WebServer server(80);
static Config* cfg_ = nullptr;
static FirelabsCore* core_ = nullptr;
static bool begun_ = false;
static uint32_t rebootAt = 0;

static void servePage() { server.send_P(200, "text/html", CONFIG_HTML); }

void WebUi::begin(Config& cfg, FirelabsCore& core) {
  cfg_ = &cfg;
  core_ = &core;

  server.on("/", HTTP_GET, servePage);
  server.onNotFound(servePage);

  // Identity for the FireLabs HA integration (config_flow probe + device info).
  server.on("/api/status", HTTP_GET, []() {
    JsonDocument d;
    d["mac"] = WiFi.macAddress();
    d["model"] = "WX";
    d["name"] = core_->deviceName;
    d["fw"] = FL_FW_VERSION;
    String out;
    serializeJson(d, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/config", HTTP_GET, []() {
    JsonDocument d;
    d["host"] = core_->hostname();
    d["mac"] = WiFi.macAddress();
    d["name"] = core_->deviceName;
    d["webhook"] = cfg_->webhookUrl;
    d["sleep_min"] = cfg_->sleepMin;
    d["quiet_start"] = cfg_->quietStart;
    d["quiet_end"] = cfg_->quietEnd;
    String out;
    serializeJson(d, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/save", HTTP_POST, []() {
    JsonDocument d;
    if (deserializeJson(d, server.arg("plain"))) {
      server.send(400, "application/json", "{\"err\":1}");
      return;
    }
    if (d["webhook"].is<const char*>()) cfg_->webhookUrl = (const char*)d["webhook"];
    if (d["sleep_min"].is<int>())       cfg_->sleepMin = d["sleep_min"];
    if (d["quiet_start"].is<int>())     cfg_->quietStart = d["quiet_start"];
    if (d["quiet_end"].is<int>())       cfg_->quietEnd = d["quiet_end"];
    cfg_->save();
    server.send(200, "application/json", "{\"ok\":1}");
    rebootAt = millis() + 1500;
  });

  server.on("/api/reset", HTTP_POST, []() {
    cfg_->clear();
    core_->clear();  // also wipe wifi/name so it reboots into setup
    server.send(200, "application/json", "{\"ok\":1}");
    rebootAt = millis() + 1000;
  });

  server.on("/update", HTTP_POST,
    []() {
      server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK, rebooting");
      if (!Update.hasError()) rebootAt = millis() + 800;
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
      else if (up.status == UPLOAD_FILE_WRITE) Update.write(up.buf, up.currentSize);
      else if (up.status == UPLOAD_FILE_END) Update.end(true);
    });

  server.begin();
  begun_ = true;
}

void WebUi::loop() {
  if (!begun_) return;
  server.handleClient();
  if (rebootAt && millis() > rebootAt) ESP.restart();
}
