# FireLabs Weather Display Spec

Battery e-paper weather display on an ESP32-S3. It pulls a curated weather bundle
from Home Assistant on each wake, paints a 4.2-inch e-paper panel, and deep-sleeps.
Unlike the S31, this device consumes data from HA rather than exposing data to it,
so the data flow and the integration's job both run the other direction.

The current build runs ESPHome; this replaces it with FireLabs firmware so the device
shares the S31's setup, web UI, OTA, and button conventions. The ESPHome config you
provided is the reference for the port; its API key, OTA password, and AP password
stay out of this repo.

Author: FireBall1725

## Hardware

LilyGo T-Energy-S3 (ESP32-S3) with an 18650 cell, driving a GoodDisplay
GDEY042T81 panel (4.2 inch, 400x300, black/white, SSD1683 controller).

| Pin | Function |
|---|---|
| GPIO12 | e-paper SPI CLK |
| GPIO11 | e-paper SPI MOSI |
| GPIO10 | e-paper CS |
| GPIO9 | e-paper DC |
| GPIO46 | e-paper RST |
| GPIO13 | e-paper BUSY |
| GPIO0 | button (BOOT); wake source and factory reset |
| GPIO3 | battery voltage ADC (hardware divider, multiply by 2) |

Battery range: 3.0V = 0%, 4.2V = 100%. The 18650 plus deep sleep is what makes a
30-minute refresh cycle last weeks instead of a day, so every milliamp of wake time
counts.

## Reused from the FireLabs standard

This device uses the same out-of-box experience as the S31: a captive-portal setup
wizard, the on-device web UI, web OTA, and the GPIO0 button. That core (config in
LittleFS, wifi provisioning, captive portal, web server, OTA, button state machine)
is currently duplicated per firmware. Once it's running on this second device, the
plan is to pull it into a shared `firelabs-core` library so every FireLabs firmware
links the same setup code instead of copying it. Extracting after the second device,
not the first, means the library's surface is shaped by two real callers rather than
one guess.

## Power model

The device is asleep almost all the time. Once configured (see Setup flow) it runs the
normal cycle; an unconfigured unit stays awake to be set up, but only for a bounded
window so a forgotten one doesn't flatten the cell. In normal mode it wakes, does its
work in a few seconds, and sleeps again.

- **Default cycle:** deep sleep 30 minutes, timer wake, refresh, sleep.
- **Quiet hours 21:00 to 06:00:** no refresh; on a timer wake in this window it
  computes the minutes until 06:00 and sleeps that long in one shot. A button wake
  or a force-wake from HA overrides this.
- **Button wake:** GPIO0 (EXT0, active low) wakes the device into an awake window
  (about 5 minutes) for OTA and web-UI access, then it refreshes and sleeps. This is
  the deliberate window to flash an update or change settings.
- **Force wake:** a control from HA (see the bundle below) keeps the device awake the
  same way without touching the button.

## Button behavior

GPIO0 carries two actions, same split as the S31:

- **Wake / tap:** wakes from deep sleep into the 5-minute awake window for OTA and
  config. Nothing destructive.
- **Hold ~5 seconds:** factory reset, clears config in LittleFS and reboots into the
  setup AP. Same accelerating-LED-style confirmation isn't available (no controllable
  LED here), so the e-paper shows the reset state instead.

## Data transport: webhook check-in

One request per wake does both directions. The device POSTs its own telemetry to a
Home Assistant webhook and gets the weather bundle back in the response, so reporting
the battery costs no extra wake time and needs no broker. The entity mapping lives
entirely in HA, so re-pointing the display at different sensors is a settings change
with no reflash.

- HA side: the integration registers a per-device webhook at `/api/webhook/<id>`.
  On each call the handler records the telemetry from the request body (battery,
  voltage, version, wake reason) into the device's exposed entities, then returns the
  weather bundle built from the mapped source entities. The webhook id is the only
  secret the device stores.
- Device side: on wake the firmware does one `POST http://<ha-host>/api/webhook/<id>`
  with its telemetry, parses the bundle from the response, and paints the panel. No HA
  long-lived token on the device, no entity ids baked into firmware.

Check-in body (device to HA):

```json
{ "battery": 87, "voltage": 3.94, "version": "0.1.0", "wake": "timer" }
```

