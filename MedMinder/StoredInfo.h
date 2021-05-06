#ifndef MED_MINDER_STORED_INFO_H
#define MED_MINDER_STORED_INFO_H

#include <stdint.h> // uint32_t
#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  /** Current time zone. */
  ace_time::TimeZoneData timeZoneData;

  /** Time when the last pill was taken. */
  uint32_t medStartTime;

  /** How often the pill should be taken. */
  ace_time::TimePeriod medInterval;
};

#endif
