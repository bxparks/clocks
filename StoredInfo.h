#ifndef MED_MINDER_STORED_INFO_H
#define MED_MINDER_STORED_INFO_H

#include <AceTime.h>
#include "MedInfo.h"

namespace med_minder {

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  /** Current time zone. */
  ace_time::TimeZoneData timeZoneData;

  /** Medication info. */
  MedInfo medInfo;
};

}

#endif
