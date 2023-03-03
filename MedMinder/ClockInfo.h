#ifndef MED_MINDER_CLOCK_INFO_H
#define MED_MINDER_CLOCK_INFO_H

#include <AceTime.h>
#include "config.h" // DISPLAY_TYPE

/** Information about the clock. */
struct ClockInfo {
  /** Blinking info should be shown. Should be toggled every 0.5 sec. */
  bool blinkShowState = false;

  /** Blinking should be suppressed. e.g. when RepeatPress is active. */
  bool suppressBlink = false;

  /** The desired timezone of the clock. */
  ace_time::TimeZone timeZone;

  /** Current time. */
  ace_time::ZonedDateTime dateTime;

  /** Time when the last pill was taken. */
  uint32_t medStartTime;

  /**
   * How often the pill should be taken. In Mode::kViewMed mode, the
   * ChangingClockInfo.medInterval field is overloaded to hold the TimePeriod
   * corresponding to "time until next med" information.
   */
  ace_time::TimePeriod medInterval;

  /**
   * Contrast level for OLED dislay, [0, 9] -> [25, 255]. Essentially brightness
   * because the background is black.
   */
  uint8_t contrastLevel;
};

#endif

