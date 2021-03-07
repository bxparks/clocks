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
  bool blinkShowState; // true if blinking info should be shown

  uint8_t hourMode; // ClockInfo::kTwelve or kTwentyFour

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    uint8_t backlightLevel; // LCD backlight level [0, 9] -> PWM value
    uint8_t contrast; // LCD contrast [0, 127]
    uint8_t bias; // LCD backlight level [0, 7]
  #else
    uint8_t contrastLevel; // OLED contrast level [0, 9] -> [0, 255]
    uint8_t invertDisplay; // 0: off, 1: on; should never be 2
  #endif

  ace_time::TimeZoneData timeZoneData;
  ace_time::ZonedDateTime dateTime;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.hourMode == b.hourMode
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      && a.backlightLevel == b.backlightLevel
      && a.contrast == b.contrast
      && a.bias == b.bias
    #else
      && a.contrastLevel == b.contrastLevel
      && a.invertDisplay == b.invertDisplay
    #endif
      && a.timeZoneData == b.timeZoneData
      && a.dateTime == b.dateTime;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
