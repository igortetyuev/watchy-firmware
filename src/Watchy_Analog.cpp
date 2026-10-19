#include "Watchy_7_SEG.h"
#include <math.h>

#define CX 100
#define CY 100
#define RADIUS 92

static void polar(float angleDeg, float r, int16_t &x, int16_t &y) {
  float a = angleDeg * M_PI / 180.0f;
  x = CX + (int16_t)roundf(r * sinf(a));
  y = CY - (int16_t)roundf(r * cosf(a));
}

static void tick(uint8_t m, float rOuter, float len, uint8_t width) {
  float a = m * 6.0f * M_PI / 180.0f;
  float dx = sinf(a), dy = -cosf(a);
  int16_t x1 = CX + (int16_t)roundf(rOuter * dx);
  int16_t y1 = CY + (int16_t)roundf(rOuter * dy);
  int16_t x2 = CX + (int16_t)roundf((rOuter - len) * dx);
  int16_t y2 = CY + (int16_t)roundf((rOuter - len) * dy);
  if (width <= 1) {
    Watchy::display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
  } else {
    // perpendicular unit vector
    float nx = -dy, ny = dx;
    int16_t ax = (int16_t)roundf(nx), ay = (int16_t)roundf(ny);
    Watchy::display.fillTriangle(x1 + ax, y1 + ay, x1 - ax, y1 - ay, x2 + ax, y2 + ay,
                         GxEPD_BLACK);
    Watchy::display.fillTriangle(x1 - ax, y1 - ay, x2 + ax, y2 + ay, x2 - ax, y2 - ay,
                         GxEPD_BLACK);
  }
}

static void label(int16_t x, int16_t y, const char *text) {
  int16_t x1, y1;
  uint16_t w, h;
  Watchy::display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  Watchy::display.setCursor(x - w / 2 - x1, y - h / 2 - y1);
  Watchy::display.print(text);
}

static void hand(float angleDeg, float length) {
  float a = angleDeg * M_PI / 180.0f;
  float dx = sinf(a), dy = -cosf(a);
  float nx = -dy, ny = dx;
  int16_t ax = (int16_t)roundf(nx), ay = (int16_t)roundf(ny);
  int16_t tx = CX + (int16_t)roundf(length * dx);
  int16_t ty = CY + (int16_t)roundf(length * dy);
  Watchy::display.fillTriangle(CX + ax, CY + ay, CX - ax, CY - ay, tx + ax, ty + ay,
                       GxEPD_BLACK);
  Watchy::display.fillTriangle(CX - ax, CY - ay, tx + ax, ty + ay, tx - ax, ty - ay,
                       GxEPD_BLACK);
}

static void drawBitmapHalf(int16_t x, int16_t y, const unsigned char *bmp,
                           int16_t w, int16_t h) {
  int16_t stride = (w + 7) / 8;
  for (int16_t dy = 0; dy < (h + 1) / 2; dy++) {
    for (int16_t dx = 0; dx < (w + 1) / 2; dx++) {
      bool set = false;
      for (int16_t sy = 2 * dy; sy <= 2 * dy + 1 && sy < h; sy++) {
        for (int16_t sx = 2 * dx; sx <= 2 * dx + 1 && sx < w; sx++) {
          if (pgm_read_byte(&bmp[sy * stride + sx / 8]) &
              (0x80 >> (sx % 8))) {
            set = true;
          }
        }
      }
      if (set) {
        Watchy::display.drawPixel(x + dx, y + dy, GxEPD_BLACK);
      }
    }
  }
}

