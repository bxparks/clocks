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
  /** display mode */
  Mode mode = Mode::kUnknown;

  /** true if blinking info should be shown */
  bool blinkShowState = true;

  /**
   * Information about how to render the clock: date, time, timezone, contrast.
   */
  ClockInfo clockInfo;

  /** Seconds from AceTime epoch. */
  acetime_t now;

  /**
   * The primary time zone of the clock, used to calculate auto-inversion of
   * the display.
   */
  ace_time::TimeZone primaryTimeZone;

  /**
   * Actual inversion mode, derived from clockInfo.
   *
   *  * 0: white on black
   *  * 1: black on white
   */
  uint8_t invertDisplay = 0;
};

inline bool operator==(const RenderingInfo& a, const RenderingInfo& b) {
  // The following fields are ordered so that frequently changing fields come
  // before infrequently changing fields. This allows the '&&' operator to
  // short-circuit faster.
  return a.blinkShowState == b.blinkShowState
      && a.now == b.now
      && a.invertDisplay == b.invertDisplay
      && a.clockInfo == b.clockInfo
      && a.primaryTimeZone == b.primaryTimeZone
      && a.mode == b.mode;
}

inline bool operator!=(const RenderingInfo& a, const RenderingInfo& b) {
  return ! (a == b);
}

#endif
