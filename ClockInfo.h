#ifndef MED_MINDER_CLOCK_INFO_H
#define MED_MINDER_CLOCK_INFO_H

#include <AceTime.h>

/** Information about the clock, mostly independent of rendering. */
struct ClockInfo {
  /** The desired timezone of the clock. */
  ace_time::TimeZone timeZone;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;
};

#endif

