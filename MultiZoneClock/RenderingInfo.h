#ifndef MULTI_ZONE_CLOCK_RENDERING_INFO_H
#define MULTI_ZONE_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "ClockInfo.h"

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */ 
struct RenderingInfo {
  Mode mode; // display mode
  bool blinkShowState; // true if blinking info should be shown

  ClockInfo clockInfo;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.clockInfo == b.clockInfo;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
