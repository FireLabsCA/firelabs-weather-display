#include "Display.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Pins.h"
#include <math.h>

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

// ---------------- weather layout ----------------

static void textAt(int x, int baseline, const GFXfont* f, const String& s) {
  epd.setFont(f);
  epd.setTextColor(GxEPD_BLACK);
  epd.setCursor(x, baseline);
  epd.print(s);
}

static void textCenter(int cx, int baseline, const GFXfont* f, const String& s) {
  epd.setFont(f);
  int16_t bx, by; uint16_t bw, bh;
  epd.getTextBounds(s, 0, baseline, &bx, &by, &bw, &bh);
  epd.setCursor(cx - bw / 2 - bx, baseline);
  epd.print(s);
}

static void textRight(int xRight, int baseline, const GFXfont* f, const String& s) {
  epd.setFont(f);
  int16_t bx, by; uint16_t bw, bh;
  epd.getTextBounds(s, 0, baseline, &bx, &by, &bw, &bh);
  epd.setCursor(xRight - bw - bx, baseline);
  epd.print(s);
}

// Built-in 5x7 font, left-aligned at (x, top). The default font is positioned
// from the top-left, not a baseline. Used for tiny labels that must stay compact.
static void tinyAt(int x, int top, const String& s) {
  epd.setFont(nullptr);
  epd.setTextSize(1);
  epd.setCursor(x, top);
  epd.print(s);
}

static String fmtNum(const WxValue& v, int dec) {
  return v.has ? String(v.v, dec) : String("--");
}

// A temperature with a small degree ring; "--" when unmapped (no ring).
static void drawTemp(int x, int baseline, const GFXfont* f, const WxValue& val, int ringR) {
  epd.setFont(f);
  epd.setTextColor(GxEPD_BLACK);
  String s = val.has ? String((int)lroundf(val.v)) : "--";
  int16_t bx, by; uint16_t bw, bh;
  epd.getTextBounds(s, x, baseline, &bx, &by, &bw, &bh);
  epd.setCursor(x, baseline);
  epd.print(s);
  if (val.has && ringR > 0) {
    int rx = x + bw + ringR + 3;
    int ry = by + ringR;
    epd.drawCircle(rx, ry, ringR, GxEPD_BLACK);
    if (ringR >= 4) epd.drawCircle(rx, ry, ringR - 1, GxEPD_BLACK);
  }
}

static void drawTempCenter(int cx, int baseline, const GFXfont* f, const WxValue& val, int ringR) {
  epd.setFont(f);
  String s = val.has ? String((int)lroundf(val.v)) : "--";
  int16_t bx, by; uint16_t bw, bh;
  epd.getTextBounds(s, 0, baseline, &bx, &by, &bw, &bh);
  int total = bw + (val.has ? 2 * ringR + 3 : 0);
  drawTemp(cx - total / 2 - bx, baseline, f, val, ringR);
}

static void drawBattery(int x, int y, int pct) {
  const int w = 26, h = 13;
  epd.drawRect(x, y, w, h, GxEPD_BLACK);
  epd.fillRect(x + w, y + 4, 2, h - 8, GxEPD_BLACK);  // terminal nub
  int fw = (w - 4) * constrain(pct, 0, 100) / 100;
  epd.fillRect(x + 2, y + 2, fw, h - 4, GxEPD_BLACK);
}

// --- procedural weather glyphs (no font/bitmap embedding) ---

// The panel is 1-bit black/white (no hardware grayscale), so "grey" is ordered
// dithering. A 4x4 Bayer matrix gives ~16 levels: a pixel is black when its
// matrix threshold is below the requested level, so level 4 ~ light grey, 8 ~
// 50%, 12 ~ dark. Shading a shape across levels gives it depth.
static const uint8_t BAYER4[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static inline bool bayerOn(int x, int y, int level) {
  return BAYER4[x & 3][y & 3] < level;
}

static void greyDisc(int cx, int cy, int r, int level) {
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++)
      if (x * x + y * y <= r * r && bayerOn(cx + x, cy + y, level))
        epd.drawPixel(cx + x, cy + y, GxEPD_BLACK);
}

static void greyRect(int x0, int y0, int w, int h, int level) {
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (bayerOn(x0 + x, y0 + y, level))
        epd.drawPixel(x0 + x, y0 + y, GxEPD_BLACK);
}

