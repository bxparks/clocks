#ifndef WORLD_CLOCK_CLOCK_INFO_H
#define WORLD_CLOCK_CLOCK_INFO_H

#include <AceTime.h>
#include "config.h"

/**
 * Data that describes the clock of a single time zone. The uint8_t and bool
 * fields are grouped together to save memory on 32-bit processors.
 */
struct ClockInfo {
  /** Size of the clock name buffer, including '\0'. */
  static uint8_t const kNameSize = 5;

  /** 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

  /** Invert display off */
  static uint8_t const kInvertDisplayOff = 0;

  /** Invert display on. */
  static uint8_t const kInvertDisplayOn = 1;

  /** Invert display automatically every minute. */
  static uint8_t const kInvertDisplayMinutely = 2;

  /** Invert display automatically every hour. */
  static uint8_t const kInvertDisplayHourly = 3;

  /** Invert display automatically half-daily. */
  static uint8_t const kInvertDisplayDaily = 4;

  /** display mode */
  Mode mode = Mode::kUnknown;

  /** true if blinking info should be shown */
  bool blinkShowState = false;

  /** Blinking should be suppressed. e.g. when RepeatPress is active. */
  bool suppressBlink = false;

  /** Hour mode, 12H or 24H. */
  uint8_t hourMode = kTwelve;

  /** Blink the colon in HH:MM. */
  bool blinkingColon = false;

  /**
   * Contrast level for OLED dislay, [0, 9] -> [25, 255]. Essentially brightness
   * because the background is black.
   */
  uint8_t contrastLevel = 5;

  /** Desired display inversion mode. [0-4] */
  uint8_t invertDisplay = 0;

  /**
   * Actual inversion mode, derived from clockInfo.
   *
   *  * 0: white on black
   *  * 1: black on white
   */
  uint8_t invertState = 0;

  /** The desired timezone of the clock. */
  ace_time::TimeZone timeZone;

  /**
   * Current time. Either from the epochSeconds clock, or from the manual
   * user's choice of the date/time components.
   */
  ace_time::ZonedDateTime dateTime;

  /** Name of this clock, e.g. City or Time Zone ID */
  const char* name = nullptr;
};


inline bool operator==(const ClockInfo& a, const ClockInfo& b) {
  return
      a.blinkShowState == b.blinkShowState
      && a.dateTime == b.dateTime
      && a.mode == b.mode
      && a.suppressBlink == b.suppressBlink
      && a.invertDisplay == b.invertDisplay
      && a.invertState == b.invertState
      && a.blinkingColon == b.blinkingColon
      && a.hourMode == b.hourMode
      && a.contrastLevel == b.contrastLevel
      && a.timeZone == b.timeZone
      && a.name == b.name;
}

inline bool operator!=(const ClockInfo& a, const ClockInfo& b) {
  return ! (a == b);
}


#endif
