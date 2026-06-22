// FireLabs Weather Display.
// Onboarding (wifi, captive portal, OTA, hold-to-reset) is handled by the shared
// FirelabsCore library. This file owns the weather behaviour and the power model:
// the device deep-sleeps between check-ins and wakes on a timer or the button.
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#include <time.h>
#include "Config.h"
#include "Display.h"
#include "WebUi.h"
#include "Weather.h"
#include "CheckIn.h"
#include "Battery.h"
#include "Branding.h"
#include "Pins.h"
#include "FirelabsCore.h"

static const uint16_t SETUP_WINDOW_MIN = 30;        // unconfigured awake window
static const uint32_t OTA_WINDOW_MS = 5UL * 60 * 1000;  // button/force-wake window
static const uint64_t US_PER_MIN = 60ULL * 1000 * 1000;

static Config cfg;
static FirelabsCore core;
static WeatherBundle bundle;

// When staying awake (config prompt or an OTA window), deep-sleep once millis
// passes sleepAtMs; sleepMinutes is how long to sleep then. 0 = not staying awake.
static uint32_t sleepAtMs = 0;
static uint16_t sleepMinutes = 30;

// Survives deep sleep, so a persistent error or the config screen isn't repainted
// every wake (an e-paper refresh we don't need to spend).
RTC_DATA_ATTR static int lastScreen = 0;
enum { SCREEN_NONE = 0, SCREEN_WEATHER, SCREEN_ERROR, SCREEN_CONFIG };

static const char* wakeReason() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER: return "timer";
    case ESP_SLEEP_WAKEUP_EXT0:  return "button";
    default:                     return "boot";
  }
}

static void deepSleep(uint16_t minutes) {
  if (minutes < 1) minutes = 1;
  Serial.printf("[FL-WX] deep sleep %u min\n", minutes);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)minutes * US_PER_MIN);
  esp_sleep_enable_ext0_wakeup(PIN_BUTTON, 0);  // wake on button press (active low)
  esp_deep_sleep_start();                        // never returns
}

static void startServices() {
  MDNS.begin(core.hostname().c_str());
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("firelabs", "tcp", 80);  // HA zeroconf discovery
  MDNS.addServiceTxt("firelabs", "tcp", "model", "WX");
  MDNS.addServiceTxt("firelabs", "tcp", "mac", WiFi.macAddress());
  WebUi::begin(cfg, core);
}

