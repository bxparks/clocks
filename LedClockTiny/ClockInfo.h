#ifndef LED_CLOCK_TINY_CLOCK_INFO_H
#define LED_CLOCK_TINY_CLOCK_INFO_H

#include <stdint.h>
#include <AceTime.h>
#include "config.h"

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
  uint8_t hourMode = kTwelve;

  /** Brightness, 0 - 7 for Tm1636Display. */
  uint8_t brightness = 1;

  /** True if DST being observed. */
  bool isDst;

  /** DateTime from the TimeKeeper. */
  ace_time::OffsetDateTime dateTime;
};

#endif
