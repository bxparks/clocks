#ifndef MULTI_ZONE_CLOCK_CLOCK_INFO_H
#define MULTI_ZONE_CLOCK_CLOCK_INFO_H

#include <AceTime.h>
#include "config.h" // DISPLAY_TYPE

/**
 * Information about the state of the clock. Many of these fields are
 * user-configurable settings which make sense to be stored in EEPROM.
 */
struct ClockInfo {
  /** 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

  /** display mode */
  Mode mode = Mode::kUnknown;

  /** Blinking info should be shown. Should be toggled every 0.5 sec. */
  bool blinkShowState = false;

  /** Blinking should be suppressed. e.g. when RepeatPress is active. */
  bool suppressBlink = false;

  /** 12/24 mode */
  uint8_t hourMode;

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
  static uint8_t const kInvertDisplayAuto = 2;

  /** Invert display mode. Only for OLED. */
  uint8_t invertDisplay;

#endif

  /** The desired timezones of the clock. */
  ace_time::TimeZoneData zones[NUM_TIME_ZONES];

  /**
   * Current time. It might be possible to track just the epochSeconds instead
   * of converting it to a ZonedDateTime at each iteration. But I think it then
   * becomes far more difficult to allow the user to change the various
   * date/time fields (e.g. month, day, year, hour, etc).
   */
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
};

inline bool operator==(const ClockInfo& a, const ClockInfo& b) {
  // Fields most likely to change are compared earlier than later.
  bool isEqual = a.mode == b.mode
    && a.blinkShowState == b.blinkShowState
    && a.suppressBlink == b.suppressBlink
    && a.dateTime == b.dateTime
    && a.prevSync == b.prevSync
    && a.nextSync == b.nextSync
    && a.clockSkew == b.clockSkew
    && a.syncStatusCode == b.syncStatusCode
    && a.hourMode == b.hourMode
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    && a.backlightLevel == b.backlightLevel
    && a.contrast == b.contrast
    && a.bias == b.bias
  #else
    && a.contrastLevel == b.contrastLevel
    && a.invertDisplay == b.invertDisplay
  #endif
    ;
  if (! isEqual) return false;

  // Check the timezones using a loop because of NUM_TIME_ZONES.
  for (uint8_t i = 0; i < NUM_TIME_ZONES; ++i) {
    if (a.zones[i] != b.zones[i]) return false;
  }
  return true;
}

inline bool operator!=(const ClockInfo& a, const ClockInfo& b) {
  return ! (a == b);
}

#endif
