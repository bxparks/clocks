#ifndef WORLD_CLOCK_RENDERING_INFO_H
#define WORLD_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"
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
  uint8_t hourMode;
  bool blinkingColon;
  uint8_t contrastLevel;
  uint8_t invertDisplay; // 0: off, 1: on
  const char* name;
  acetime_t now; // seconds from AceTime epoch
  TimeZone timeZone;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  return a.mode == b.mode
      && a.blinkShowState == b.blinkShowState
      && a.hourMode == b.hourMode
      && a.blinkingColon == b.blinkingColon
      && a.contrastLevel == b.contrastLevel
      && a.invertDisplay == b.invertDisplay
      && a.name == b.name
      && a.now == b.now
      && a.timeZone == b.timeZone;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
