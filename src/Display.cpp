#include "Display.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Pins.h"

static GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> epd(
    GxEPD2_420_GDEY042T81(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

static void header() {
  epd.drawRect(0, 0, epd.width(), epd.height(), GxEPD_BLACK);
  epd.setFont(&FreeSansBold12pt7b);
  epd.setTextColor(GxEPD_BLACK);
  epd.setCursor(18, 38);
  epd.print("FireLabs Weather Display");
  epd.drawLine(14, 52, epd.width() - 14, 52, GxEPD_BLACK);
}

void Display::begin() {
  SPI.begin(PIN_EPD_CLK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
  epd.init(115200, true, 2, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  epd.setRotation(0);
}

void Display::showSetup(const String& apSsid) {
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    header();
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(18, 100);
    epd.print("Set up");
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(18, 140);
    epd.print("1. On your phone, join wifi:");
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(40, 165);
    epd.print(apSsid);
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(18, 205);
    epd.print("2. Open a browser to:");
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(40, 230);
    epd.print("http://192.168.4.1");
  } while (epd.nextPage());
  epd.hibernate();
}

void Display::showConfigPrompt(const String& host, const String& ip) {
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    header();
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(18, 100);
    epd.print("Almost there");
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(18, 140);
    epd.print("On your network. Open the web UI");
    epd.setCursor(18, 162);
    epd.print("and set the Home Assistant webhook:");
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(40, 200);
    epd.print("http://" + host + ".local");
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(40, 224);
    epd.print("or http://" + ip);
  } while (epd.nextPage());
  epd.hibernate();
}

void Display::showMessage(const String& title, const String& line) {
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    header();
    epd.setFont(&FreeSansBold9pt7b);
    epd.setCursor(18, 110);
    epd.print(title);
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(18, 145);
    epd.print(line);
  } while (epd.nextPage());
  epd.hibernate();
}
