#ifndef MULTI_ZONE_CLOCK_STORED_INFO_H
#define MULTI_ZONE_CLOCK_STORED_INFO_H

#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  /** Either ClockInfo::kTwelve or ClockInfo::kTwentyFour. */
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

  /** Invert display mode. Only for OLED. */
  uint8_t invertDisplay;

  /** TimeZone serialization. */
  ace_time::TimeZoneData zones[NUM_TIME_ZONES];

};

#endif
