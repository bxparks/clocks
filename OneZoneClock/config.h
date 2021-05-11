#ifndef ONE_ZONE_CLOCK_CONFIG_H
#define ONE_ZONE_CLOCK_CONFIG_H

#include <stdint.h>

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

#define CLOCK_VERSION_STRING "0.2"

// Set to 1 to print debugging info to SERIAL_PORT_MONITOR
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// Set to 1 to enable periodic calculation of the frames-per-second.
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

// Determine whether "auto" time zone uses Basic or Extended. Extended is too
// big for a Nano or Pro Micro, but will work on an ESP8266 or ESP32.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

// SystemClock
#define SYSTEM_CLOCK_TYPE_LOOP 0
#define SYSTEM_CLOCK_TYPE_COROUTINE 1
#define SYSTEM_CLOCK_TYPE SYSTEM_CLOCK_TYPE_COROUTINE
#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  #define SYSTEM_CLOCK SystemClockLoop
#else
  #define SYSTEM_CLOCK SystemClockCoroutine
#endif


// Options that define the authoratative source of the time.
#define TIME_SOURCE_TYPE_NONE 0
#define TIME_SOURCE_TYPE_DS3231 1
#define TIME_SOURCE_TYPE_NTP 2
#define TIME_SOURCE_TYPE_BOTH 3
#define TIME_SOURCE_TYPE_STMRTC 4
#define TIME_SOURCE_TYPE_STM32F1RTC 5

// Button options: either digital buttons using ButtonConfig, 2 analog buttons
// using LadderButtonConfig, or 4 analog buttons using LadderButtonConfig:
//  * AVR: 10-bit analog pin
//  * ESP8266: 10-bit analog pin
//  * ESP32: 12-bit analog pin
#define BUTTON_TYPE_DIGITAL 0
#define BUTTON_TYPE_ANALOG 1

#if ! defined(AUNITER) // Arduino IDE in interactive mode
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(EPOXY_DUINO)
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_NANO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_MICRO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Pro Micro has just enough memory for Basic TimeZone w/ 4 timezones.
  // In certain configurations, it does not have enough memory, in which case
  // the Manual timezones must be used.
  #undef TIME_ZONE_TYPE
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_LED_CLOCK)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Pro Micro has just enough memory for Basic TimeZone w/ 4 timezones.
  // In certain configurations, it does not have enough memory, in which case
  // the Manual timezones must be used.
  #undef TIME_ZONE_TYPE
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP true

#elif defined(AUNITER_MINI_MINDER)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_MEGA)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_SAMD)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 0
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 11
  #define CHANGE_BUTTON_PIN 10

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP true

#elif defined(AUNITER_STM32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_STM32F1RTC

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A0
  #define CHANGE_BUTTON_PIN A1

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_ESP8266)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#elif defined(AUNITER_D1MINI)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_COUNT 2
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BITS 10

  // Display parameters
  #define DISPLAY_TYPE DISPLAY_TYPE_LCD
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    #define LCD_SPI_DATA_COMMAND_PIN D4
    #define LCD_BACKLIGHT_PIN D3
    #define LCD_INITIAL_CONTRAST 20
    #define LCD_INITIAL_BIAS 7
  #else
    #define OLED_REMAP true
  #endif

#elif defined(AUNITER_D1MINI_LARGE)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BITS 10

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

#elif defined(AUNITER_ESP32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_BOTH

  // Button parameters
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 4

  // Display type
  #define DISPLAY_TYPE DISPLAY_TYPE_OLED
  #define OLED_REMAP false

#else
  #error Unknown AUNITER environment
#endif

//------------------------------------------------------------------
// Button state transition nodes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
  kUnknown = 0, // must be identical to ace_utils::mode_groups::kModeUnknown

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
  kChangeTimeZoneOffset,
  kChangeTimeZoneDst,
#else
  kChangeTimeZoneName,
#endif

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
  kChangeSettingsBacklight,
  kChangeSettingsContrast,
  kChangeSettingsBias,
#else
  kChangeSettingsContrast,
  kChangeInvertDisplay,
#endif
};

#endif
