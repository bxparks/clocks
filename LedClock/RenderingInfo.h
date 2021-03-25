#ifndef LED_CLOCK_RENDERING_INFO_H
#define LED_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */ 
struct RenderingInfo {
  Mode mode = Mode::kUnknown; // display mode
  bool blinkShowState = true; // true if blinking info should be shown
  uint8_t hourMode = 0; // 12/24 mode
  ace_time::TimeZoneData timeZoneData;
  ace_time::ZonedDateTime dateTime; // seconds from AceTime epoch
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.hourMode == b.hourMode
      && a.timeZoneData == b.timeZoneData
      && a.dateTime == b.dateTime;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
