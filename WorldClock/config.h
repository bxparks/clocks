#ifndef WORLD_CLOCK_CONFIG_H
#define WORLD_CLOCK_CONFIG_H

#include <stdint.h>

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

#define WORLD_CLOCK_VERSION_STRING "2025.04.25"

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

// Whether to use Basic TimeZone or Extended TimeZone. Manual not supported.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

// Communication interfaces for LED and DS3231 RTC.
// Used by OLED_INTERFACE_TYPE and DS3231_INTERFACE_TYPE macros.
#define INTERFACE_TYPE_DIRECT 0
#define INTERFACE_TYPE_DIRECT_FAST 1
#define INTERFACE_TYPE_HARD_SPI 2
#define INTERFACE_TYPE_HARD_SPI_FAST 3
#define INTERFACE_TYPE_SIMPLE_SPI 4
#define INTERFACE_TYPE_SIMPLE_SPI_FAST 5
#define INTERFACE_TYPE_SIMPLE_TMI 6
#define INTERFACE_TYPE_SIMPLE_TMI_FAST 7
#define INTERFACE_TYPE_TWO_WIRE 8
#define INTERFACE_TYPE_SIMPLE_WIRE 9
#define INTERFACE_TYPE_SIMPLE_WIRE_FAST 10
#define INTERFACE_TYPE_SSD1306_SPI 11 /* Use SSD1306AsciiSpi */
#define INTERFACE_TYPE_SSD1306_SOFT_SPI 12 /* Use SSD1306AsciiSoftSpi */

// Interface configuration for DS3231
#define DS3231_INTERFACE_TYPE INTERFACE_TYPE_TWO_WIRE
#define SDA_PIN SDA
#define SCL_PIN SCL
#define WIRE_BIT_DELAY 1

// Interface configuration for SPI OLEDs
#define OLED_INTERFACE_TYPE INTERFACE_TYPE_SSD1306_SPI
#define OLED_REMAP false
#define OLED_CS0_PIN 18
#define OLED_CS1_PIN 19
#define OLED_CS2_PIN 20
#define OLED_RST_PIN 4
#define OLED_DC_PIN 10
#define OLED_DATA_PIN MOSI
#define OLED_CLOCK_PIN SCK

//------------------------------------------------------------------
// Rendering modes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
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
};

#endif
