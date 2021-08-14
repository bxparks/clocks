#ifndef ONE_ZONE_CLOCK_STORED_INFO_H
#define ONE_ZONE_CLOCK_STORED_INFO_H

#include "config.h"
#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  /** Either ClockInfo::kTwelve or ClockInfo::kTwentyFour. */
  uint8_t hourMode;

  // Keeping uint8_t grouped together helps reduce storage on 32-bit processors
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

  /** Invert display mode, [0-2]. Only for OLED. */
  uint8_t invertDisplay;

#endif

#if ENABLE_LED_DISPLAY
  /** Enable LED Module or not interactively. */
  bool ledOnOff;

  /** Brightness of LED module. */
  uint8_t ledBrightness;
#endif

  /** TimeZone serialization. */
  ace_time::TimeZoneData timeZoneData;
};

#endif
