#ifndef MED_MINDER_MED_INFO_H
#define MED_MINDER_MED_INFO_H

#include <AceTime.h>

using namespace ace_time;

/**
 * Information about when the pill was last ingested, how often it should be
 * ingested. The target time when it should be next ingested is (startTime +
 * interval).
 *
 * The time left to next pill is (startTime + interval - now()).
 */
struct MedInfo {
  uint32_t startTime; // when the last pill was taken
  TimePeriod interval; // how often the pill should be taken
};

#endif
