#ifndef ONE_ZONE_CLOCK_RENDERING_INFO_H
#define ONE_ZONE_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h" // Mode::kUnknown
#include "ClockInfo.h"

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */
struct RenderingInfo {
  Mode mode = Mode::kUnknown; // display mode
  bool blinkShowState; // true if blinking info should be shown

  ClockInfo clockInfo;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return
    a.blinkShowState == b.blinkShowState
    && a.mode == b.mode
    && a.clockInfo == b.clockInfo;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
