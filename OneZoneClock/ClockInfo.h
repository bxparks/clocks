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
  static uint8_t const kInvertDisplayAuto = 2;

  /** Invert display mode. Only for OLED. */
  uint8_t invertDisplay;

#endif

  /** The desired timezone of the clock. */
  ace_time::TimeZoneData timeZoneData;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;
};

#endif