static void glyphSun(int cx, int cy, int r) {
  epd.fillCircle(cx, cy, r * 0.55, GxEPD_BLACK);
  for (int a = 0; a < 360; a += 45) {
    float rad = a * PI / 180.0f;
    epd.drawLine(cx + cos(rad) * r * 0.8, cy + sin(rad) * r * 0.8,
                 cx + cos(rad) * r * 1.15, cy + sin(rad) * r * 1.15, GxEPD_BLACK);
  }
}

// A shaded cloud: a light crown, a medium body, and a darker underside, so it
// reads with depth instead of a flat tone. A thin solid base anchors the bottom.
static void glyphCloud(int cx, int cy, int r) {
  greyDisc(cx - r * 0.05, cy - r * 0.45, r * 0.55, 5);  // crown, light
  greyDisc(cx - r * 0.5, cy, r * 0.5, 8);               // left lobe, medium
  greyDisc(cx + r * 0.45, cy, r * 0.55, 8);             // right lobe, medium
  greyRect(cx - r * 0.9, cy, r * 1.8, r * 0.5, 8);      // body, medium
  greyRect(cx - r * 0.9, cy + r * 0.3, r * 1.8, r * 0.2, 13);  // underside, dark
  epd.drawFastHLine(cx - r * 0.9, cy + r * 0.5, r * 1.8, GxEPD_BLACK);  // base edge
}

static void glyphRain(int cx, int cy, int r) {
  glyphCloud(cx, cy - r * 0.2, r);
  for (int i = -1; i <= 1; i++) {
    int x = cx + i * r * 0.45;
    epd.drawLine(x, cy + r * 0.5, x - r * 0.15, cy + r * 0.95, GxEPD_BLACK);
  }
}

static void glyphSnow(int cx, int cy, int r) {
  glyphCloud(cx, cy - r * 0.2, r);
  for (int i = -1; i <= 1; i++)
    epd.fillCircle(cx + i * r * 0.45, cy + r * 0.7, 2, GxEPD_BLACK);
}

static void glyphStorm(int cx, int cy, int r) {
  glyphCloud(cx, cy - r * 0.2, r);
  epd.fillTriangle(cx - 2, cy + r * 0.4, cx + 6, cy + r * 0.4,
                   cx - 4, cy + r * 1.0, GxEPD_BLACK);
  epd.fillTriangle(cx + 6, cy + r * 0.4, cx + 2, cy + r * 0.75,
                   cx + 9, cy + r * 0.55, GxEPD_BLACK);
}

static void glyphFog(int cx, int cy, int r) {
  for (int i = -1; i <= 2; i++)
    epd.drawLine(cx - r, cy + i * 6, cx + r, cy + i * 6, GxEPD_BLACK);
}

static void drawWx(const String& cond, int cx, int cy, int r) {
  String c = cond;
  c.toLowerCase();
  bool rain = c.indexOf("rain") >= 0 || c.indexOf("pour") >= 0 || c.indexOf("drizzl") >= 0;
  bool snow = c.indexOf("snow") >= 0 || c.indexOf("hail") >= 0;
  bool storm = c.indexOf("light") >= 0 || c.indexOf("thunder") >= 0;
  bool fog = c.indexOf("fog") >= 0 || c.indexOf("mist") >= 0 || c.indexOf("haz") >= 0;
  bool cloud = c.indexOf("cloud") >= 0 || c.indexOf("overcast") >= 0;
  bool sun = c.indexOf("sun") >= 0 || c.indexOf("clear") >= 0;
  bool partly = c.indexOf("partly") >= 0;

  if (storm) glyphStorm(cx, cy, r);
  else if (snow) glyphSnow(cx, cy, r);
  else if (rain) glyphRain(cx, cy, r);
  else if (fog) glyphFog(cx, cy, r);
  else if (partly) { glyphSun(cx - r * 0.4, cy - r * 0.4, r * 0.6); glyphCloud(cx + r * 0.1, cy + r * 0.1, r * 0.85); }
  else if (cloud) glyphCloud(cx, cy, r);
  else if (sun) glyphSun(cx, cy, r);
  else glyphCloud(cx, cy, r);
}

static String hhmm(const String& iso) {
  int t = iso.indexOf('T');
  if (t < 0 || t + 6 > (int)iso.length()) return "";
  return iso.substring(t + 1, t + 6);  // "HH:MM"
}

