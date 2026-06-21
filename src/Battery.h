#pragma once
#include <Arduino.h>

// 18650 fuel gauge off the ADC pin (hardware /2 divider). The board has no fuel
// gauge IC, so this is a voltage-to-percent estimate: 3.0 V empty, 4.2 V full.
namespace Battery {
struct Reading {
  float volts;
  int percent;
};
Reading read();
}
