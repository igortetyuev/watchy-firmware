#include "Watchy_7_SEG.h"
#include "settings.h"
#include <Wire.h>
#include <TimeLib.h>

Watchy7SEG watchy(settings);

void setup() {
  // Boot banner: prints hardware revision, RTC type, battery and current time
  // over serial on every wake. Safe to delete for zero serial output / minimal
  // wake time.
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(200);
  watchy.RTC.init();
  uint8_t rev = watchy.getBoardRevision();
  watchy.RTC.read(watchy.currentTime);
  Serial.printf("BOOT rev=%d.%d rtc=%s batt=%.2fV time=%04d-%02d-%02d %02d:%02d:%02d\n",
                rev / 10, rev % 10,
                watchy.RTC.rtcType == 1 ? "DS3231"
                : (watchy.RTC.rtcType == 2 ? "PCF8563" : "NONE"),
                watchy.getBatteryVoltage(),
                tmYearToCalendar(watchy.currentTime.Year), watchy.currentTime.Month,
                watchy.currentTime.Day, watchy.currentTime.Hour,
                watchy.currentTime.Minute, watchy.currentTime.Second);

  watchy.init();
}

void loop() {}
