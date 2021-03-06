#ifndef MED_MINDER_CLOCK_INFO_H
#define MED_MINDER_CLOCK_INFO_H

#include <AceTime.h>

/** Information about the clock, mostly independent of rendering. */
struct ClockInfo {
  /** The desired timezone of the clock. */
  ace_time::TimeZone timeZone;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;

  /** Time when the last pill was taken. */
  uint32_t medStartTime;

  /**
   * How often the pill should be taken. In MODE_VIEW_MED mode, the
   * ChangingClockInfo.medInterval field is overloaded to hold the TimePeriod
   * corresponding to "time until next med" information.
   */
  TimePeriod medInterval;
};

#endif

