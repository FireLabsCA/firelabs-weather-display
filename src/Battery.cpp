#include "Battery.h"
#include "Pins.h"

namespace {
const float kEmpty = 3.0f;  // volts at 0%
const float kFull = 4.2f;   // volts at 100%
}

Battery::Reading Battery::read() {
  // Average a handful of samples; the ADC is noisy and this only runs once a wake.
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BATTERY);
  mv /= 16;

  float volts = (mv * 2.0f) / 1000.0f;  // undo the /2 divider
  float pct = (volts - kEmpty) / (kFull - kEmpty) * 100.0f;
  return {volts, (int)constrain(pct, 0.0f, 100.0f)};
}
