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
#define DISPLAY_TYPE DISPLAY_TYPE_OLED

// Determine whether "auto" time zone uses Basic or Extended. Extended is too
// big for a Nano or Pro Micro, but will work on an ESP8266 or ESP32.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

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

// Button options: either digital ButtonConfig or analog LadderButtonConfig.
// AVR: 8-bit analog pin
// ESP8266: 10-bit analog pin
#define BUTTON_TYPE_DIGITAL 0
#define BUTTON_TYPE_ANALOG 1
#define BUTTON_TYPE BUTTON_TYPE_DIGITAL

#if ! defined(AUNITER) // Arduino IDE in interactive mode
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false

#elif defined(EPOXY_DUINO)
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false

#elif defined(AUNITER_NANO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false
#elif defined(AUNITER_MICRO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1

  // Pro Micro has just enough memory for Basic TimeZone w/ 4 timezones.
  // In certain configurations, it does not have enough memory, in which case
  // the Manual timezones must be used.
  #undef TIME_ZONE_TYPE
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false
#elif defined(AUNITER_MINI_MINDER)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false
#elif defined(AUNITER_MEGA)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false
#elif defined(AUNITER_SAMD)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 0
  #define MODE_BUTTON_PIN 11
  #define CHANGE_BUTTON_PIN 10
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP true
#elif defined(AUNITER_STM32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN A0
  #define CHANGE_BUTTON_PIN A1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_STM32F1RTC
  #define OLED_REMAP false
#elif defined(AUNITER_ESP8266)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define OLED_REMAP false
#elif defined(AUNITER_D1MINI)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_BOTH

  // Button parameters
  #undef BUTTON_TYPE
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  #define ANALOG_BITS 10

  // Display parameters
  #undef DISPLAY_TYPE
  #define DISPLAY_TYPE DISPLAY_TYPE_LCD
  #define LCD_SPI_DATA_COMMAND_PIN D4
  #define LCD_BACKLIGHT_PIN D3
  #define LCD_INITIAL_CONTRAST 20
  #define LCD_INITIAL_BIAS 7
  #define OLED_REMAP true
#elif defined(AUNITER_ESP32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_EEPROM 1
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 4
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_BOTH
  #define OLED_REMAP false
#else
  #error Unknown AUNITER environment
#endif

//------------------------------------------------------------------
// Button state transition nodes.
//------------------------------------------------------------------

// must be identical to ace_utils::mode_groups::kModeUnknown
const uint8_t MODE_UNKNOWN = 0;

const uint8_t MODE_DATE_TIME = 1;
const uint8_t MODE_TIME_ZONE = 2;
const uint8_t MODE_SETTINGS = 3;
const uint8_t MODE_SYSCLOCK = 4;
const uint8_t MODE_ABOUT = 5;

const uint8_t MODE_CHANGE_YEAR = 10;
const uint8_t MODE_CHANGE_MONTH = 11;
const uint8_t MODE_CHANGE_DAY = 12;
const uint8_t MODE_CHANGE_HOUR = 13;
const uint8_t MODE_CHANGE_MINUTE = 14;
const uint8_t MODE_CHANGE_SECOND = 15;

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
const uint8_t MODE_CHANGE_TIME_ZONE_OFFSET = 20;
const uint8_t MODE_CHANGE_TIME_ZONE_DST = 21;
#else
const uint8_t MODE_CHANGE_TIME_ZONE_NAME = 20;
#endif

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
const uint8_t MODE_CHANGE_SETTINGS_BACKLIGHT = 30;
const uint8_t MODE_CHANGE_SETTINGS_CONTRAST = 31;
const uint8_t MODE_CHANGE_SETTINGS_BIAS = 32;
#else
const uint8_t MODE_CHANGE_SETTINGS_CONTRAST = 30;
const uint8_t MODE_CHANGE_INVERT_DISPLAY = 31;
#endif


#endif