// HA condition codes are single tokens like "partlycloudy"; map them to readable
// labels. Anything unknown falls back to title-casing on separators.
static String conditionLabel(const String& cond) {
  if (cond.isEmpty()) return "--";
  if (cond == "partlycloudy") return "Partly Cloudy";
  if (cond == "clear-night") return "Clear";
  if (cond == "cloudy") return "Cloudy";
  if (cond == "fog") return "Fog";
  if (cond == "hail") return "Hail";
  if (cond == "lightning") return "Thunder";
  if (cond == "lightning-rainy") return "Thunderstorms";
  if (cond == "pouring") return "Pouring";
  if (cond == "rainy") return "Rain";
  if (cond == "snowy") return "Snow";
  if (cond == "snowy-rainy") return "Sleet";
  if (cond == "sunny") return "Sunny";
  if (cond == "windy" || cond == "windy-variant") return "Windy";
  if (cond == "exceptional") return "Severe";

  String s = cond;
  s.replace("-", " ");
  s.replace("_", " ");
  bool cap = true;
  for (uint16_t i = 0; i < s.length(); i++) {
    if (cap && isalpha(s[i])) { s.setCharAt(i, toupper(s[i])); cap = false; }
    else if (s[i] == ' ') cap = true;
  }
  return s;
}

void Display::showWeather(const WeatherBundle& wx, int batteryPct) {
  const WxCurrent& c = wx.current;
  epd.setFullWindow();  // full refresh every update: crisp, no ghosting
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);

    // Header: the city fills the band; the last-updated time sits under the line.
    textAt(14, 29, &FreeSansBold18pt7b, wx.location.length() ? wx.location : String("Weather"));
    drawBattery(360, 4, batteryPct);
    textCenter(373, 32, &FreeSans9pt7b, String(batteryPct) + "%");  // % under the icon
    epd.drawLine(0, 37, epd.width(), 37, GxEPD_BLACK);
    String upd = hhmm(wx.updated);
    if (upd.length()) tinyAt(14, 42, "Last Updated " + upd);

    // Current: big temp + condition (left), a large weather glyph (centre), and
    // a high/low/feels column (right).
    drawTemp(16, 112, &FreeSansBold24pt7b, c.temp, 6);
    textAt(18, 142, &FreeSans9pt7b, conditionLabel(c.condition));
    drawWx(c.condition, 200, 90, 40);

    textAt(298, 72, &FreeSansBold9pt7b, "High");
    drawTemp(344, 72, &FreeSansBold9pt7b, c.high, 3);
    textAt(298, 100, &FreeSansBold9pt7b, "Low");
    drawTemp(344, 100, &FreeSansBold9pt7b, c.low, 3);
    textAt(298, 128, &FreeSans9pt7b, "Feels");
    drawTemp(344, 128, &FreeSans9pt7b, c.feels, 3);

    epd.drawLine(0, 152, epd.width(), 152, GxEPD_BLACK);

    // Details: two rows of three compact stats, with units from the HA sensors.
    textCenter(67, 178, &FreeSans9pt7b, "Hum " + fmtNum(c.humidity, 0) + "%");
    textCenter(200, 178, &FreeSans9pt7b, "Wind " + fmtNum(c.wind, 0) + " " + wx.windUnit);
    textCenter(333, 178, &FreeSans9pt7b, "UV " + fmtNum(c.uv, 0));
    textCenter(67, 204, &FreeSans9pt7b, "Gust " + fmtNum(c.gust, 0) + " " + wx.windUnit);
    textCenter(200, 204, &FreeSans9pt7b, "PoP " + fmtNum(c.pop, 0) + "%");
    textCenter(333, 204, &FreeSans9pt7b, "Pcp " + fmtNum(c.precip, 1) + " " + wx.precipUnit);

    epd.drawLine(0, 214, epd.width(), 214, GxEPD_BLACK);

    // Forecast: up to five hourly slots.
    for (int i = 1; i < 5; i++) epd.drawLine(i * 80, 220, i * 80, 298, GxEPD_BLACK);
    for (int i = 0; i < wx.forecastCount && i < 5; i++) {
      const WxForecast& f = wx.forecast[i];
      int cx = 40 + i * 80;
      textCenter(cx, 236, &FreeSansBold9pt7b, f.time.length() ? f.time : "--");
      drawWx(f.cond, cx, 260, 13);
      drawTempCenter(cx, 294, &FreeSans9pt7b, f.temp, 2);
    }
  } while (epd.nextPage());
  epd.hibernate();
}
