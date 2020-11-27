#ifndef WORLD_CLOCK_LCD_RENDERING_INFO_H
#define WORLD_CLOCK_LCD_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */ 
struct RenderingInfo {
  uint8_t mode; // display mode, see MODE_xxx in config.h
  bool suppressBlink; // true if blinking should be suppressed
  bool blinkShowState; // true if blinking info should be shown

  uint8_t hourMode; // ClockInfo::kTwelve or kTwentyFour
  ace_time::TimeZoneData zones[NUM_TIME_ZONES];
  ace_time::ZonedDateTime dateTime;

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    uint8_t backlightLevel; // LCD backlight level [0, 9]
    uint8_t contrast; // LCD contrast [0, 127]
    uint8_t bias; // LCD backlight level [0, 7]
  #else
    uint8_t contrastLevel; // OLED contrast level [0, 9] -> [0, 255]
  #endif
};

#endif
