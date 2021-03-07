#ifndef ONE_ZONE_CLOCK_RENDERING_INFO_H
#define ONE_ZONE_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */ 
struct RenderingInfo {
  uint8_t mode = MODE_UNKNOWN; // display mode, see MODE_xxx in config.h
  bool suppressBlink; // true if blinking should be suppressed
  bool blinkShowState; // true if blinking info should be shown

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    uint8_t backlightLevel; // LCD backlight level [0, 9]
    uint8_t contrast; // LCD contrast [0, 127]
    uint8_t bias; // LCD backlight level [0, 7]
  #else
    uint8_t contrastLevel; // OLED contrast level [0, 9] -> [0, 255]
  #endif

  uint8_t hourMode; // ClockInfo::kTwelve or kTwentyFour
  uint8_t invertDisplay; // 0: off, 1: on; should never be 2

  ace_time::TimeZoneData timeZoneData;
  ace_time::ZonedDateTime dateTime;
};

#endif
