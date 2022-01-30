#ifndef MULTI_ZONE_CLOCK_CONFIG_H
#define MULTI_ZONE_CLOCK_CONFIG_H

#include <stdint.h>

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

// Set to >= 1 to print debugging info to SERIAL_PORT_MONITOR
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// Set to >= 1 to enable periodic calculation of the frames-per-second.
#ifndef ENABLE_FPS_DEBUG
#define ENABLE_FPS_DEBUG 0
#endif

// Set to 1 to force the ClockInfo to its initial state
#define FORCE_INITIALIZE 0

// OLED address: 0X3C+SA0 - 0x3C or 0x3D
#define OLED_I2C_ADDRESS 0x3C

// Define the display type, either a 128x64 OLED or a 88x48 LCD
#define DISPLAY_TYPE_OLED 0
#define DISPLAY_TYPE_LCD 1

// Determine type of TimeZone. Extended is too big for a Nano or Pro Micro, but
// will work on an ESP8266 or ESP32.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE_BASICDB 3
#define TIME_ZONE_TYPE_EXTENDEDDB 4

// If Basic or Extended, number of time zones to display.
#define NUM_TIME_ZONES 4

// List of clock types for the referenceClock and backupClock of the
// SystemClock. Used as the value of the TIME_SOURCE_TYPE and
// BACKUP_TIME_SOURCE_TYPE macros.
#define TIME_SOURCE_TYPE_NONE 0
#define TIME_SOURCE_TYPE_DS3231 1
#define TIME_SOURCE_TYPE_NTP 2
#define TIME_SOURCE_TYPE_ESP_SNTP 3
#define TIME_SOURCE_TYPE_STMRTC 4
#define TIME_SOURCE_TYPE_STM32F1RTC 5

// SystemClock
#define SYSTEM_CLOCK_TYPE_LOOP 0
#define SYSTEM_CLOCK_TYPE_COROUTINE 1
#define SYSTEM_CLOCK_TYPE SYSTEM_CLOCK_TYPE_COROUTINE
#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  #define SYSTEM_CLOCK SystemClockLoop
#else
  #define SYSTEM_CLOCK SystemClockCoroutine
#endif

// Button options: either digital ButtonConfig or analog LadderButtonConfig.
// AVR: 10-bit analog pin
// ESP8266: 10-bit analog pin
// ESP32: 12-bit analog pin
#define BUTTON_TYPE_DIGITAL 0
#define BUTTON_TYPE_ANALOG 1

//-----------------------------------------------------------------------------

#if defined(EPOXY_DUINO)
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif ! defined(AUNITER) // Arduino IDE in interactive mode
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_NANO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_MICRO_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP true

#elif defined(AUNITER_MEGA_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_SAMD_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 0
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_EXTENDED

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 11
  #define CHANGE_BUTTON_PIN 10

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP true

#elif defined(AUNITER_WORLD_CLOCK_LCD)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BUTTON_LEVELS { \
      0 /*0%, "short" to ground w/ 470*/, \
      512 /*50%, 10k*/, \
      1023 /*100%, open*/ \
    }

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_LCD
  #define LCD_SPI_DATA_COMMAND_PIN D4
  #define LCD_BACKLIGHT_PIN D3
  #define LCD_INITIAL_CONTRAST 20
  #define LCD_INITIAL_BIAS 7
  #define OLED_REMAP true

#elif defined(AUNITER_D1MINI_SMALL_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_EXTENDED

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN D7
  #define CHANGE_BUTTON_PIN D5

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_D1MINI_LARGE_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_EXTENDED

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BUTTON_LEVELS { \
      0 /*short to ground*/, \
      327 /*32%, 4.7k*/, \
      512 /*50%, 10k*/, \
      844 /*82%, 47k*/, \
      1023 /*100%, open*/ \
    }

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_LCD
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    #define LCD_SPI_DATA_COMMAND_PIN D4
    #define LCD_BACKLIGHT_PIN D3
    #define LCD_INITIAL_CONTRAST 20
    #define LCD_INITIAL_BIAS 7
  #else
    #define OLED_REMAP false
  #endif

// ESP32 Box using EzSBC board.
#elif defined(AUNITER_ESP32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_ESP_SNTP
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_NONE
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_EXTENDED

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 4

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

// ESP32 large dev board.
#elif defined(AUNITER_ESP32_OLED)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_EXTENDED

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BUTTON_LEVELS { \
      0 /*short to ground*/, \
      327 /*32%, 4.7k*/, \
      512 /*50%, 10k*/, \
      844 /*82%, 47k*/, \
      1023 /*100%, open*/ \
    }

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#else
  #error Unknown AUNITER environment
#endif

//------------------------------------------------------------------
// Button state transition nodes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
  // must be identical to ace_utils::mode_groups::kModeUnknown
  kUnknown = 0,

  kViewDateTime,
  kViewTimeZone,
  kViewSettings,
  kViewSysclock,
  kViewAbout,

  kChangeYear,
  kChangeMonth,
  kChangeDay,
  kChangeHour,
  kChangeMinute,
  kChangeSecond,

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
  kChangeTimeZone0Offset,
  kChangeTimeZone0Dst,
  kChangeTimeZone1Offset,
  kChangeTimeZone1Dst,
  kChangeTimeZone2Offset,
  kChangeTimeZone2Dst,
  kChangeTimeZone3Offset,
  kChangeTimeZone3Dst,
#else
  kChangeTimeZone0Name,
  kChangeTimeZone1Name,
  kChangeTimeZone2Name,
  kChangeTimeZone3Name,
#endif

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
  kChangeSettingsBacklight,
  kChangeSettingsContrast,
  kChangeSettingsBias,
  kChangeInvertDisplay,
#else
  kChangeSettingsContrast,
  kChangeInvertDisplay,
#endif

};

#endif
