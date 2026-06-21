#pragma once
class Config;
class FirelabsCore;

// Device config web UI (online): webhook + power settings + OTA. WiFi onboarding
// is the FireLabs core's job, not this.
namespace WebUi {
void begin(Config& cfg, FirelabsCore& core);
void loop();
}
