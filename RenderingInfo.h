#ifndef MED_MINDER_RENDERING_INFO_H
#define MED_MINDER_RENDERING_INFO_H

#include <AceTime.h>
#include "config.h"

static const uint8_t MODE_UNKNOWN = 0; // uninitialized
static const uint8_t MODE_DATE_TIME = 1;
static const uint8_t MODE_CHANGE_YEAR = 2;
static const uint8_t MODE_CHANGE_MONTH = 3;
static const uint8_t MODE_CHANGE_DAY = 4;
static const uint8_t MODE_CHANGE_HOUR = 5;
static const uint8_t MODE_CHANGE_MINUTE = 6;
static const uint8_t MODE_CHANGE_SECOND = 7;
static const uint8_t MODE_VIEW_MED = 8;
static const uint8_t MODE_CHANGE_MED_HOUR = 9;
static const uint8_t MODE_CHANGE_MED_MINUTE = 10;
static const uint8_t MODE_TIME_ZONE = 11;
static const uint8_t MODE_CHANGE_TIME_ZONE_HOUR = 12;
static const uint8_t MODE_CHANGE_TIME_ZONE_MINUTE = 13;
static const uint8_t MODE_CHANGE_TIME_ZONE_DST = 14;

namespace med_minder {

using namespace ace_time;

/**
 * This is essential the "model" in an MVC architecture. Contains the
 * information which will be rendered to the display.
 */ 
struct RenderingInfo {
  uint8_t mode = 0;
  DateTime dateTime; // currentDateTime, changingDateTime
  TimePeriod timePeriod; // med interval, med remaining
  bool suppressBlink = false; // true if blinking should be suppressed
  bool blinkShowState = true; // true if should be rendered
};

}

#endif