Response bundle (HA to device; firmware defines the field names, the integration
fills them):

```json
{
  "updated": "2026-06-21T15:04:00",
  "current": {
    "temp": 12.4, "feels_like": 10.1, "condition": "cloudy",
    "humidity": 65, "wind": 15, "gust": 25, "precip": 0.2, "uv": 3,
    "high": 14, "low": 5, "pop": 30
  },
  "forecast": [
    {"time": "3PM", "temp": 13, "cond": "rainy"},
    {"time": "4PM", "temp": 13, "cond": "cloudy"}
  ],
  "settings": { "sleep_min": 30, "quiet_start": 21, "quiet_end": 6, "force_wake": false },
  "ota": { "version": "0.0.0", "url": "" }
}
```

MQTT stays as the failover for setups that don't run this integration: the same
bundle published to a retained topic the device reads on wake. Pick one, the webhook
or MQTT, not both.

## Mass OTA hook

The bundle carries an `ota` block with a target version and a bin URL. When the
device fetches its payload and `ota.version` is newer than `FL_FW_VERSION`, it stays
awake, downloads the bin from `ota.url`, and flashes instead of sleeping. That lets
the integration push a firmware rollout to a fleet of sleepy devices: set the target
version once, and each device updates the next time it wakes, with no per-device
button press. First cut can ship the field and ignore it; the staying-awake-to-update
logic lands when the fleet feature is built.

## Setup flow and the panel

Setup runs in two phases, and the device doesn't sleep until it's fully configured.
The panel carries every instruction, since this board has no LED to blink.

Phase 1, no saved wifi: the FireLabs core starts the `FireLabs WX <mac-suffix>` AP.
The panel shows the AP name (open network) and `http://192.168.4.1`. The captive-portal
wizard takes wifi plus a device name, same as the S31, then reboots onto the network.

Phase 2, on the network but no webhook set: the device stays awake and paints
`Configure at http://fl-wx-<name>.local` plus its IP. You open the web UI and enter the
HA webhook URL and any settings. It holds awake the whole time, so the web UI and OTA
stay reachable.

Once the webhook URL is set and the first check-in succeeds, it switches to normal mode:
deep-sleep cycles waking on the timer or the button. If it's left unconfigured, it holds
awake for about 30 minutes (a settable timeout) and then deep-sleeps, so a forgotten unit
doesn't drain the 18650; the next timer wake or a button press brings the config screen
back.

## Home Assistant integration

This is a new device model in `firelabs-hass`, and it's a different shape than the
S31. The S31 model reads entities from the device; the weather display model mostly
configures the device and serves it data.

- **Options flow with entity pickers:** map HA entities to bundle fields (current
  temp, feels-like, humidity, wind, gust, precip, UV, EC high/low/pop/icon/condition,
  and five forecast-hour entities), plus settings (sleep minutes, quiet hours).
- **Webhook:** registered per config entry, returns the bundle from the mapped
  entities on each GET.
- **Exposed entities, updated from each check-in:** battery percentage, battery
  voltage, firmware version, wake reason, and a last-seen timestamp that also drives
  availability (mark the device unavailable if it misses a couple of cycles). These
  refresh every wake, about every 30 minutes, which is fine for battery monitoring.
- **Force wake:** the one control going the other way (HA to device). It's a switch
  the integration owns; its value rides back in the bundle's `settings.force_wake`,
  and the device honors it on the next wake.

The per-model dispatch already in the integration handles this; `models/weather_display.py`
provides its builders and the options flow.

## Display layout

Port the existing ESPHome display lambda field for field on the
400x300 panel: header with location and battery, last-updated line, current
temperature with a weather glyph and condition, a feels-like / high / low row, a
two-row details grid (humidity, gusts, PoP, wind, precip, UV), and a five-slot hourly
forecast strip. Fonts: Roboto at 56/24/18/14/11 px and the Material Design Icons
weather glyphs, embedded in flash. Rendering uses GxEPD2 for the GDEY042T81.

## Pending decisions

- What counts as "configured": the webhook URL being set, or the first check-in that
  returns a valid bundle.
- Awake-window length on a button wake (5 minutes assumed) before the device refreshes
  and sleeps.
