#ifndef LED_CLOCK_TINY_STORED_INFO_H
#define LED_CLOCK_TINY_STORED_INFO_H

#include <stdint.h>
#include <AceTime.h>

/** Data that is saved to and retrieved from EEPROM. */
struct StoredInfo {
  uint8_t hourMode;
  uint8_t brightness;
};

#endif
