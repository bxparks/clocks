#ifndef ONE_ZONE_CLOCK_CLOCK_INFO_H
#define ONE_ZONE_CLOCK_CLOCK_INFO_H

#include <AceTime.h>

/** Information about the clock, mostly independent of rendering. */
struct ClockInfo {
  /** 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

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

#endif

  /** The desired timezone of the clock. */
  ace_time::TimeZoneData timeZoneData;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;
};

#endif
