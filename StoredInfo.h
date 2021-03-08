#ifndef LED_CLOCK_STORED_INFO_H
#define LED_CLOCK_STORED_INFO_H

#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  uint8_t hourMode;
  ace_time::TimeZoneData timeZoneData;
};

#endif
