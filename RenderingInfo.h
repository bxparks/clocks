#ifndef WORLD_CLOCK_RENDERING_INFO_H
#define WORLD_CLOCK_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"
#include "ClockInfo.h"

using namespace ace_time;

/**
 * Data used by the Presenter (the "View") to determine what has changed and
 * what needs to be displayed.
 */ 
struct RenderingInfo {
  uint8_t mode = MODE_UNKNOWN; // display mode
  bool suppressBlink = false; // true if blinking should be suppressed
  bool blinkShowState = true; // true if blinking info should be shown
  uint32_t now; // seconds from AceTime epoch
  ClockInfo clockInfo; // time zone and other info about the clock
};

#endif