// Keep the ESP32 clock roughly in sync with HA's time (from the bundle), so a
// quiet-hours wake can decide to skip the check-in without first joining wifi.
// Timezone is ignored: we store and read the same local wall-clock, which is all
// the quiet-hours comparison needs.
static void syncClock(const String& updated) {
  struct tm tm = {};
  int y, mo, d, h, mi, s;
  if (sscanf(updated.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6) {
    tm.tm_year = y - 1900; tm.tm_mon = mo - 1; tm.tm_mday = d;
    tm.tm_hour = h; tm.tm_min = mi; tm.tm_sec = s;
    struct timeval tv = {mktime(&tm), 0};
    settimeofday(&tv, nullptr);
  }
}

static bool inQuietHours(int hour, int qs, int qe) {
  if (qs == qe) return false;
  if (qs < qe) return hour >= qs && hour < qe;
  return hour >= qs || hour < qe;  // window wraps midnight
}

static uint16_t minutesUntilHour(int targetHour) {
  time_t now = time(nullptr);
  struct tm* lt = localtime(&now);
  int diff = targetHour * 60 - (lt->tm_hour * 60 + lt->tm_min);
  if (diff <= 0) diff += 24 * 60;
  return diff;
}

// Persist the schedule HA sent, for the next wake's quiet-hours check. Only write
// flash when something actually changed.
static void persistSchedule() {
  if (cfg.sleepMin == bundle.settings.sleepMin &&
      cfg.quietStart == bundle.settings.quietStart &&
      cfg.quietEnd == bundle.settings.quietEnd) {
    return;
  }
  cfg.sleepMin = bundle.settings.sleepMin;
  cfg.quietStart = bundle.settings.quietStart;
  cfg.quietEnd = bundle.settings.quietEnd;
  cfg.save();
}

static void checkInRenderSleep(const char* wake, bool otaWindow) {
  Battery::Reading b = Battery::read();
  CheckIn::Telemetry tel{b.percent, b.volts, FL_FW_VERSION, wake};
  int code = 0;
  Serial.printf("[FL-WX] check-in (%s) batt=%d%%\n", wake, b.percent);

  // Retry once: a TLS handshake can flake on a cold wake, and a wasted refresh
  // (or a 30-minute gap) is worse than a second attempt.
  bool ok = CheckIn::run(cfg.webhookUrl, tel, bundle, code);
  if (!ok) {
    delay(1000);
    ok = CheckIn::run(cfg.webhookUrl, tel, bundle, code);
  }

  if (ok) {
    Display::showWeather(bundle, b.percent);
    lastScreen = SCREEN_WEATHER;
    syncClock(bundle.updated);
    persistSchedule();
  } else {
    Serial.printf("[FL-WX] check-in failed http=%d\n", code);
    if (lastScreen != SCREEN_ERROR) {
      Display::showMessage("Can't reach Home Assistant",
                           "Check-in failed (" + String(code) + "). Retrying.");
      lastScreen = SCREEN_ERROR;
    }
  }

  // A button tap or a power-on always opens an OTA window, even on a failed
  // check-in, so the device can always be recovered and flashed. A good bundle's
  // force-wake switch does too.
  if (otaWindow || (ok && bundle.settings.forceWake)) {
    startServices();
    sleepAtMs = millis() + OTA_WINDOW_MS;
    sleepMinutes = ok ? bundle.settings.sleepMin : cfg.sleepMin;
    Serial.println("[FL-WX] OTA window open (5 min)");
    return;
  }
  deepSleep(ok ? bundle.settings.sleepMin : cfg.sleepMin);
}

void setup() {
  Serial.begin(115200);
  delay(80);
  const char* wake = wakeReason();
  Serial.printf("\n[FL-WX] wake=%s\n", wake);

  Display::begin();
  core.apPrefix = "FireLabs WX";
  core.hostPrefix = "fl-wx";
  core.begin();
  core.enableButton(0);  // GPIO0: hold to factory reset (after a tap wakes it)
  core.onHold([]() {
    Serial.println("[FL-WX] hold -> factory reset");
    cfg.clear();
    core.factoryReset();
  });
  cfg.load();

  // First boot, no wifi: setup portal, stay awake. After the window with no one
  // provisioning, deep-sleep; a later wake retries.
  if (!core.hasWifi()) {
    Serial.println("[FL-WX] no wifi, setup portal");
    Display::showSetup(core.apSsid());
    lastScreen = SCREEN_CONFIG;
    core.onTimeout([]() { deepSleep(SETUP_WINDOW_MIN); });
    core.startSetupPortal((uint32_t)SETUP_WINDOW_MIN * 60 * 1000);
    return;
  }

  // Quiet hours: if the clock has been set on a prior check-in and we're inside
  // the window, skip the check-in (and its wifi power) and sleep until it ends.
  // Only on a timer wake -- a button always services the user.
  if (strcmp(wake, "timer") == 0 && cfg.quietStart != cfg.quietEnd) {
    time_t now = time(nullptr);
    if (now > 1700000000) {  // clock was set at least once
      struct tm* lt = localtime(&now);
      if (inQuietHours(lt->tm_hour, cfg.quietStart, cfg.quietEnd)) {
        Serial.println("[FL-WX] quiet hours, back to sleep");
        deepSleep(minutesUntilHour(cfg.quietEnd));
      }
    }
  }

  // Connect silently -- no "connecting" splash, so a routine wake only refreshes
  // the panel once, with the weather itself.
  if (!core.connect(20000)) {
    Serial.println("[FL-WX] wifi failed");
    if (lastScreen != SCREEN_ERROR) {
      Display::showMessage("No Wi-Fi", "Couldn't join " + core.wifiSsid + ".");
      lastScreen = SCREEN_ERROR;
    }
    deepSleep(cfg.sleepMin);
  }
  Serial.printf("[FL-WX] online %s\n", WiFi.localIP().toString().c_str());

  // On the network but no webhook yet: hold awake on the config prompt so the
  // user can set it; sleep after the setup window.
  if (!cfg.isConfigured()) {
    Serial.println("[FL-WX] not configured, config prompt");
    startServices();
    if (lastScreen != SCREEN_CONFIG) {
      Display::showConfigPrompt(core.hostname(), WiFi.localIP().toString());
      lastScreen = SCREEN_CONFIG;
    }
    sleepAtMs = millis() + (uint32_t)SETUP_WINDOW_MIN * 60 * 1000;
    sleepMinutes = cfg.sleepMin;
    return;
  }

  // Configured: check in, render, sleep. Anything but a plain timer wake (a
  // button tap or a power-on/reset) opens an OTA window, so a power cycle is
  // always a reliable way to catch the device for flashing.
  checkInRenderSleep(wake, strcmp(wake, "timer") != 0);
}

void loop() {
  core.loop();
  WebUi::loop();
  if (sleepAtMs && millis() > sleepAtMs) {
    deepSleep(sleepMinutes);
  }
  delay(5);
}
