#ifndef WORLD_CLOCK_STORED_INFO_H
#define WORLD_CLOCK_STORED_INFO_H

#include "ClockInfo.h"

/**
 * Data that is saved to and retrieved from EEPROM.
 * These settings apply to all clocks.
 */
struct StoredInfo {
  /** Hour mode, 12H or 24H. */
  uint8_t hourMode;

  /** Blink the colon in HH:MM. */
  bool blinkingColon;

  /** Oled contrast level [0-9]. */
  uint8_t contrastLevel;

  /** Invert display mode, [0-2]. */
  uint8_t invertDisplay;
};

#endif
