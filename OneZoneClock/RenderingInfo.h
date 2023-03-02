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
  ClockInfo clockInfo;

  /**
   * Actual inversion mode, derived from clockInfo.
   *
   *  * 0: white on black
   *  * 1: black on white
   */
  uint8_t invertDisplay = 0;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.clockInfo == b.clockInfo;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
