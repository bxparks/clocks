#ifndef ONE_ZONE_CLOCK_CLOCK_INFO_H
#define ONE_ZONE_CLOCK_CLOCK_INFO_H

#include <AceTime.h>
#include "config.h" // DISPLAY_TYPE

/** Information about the clock, mostly independent of rendering. */
struct ClockInfo {
  /** 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

  /** 12/24 mode */
  uint8_t hourMode;

  // Keeping uint8_t grouped together helps reduce storage on 32-bit processors
#if DISPLAY_TYPE == DISPLAY_TYPE_LCD

  /** Backlight level, [0, 9] -> [0, 1023] */
  uint8_t backlightLevel;

  /** Contrast, [0, 127] */
  uint8_t contrast;

  /** Bias level, [0, 7] */
  uint8_t bias;

#else

  /**
   * Contrast level for OLED dislay, [0, 9] -> [25, 255]. Essentially brightness
   * because the background is black.
   */
  uint8_t contrastLevel;

  /** Invert display off */
  static uint8_t const kInvertDisplayOff = 0;

  /** Invert display on. */
  static uint8_t const kInvertDisplayOn = 1;

  /** Invert display auto. */
  static uint8_t const kInvertDisplayMinutely = 2;

  /** Invert display automatically every hour. */
  static uint8_t const kInvertDisplayHourly = 3;

  /** Invert display automatically half-daily. */
  static uint8_t const kInvertDisplayDaily = 4;

  /** Invert display mode. Only for OLED. */
  uint8_t invertDisplay;

#endif

  /** The desired timezone of the clock. */
  ace_time::TimeZoneData timeZoneData;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;

  // Fields related to SYSCLOCK

  /** TimePeriod since last sync. */
  ace_time::TimePeriod prevSync;

  /** TimePeriod until next sync. */
  ace_time::TimePeriod nextSync;

  /** TimePeriod of clock skew. */
  ace_time::TimePeriod clockSkew;

  /** Status code of the most recent sync attempt. */
  uint8_t syncStatusCode;

#if ENABLE_DHT22
  /** Current temperature in Celcius. */
  float temperatureC;

  /** Current humidity. */
  float humidity;
#endif

#if ENABLE_LED_DISPLAY
  /** Enable LED Module or not interactively. */
  bool ledOnOff = true;

  uint8_t ledBrightness = 1;
#endif
};

inline bool operator==(const ClockInfo& a, const ClockInfo& b) {
  // Fields most likely to change are compared earlier than later.
  return a.dateTime == b.dateTime
    && a.prevSync == b.prevSync
    && a.nextSync == b.nextSync
    && a.clockSkew == b.clockSkew
    && a.syncStatusCode == b.syncStatusCode
    && a.hourMode == b.hourMode
  #if ENABLE_DHT22
    && a.temperatureC == b.temperatureC
    && a.humidity == b.humidity
  #endif
  #if ENABLE_LED_DISPLAY
    && a.ledOnOff == b.ledOnOff
    && a.ledBrightness == b.ledBrightness
  #endif
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    && a.backlightLevel == b.backlightLevel
    && a.contrast == b.contrast
    && a.bias == b.bias
  #else
    && a.contrastLevel == b.contrastLevel
    && a.invertDisplay == b.invertDisplay
  #endif
    && a.timeZoneData == b.timeZoneData;
}

inline bool operator!=(const ClockInfo& a, const ClockInfo& b) {
  return ! (a == b);
}

#endif
