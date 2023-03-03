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
  Mode mode = Mode::kUnknown;
  bool blinkShowState = true; // true if should be rendered
  bool suppressBlink = false;
  TimeZone timeZone; // currentTimeZone or changingTimeZone
  ZonedDateTime dateTime; // currentDateTime or changingDateTime
  TimePeriod timePeriod; // med interval or med remaining
  uint8_t contrastLevel;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.suppressBlink == b.suppressBlink
      && a.timeZone == b.timeZone
      && a.dateTime == b.dateTime
      && a.timePeriod == b.timePeriod
      && a.contrastLevel == b.contrastLevel;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
