// FireLabs Weather Display.
// Onboarding (wifi, captive portal, OTA, hold-to-reset) is handled by the shared
// FirelabsCore library so every FireLabs device sets up the same way. This file
// owns only the weather-display behaviour: the e-paper screen and (later) the
// webhook check-in, deep sleep, and battery reporting.
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Config.h"
#include "Display.h"
#include "WebUi.h"
#include "Weather.h"
#include "CheckIn.h"
#include "Battery.h"
#include "Branding.h"
#include "FirelabsCore.h"

static const uint32_t FALLBACK_PORTAL_MS = 5UL * 60 * 1000;   // reboot, retry wifi
static const uint32_t SETUP_PORTAL_MS    = 30UL * 60 * 1000;  // fresh first boot

static Config cfg;
static FirelabsCore core;
static WeatherBundle bundle;
static bool weatherMode = false;     // configured and showing weather
static uint32_t refreshMs = 0;       // effective wake interval, set by the bundle

// Check in with HA, then paint the result (or the error). Until deep sleep lands
// (next milestone) this just runs on a millis timer instead of a wake cycle.
static void refresh(const char* wake) {
  Battery::Reading b = Battery::read();
  CheckIn::Telemetry tel{b.percent, b.volts, FL_FW_VERSION, wake};
  int code = 0;
  Serial.printf("[FL-WX] check-in (%s) batt=%d%% %.2fV\n", wake, b.percent, b.volts);
  if (CheckIn::run(cfg.webhookUrl, tel, bundle, code)) {
    Serial.printf("[FL-WX] bundle ok, %u forecast slots\n", bundle.forecastCount);
    Display::showWeather(bundle, b.percent);
    refreshMs = (uint32_t)bundle.settings.sleepMin * 60UL * 1000;
  } else {
    Serial.printf("[FL-WX] check-in failed, http=%d\n", code);
    Display::showMessage("Check-in failed", "HA returned " + String(code));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[FL-WX] boot");

  Display::begin();

  core.apPrefix = "FireLabs WX";
  core.hostPrefix = "fl-wx";
  core.begin();
  core.enableButton(0);                 // GPIO0: hold to factory reset
  core.onHold([]() {
    Serial.println("[FL-WX] hold -> factory reset");
    cfg.clear();
    core.factoryReset();
  });

  cfg.load();

  if (!core.hasWifi()) {
    Serial.println("[FL-WX] no wifi creds, fresh setup portal");
    Display::showSetup(core.apSsid());
    core.startSetupPortal(SETUP_PORTAL_MS);
    return;
  }

  Serial.printf("[FL-WX] connecting to '%s'\n", core.wifiSsid.c_str());
  Display::showMessage("Connecting", "Joining " + core.wifiSsid + "...");
  if (!core.connect(20000)) {
    Serial.println("[FL-WX] wifi failed, fallback portal (reboots in 5 min)");
    Display::showSetup(core.apSsid());
    core.startSetupPortal(FALLBACK_PORTAL_MS);
    return;
  }
  Serial.printf("[FL-WX] online at %s\n", WiFi.localIP().toString().c_str());

  MDNS.begin(core.hostname().c_str());
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("firelabs", "tcp", 80);  // for HA zeroconf discovery
  MDNS.addServiceTxt("firelabs", "tcp", "model", "WX");
  MDNS.addServiceTxt("firelabs", "tcp", "mac", WiFi.macAddress());
  WebUi::begin(cfg, core);

  if (!cfg.isConfigured()) {
    Serial.println("[FL-WX] online but no webhook, showing config prompt");
    Display::showConfigPrompt(core.hostname(), WiFi.localIP().toString());
    return;
  }

  // Configured: first check-in + render. The web UI and OTA stay reachable.
  Serial.println("[FL-WX] configured, first check-in");
  weatherMode = true;
  refresh("boot");
}

void loop() {
  core.loop();
  WebUi::loop();

  if (weatherMode) {
    static uint32_t lastRefresh = 0;
    uint32_t iv = refreshMs ? refreshMs : (uint32_t)cfg.sleepMin * 60UL * 1000;
    if (millis() - lastRefresh > iv) {
      lastRefresh = millis();
      refresh("timer");
    }
  }

  static uint32_t hb = 0;
  if (millis() - hb > 3000) {
    hb = millis();
    Serial.printf("[FL-WX] hb sta=%d ip=%s\n",
                  WiFi.status() == WL_CONNECTED, WiFi.localIP().toString().c_str());
  }
  delay(5);
}
