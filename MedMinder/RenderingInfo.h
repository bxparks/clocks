#ifndef MED_MINDER_RENDERING_INFO_H
#define MED_MINDER_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"

using namespace ace_time;

/**
 * This is essential the "model" in an MVC architecture. Contains the
 * information which will be rendered to the display.
 */ 
struct RenderingInfo {
  uint8_t mode = 0;
  bool blinkShowState = true; // true if should be rendered
  TimeZone timeZone; // currentTimeZone or changingTimeZone
  ZonedDateTime dateTime; // currentDateTime or changingDateTime
  TimePeriod timePeriod; // med interval or med remaining
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.timeZone == b.timeZone
      && a.dateTime == b.dateTime
      && a.timePeriod == b.timePeriod;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
