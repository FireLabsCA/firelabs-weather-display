#include "Display.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold24pt7b.h>
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

static void glyphSun(int cx, int cy, int r) {
  epd.fillCircle(cx, cy, r * 0.55, GxEPD_BLACK);
  for (int a = 0; a < 360; a += 45) {
    float rad = a * PI / 180.0f;
    epd.drawLine(cx + cos(rad) * r * 0.8, cy + sin(rad) * r * 0.8,
                 cx + cos(rad) * r * 1.15, cy + sin(rad) * r * 1.15, GxEPD_BLACK);
  }
}

static void glyphCloud(int cx, int cy, int r) {
  epd.fillCircle(cx - r * 0.5, cy, r * 0.5, GxEPD_BLACK);
  epd.fillCircle(cx + r * 0.45, cy, r * 0.55, GxEPD_BLACK);
  epd.fillCircle(cx - r * 0.05, cy - r * 0.45, r * 0.55, GxEPD_BLACK);
  epd.fillRect(cx - r * 0.9, cy, r * 1.8, r * 0.5, GxEPD_BLACK);
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

static String pretty(const String& cond) {
  if (cond.isEmpty()) return "--";
  String s = cond;
  s.replace("-", " ");
  s.replace("_", " ");
  s.setCharAt(0, toupper(s.charAt(0)));
  return s;
}

void Display::showWeather(const WeatherBundle& wx, int batteryPct) {
  const WxCurrent& c = wx.current;
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);

    // Header: last-updated on the left, battery on the right.
    String upd = hhmm(wx.updated);
    textAt(14, 22, &FreeSans9pt7b, upd.length() ? "Updated " + upd : "FireLabs WX");
    textRight(352, 22, &FreeSansBold9pt7b, String(batteryPct) + "%");
    drawBattery(360, 9, batteryPct);
    epd.drawLine(0, 32, epd.width(), 32, GxEPD_BLACK);

    // Current: big temperature + condition, a glyph, and a high/low/feels column.
    drawTemp(16, 115, &FreeSansBold24pt7b, c.temp, 6);
    textAt(20, 144, &FreeSans9pt7b, pretty(c.condition));
    drawWx(c.condition, 200, 92, 38);

    textAt(296, 72, &FreeSansBold9pt7b, "High");
    drawTemp(344, 72, &FreeSansBold9pt7b, c.high, 3);
    textAt(296, 100, &FreeSansBold9pt7b, "Low");
    drawTemp(344, 100, &FreeSansBold9pt7b, c.low, 3);
    textAt(296, 128, &FreeSans9pt7b, "Feels");
    drawTemp(344, 128, &FreeSans9pt7b, c.feels, 3);

    epd.drawLine(0, 152, epd.width(), 152, GxEPD_BLACK);

    // Details: two rows of three compact stats.
    textCenter(67, 178, &FreeSans9pt7b, "Hum " + fmtNum(c.humidity, 0) + "%");
    textCenter(200, 178, &FreeSans9pt7b, "Wind " + fmtNum(c.wind, 0));
    textCenter(333, 178, &FreeSans9pt7b, "UV " + fmtNum(c.uv, 0));
    textCenter(67, 204, &FreeSans9pt7b, "Gust " + fmtNum(c.gust, 0));
    textCenter(200, 204, &FreeSans9pt7b, "PoP " + fmtNum(c.pop, 0) + "%");
    textCenter(333, 204, &FreeSans9pt7b, "Pcp " + fmtNum(c.precip, 1));

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
