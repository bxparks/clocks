#ifndef WORLD_CLOCK_LCD_STORED_INFO_H
#define WORLD_CLOCK_LCD_STORED_INFO_H

#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  /** 12:00:00 AM to 12:00:00 PM */
  static uint8_t const kTwelve = 0;

  /** 00:00:00 - 23:59:59 */
  static uint8_t const kTwentyFour = 1;

  /** Either kTwelve or kTwentyFour. */
  uint8_t hourMode;

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD

  /** Backlight level, [0, 9] */
  uint8_t backlightLevel;

  /** Contrast, [0, 127] */
  uint8_t contrast;

  /** Bias level, [0, 7] */
  uint8_t bias;

#else

  /**
   * Contrast level for OLED dislay, [0, 9] -> [0, 255]. Essentially brightness
   * because the background is black.
   */
  uint8_t contrastLevel;

#endif

  /** TimeZone serialization. */
  ace_time::TimeZoneData zones[NUM_TIME_ZONES];

};

#endif