void Watchy7SEG::drawAnalogFace(uint8_t style) {
  static const char *roman[] = {"I",  "II", "III", "IV",  "V",  "VI",
                                "VII", "VIII", "IX", "X", "XI", "XII"};
  char buf[12];

  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);

  switch (style) {
  case 1:
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 15 == 0) {
        tick(m, RADIUS, 16, 3);
      } else if (m % 5 == 0) {
        tick(m, RADIUS, 12, 3);
      } else {
        tick(m, RADIUS, 6, 1);
      }
    }
    break;
  case 2:
  case 9:
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 15 == 0) {
        tick(m, RADIUS, 16, 3);
      } else if (m % 5 == 0) {
        tick(m, RADIUS, 12, 3);
      } else {
        tick(m, RADIUS, 6, 1);
      }
    }
    if (style == 2) {
      display.setFont(&FreeSans12pt7b);
      for (uint8_t h = 1; h <= 12; h++) {
        int16_t x, y;
        polar(h * 30.0f, RADIUS - 26, x, y);
        snprintf(buf, sizeof buf, "%d", h);
        label(x, y, buf);
      }
    } else {
      for (uint8_t h = 1; h <= 12; h++) {
        int16_t x, y;
        bool quarter = (h % 3 == 0);
        polar(h * 30.0f, quarter ? RADIUS - 30 : RADIUS - 22, x, y);
        display.setFont(quarter ? &FreeSansBold18pt7b : &FreeSansBold9pt7b);
        snprintf(buf, sizeof buf, "%d", h);
        label(x, y, buf);
      }
    }
    break;
  case 3:
    display.drawCircle(CX, CY, RADIUS, GxEPD_BLACK);
    for (uint8_t m = 0; m < 60; m += 5) {
      tick(m, RADIUS, 10, 3);
    }
    display.setFont(&FreeSansBold12pt7b);
    for (uint8_t h = 1; h <= 12; h++) {
      int16_t x, y;
      polar(h * 30.0f, RADIUS - 26, x, y);
      snprintf(buf, sizeof buf, "%d", h);
      label(x, y, buf);
    }
    break;
  case 4:
    display.drawCircle(CX, CY, RADIUS, GxEPD_BLACK);
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 5 == 0) {
        tick(m, RADIUS, 10, 3);
      } else {
        tick(m, RADIUS, 5, 1);
      }
    }
    display.setFont(&FreeSans12pt7b);
    for (uint8_t h = 1; h <= 12; h++) {
      int16_t x, y;
      polar(h * 30.0f, RADIUS - 26, x, y);
      snprintf(buf, sizeof buf, "%d", h);
      label(x, y, buf);
    }
    break;
  case 5:
    display.drawCircle(CX, CY, RADIUS, GxEPD_BLACK);
    display.drawCircle(CX, CY, RADIUS - 7, GxEPD_BLACK);
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 5 == 0) {
        tick(m, RADIUS, 7, 3);
      } else {
        tick(m, RADIUS, 7, 1);
      }
    }
    display.setFont(&FreeSerif12pt7b);
    for (uint8_t h = 1; h <= 12; h++) {
      int16_t x, y;
      polar(h * 30.0f, RADIUS - 24, x, y);
      label(x, y, roman[h - 1]);
    }
    break;
  case 6:
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 5 == 0) {
        tick(m, RADIUS, 9, 1);
      } else {
        tick(m, RADIUS, 4, 1);
      }
    }
    display.setFont(&FreeSerif12pt7b);
    for (uint8_t h = 1; h <= 12; h++) {
      int16_t x, y;
      polar(h * 30.0f, RADIUS - 22, x, y);
      label(x, y, roman[h - 1]);
    }
    break;
  case 7:
    display.setFont(&FreeSerif12pt7b);
    for (uint8_t h = 1; h <= 12; h++) {
      int16_t x, y;
      polar(h * 30.0f, RADIUS - 18, x, y);
      snprintf(buf, sizeof buf, "%d", h);
      label(x, y, buf);
    }
    break;
  case 8:
    for (uint8_t m = 0; m < 60; m++) {
      if (m % 5 == 0) {
        tick(m, RADIUS, m % 15 == 0 ? 14 : 10, 3);
      } else {
        int16_t x, y;
        polar(m * 6.0f, RADIUS - 3, x, y);
        display.fillCircle(x, y, 2, GxEPD_BLACK);
      }
    }
    break;
  }

  // date under the center, drawn before the hands
  display.setFont(&Cyrillic9pt7b);
  snprintf(buf, sizeof buf, "%02d.%02d.%d", currentTime.Day, currentTime.Month,
           tmYearToCalendar(currentTime.Year));
  label(100, 125, buf);

  hand(((currentTime.Hour % 12) + currentTime.Minute / 60.0f) * 30.0f, 64);
  hand(currentTime.Minute * 6.0f, 83);
  if (style == 8) {
    display.drawCircle(CX, CY, 4, GxEPD_BLACK);
  } else {
    display.fillCircle(CX, CY, 4, GxEPD_BLACK);
  }

  drawBitmapHalf(178, 3, battery, 37, 21);
  int8_t level = batteryLevel();
  for (int8_t i = 0; i < level; i++) {
    display.fillRect(178 + 3 + i * 4, 3 + 3, 3, 5, GxEPD_BLACK);
  }
  drawBitmapHalf(3, 3, WIFI_CONFIGURED ? wifi : wifioff, 26, 18);
}
