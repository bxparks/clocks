#ifndef WORLD_CLOCK_CONFIG_H
#define WORLD_CLOCK_CONFIG_H

#include <stdint.h>

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// PersistentStore
#define ENABLE_EEPROM 1

// Set to 1 to factory reset.
#define FORCE_INITIALIZE 0

// This program should compile for most target environments, including AVR,
// ESP8266, and ESP32. The parameters below are for the specific device that I
// built which has a Pro Micro with 3 OLED displays using SPI, a DS3231 over
// I2C, and 2 buttons.
#define MODE_BUTTON_PIN 8
#define CHANGE_BUTTON_PIN 9
#define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
#define OLED_REMAP false
#define OLED_CS0_PIN 18
#define OLED_CS1_PIN 19
#define OLED_CS2_PIN 20
#define OLED_RST_PIN 4
#define OLED_DC_PIN 10

// Whether to use Manual TimeZone, Basic TimeZone or Extended TimeZone.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

//------------------------------------------------------------------
// Rendering modes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
  // must be identical to ace_utils::mode_groups::kModeUnknown
  kUnknown = 0,

  kViewDateTime,
  kViewSettings,
  kViewAbout,

  kChangeYear,
  kChangeMonth,
  kChangeDay,
  kChangeHour,
  kChangeMinute,
  kChangeSecond,

  kChangeHourMode,
  kChangeBlinkingColon,
  kChangeContrast, // OLED contrast/brightness
  kChangeInvertDisplay,

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
  kChangeTimeZoneDst0,
  kChangeTimeZoneDst1,
  kChangeTimeZoneDst2,
#endif

};

#endif
