#ifndef CHRISTMAS_CLOCK_CLOCK_INFO_H
#define CHRISTMAS_CLOCK_CLOCK_INFO_H

#include <stdint.h>
#include <AceTime.h>
#include "config.h"

struct ClockInfo {
  /** 12 Hour mode. 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 24 Hour mode. 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

  /** display mode */
  Mode mode = Mode::kUnknown;

  /** Blinking info should be shown. Should be toggled every 0.5 sec. */
  bool blinkShowState = false;

  /** Blinking should be suppressed. e.g. when RepeatPress is active. */
  bool suppressBlink = false;

  /** 12/24 mode */
  uint8_t hourMode = kTwelve;

  /** Brightness, 1 - 7 for Tm1636Display; 1 - SUBFIELDS for ScanningDisplay. */
  uint8_t brightness = 1;

  /** Desired timeZoneData. */
  ace_time::TimeZoneData timeZoneData;

  /** DateTime from the TimeKeeper. */
  ace_time::ZonedDateTime dateTime;
};

inline bool operator==(const ClockInfo& a, const ClockInfo& b) {
  return a.mode == b.mode
      && a.suppressBlink == b.suppressBlink
      && a.blinkShowState == b.blinkShowState
      && a.hourMode == b.hourMode
      && a.brightness == b.brightness
      && a.timeZoneData == b.timeZoneData
      && a.dateTime == b.dateTime;
}

inline bool operator!=(const ClockInfo& a, const ClockInfo& b) {
  return ! (a == b);
}

#endif
