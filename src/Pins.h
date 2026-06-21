#pragma once
#include <Arduino.h>

// LilyGo T-Energy-S3 (ESP32-S3) + GoodDisplay GDEY042T81 (4.2", 400x300, SSD1683)
static const int8_t PIN_EPD_CLK  = 12;  // SPI SCK
static const int8_t PIN_EPD_MOSI = 11;  // SPI MOSI
static const int8_t PIN_EPD_CS   = 10;
static const int8_t PIN_EPD_DC   = 9;
static const int8_t PIN_EPD_RST  = 46;
static const int8_t PIN_EPD_BUSY = 13;

static const gpio_num_t PIN_BUTTON = GPIO_NUM_0;  // BOOT: EXT0 wake + factory reset
static const int8_t PIN_BATTERY    = 3;           // ADC, hardware /2 divider
