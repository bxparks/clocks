#ifndef WORLD_CLOCK_RENDERING_INFO_H
#define WORLD_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "ClockInfo.h"

using namespace ace_time;

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed. Contains similar information as ClockInfo but in
 * addition to other information related to the presentation of the clock.
 */ 
struct RenderingInfo {
  uint8_t mode = MODE_UNKNOWN; // display mode
  bool blinkShowState = true; // true if blinking info should be shown
  bool isRendered = false; // true if the info has been rendered

  ClockInfo clockInfo;
  acetime_t now; // seconds from AceTime epoch
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.now == b.now
      && a.blinkShowState == b.blinkShowState
      && a.clockInfo == b.clockInfo
      && a.mode == b.mode;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
