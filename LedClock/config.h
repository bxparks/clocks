#ifndef LED_CLOCK_CONFIG_H
#define LED_CLOCK_CONFIG_H

#include <stdint.h> // uint8_t

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

// Set >= 1 to print debugging info to SERIAL_PORT_MONITOR
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// Determine whether "auto" time zone uses Basic or Extended. Extended is too
// big for a Nano or Pro Micro, but will work on an ESP8266 or ESP32.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

// Options that determine the source of time.
#define TIME_SOURCE_TYPE_NONE 0
#define TIME_SOURCE_TYPE_DS3231 1
#define TIME_SOURCE_TYPE_NTP 2
#define TIME_SOURCE_TYPE_BOTH 3
#define TIME_SOURCE_TYPE_STMRTC 4
#define TIME_SOURCE_TYPE_STM32F1RTC 5

// Type of LED display.
#define LED_DISPLAY_TYPE_SCANNING 0
#define LED_DISPLAY_TYPE_TM1637 1

// Options that determine that type of LED display.
#define LED_MATRIX_MODE_NONE 0
#define LED_MATRIX_MODE_DIRECT 1
#define LED_MATRIX_MODE_PARIAL_SW_SPI 2
#define LED_MATRIX_MODE_SINGLE_HW_SPI 3
#define LED_MATRIX_MODE_DUAL_SW_SPI 4
#define LED_MATRIX_MODE_DUAL_HW_SPI 5
#define LED_MATRIX_MODE_DIRECT_FAST 6
#define LED_MATRIX_MODE_SINGLE_SW_SPI_FAST 7
#define LED_MATRIX_MODE_DUAL_SW_SPI_FAST 8

#if defined(EPOXY_DUINO)
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  #define LED_MATRIX_MODE LED_MATRIX_MODE_DIRECT

  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define LED_MODULE_TYPE LED_MODULE_DIRECT

#elif !defined(AUNITER)
  #warning Arduino IDE detected. Check config parameters.
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  #define LED_MODULE_TYPE LED_MODULE_DIRECT

#elif defined(AUNITER_LED_CLOCK_DIRECT)
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  //#define LED_MATRIX_MODE LED_MATRIX_MODE_DIRECT
  #define LED_MATRIX_MODE LED_MATRIX_MODE_DIRECT_FAST

  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

#elif defined(AUNITER_LED_CLOCK_SINGLE)
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  //#define LED_MATRIX_MODE LED_MATRIX_MODE_SINGLE_SW_SPI
  //#define LED_MATRIX_MODE LED_MATRIX_MODE_SINGLE_HW_SPI
  #define LED_MATRIX_MODE LED_MATRIX_MODE_SINGLE_SW_SPI_FAST

  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

#elif defined(AUNITER_LED_CLOCK_DUAL)
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  //#define LED_MATRIX_MODE LED_MATRIX_MODE_DUAL_SW_SPI
  //#define LED_MATRIX_MODE LED_MATRIX_MODE_DUAL_HW_SPI
  #define LED_MATRIX_MODE LED_MATRIX_MODE_DUAL_SW_SPI_FAST

  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

#elif defined(AUNITER_LED_CLOCK_TM1637)
  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define LED_MATRIX_MODE LED_MATRIX_MODE_NONE

  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

#else
  #error Unknown AUNITER environment
#endif

//------------------------------------------------------------------
// Button state transition nodes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
  kUnknown = 0, // uninitialized

  kViewDateTime,
  kViewHourMinute,
  kViewMinuteSecond,
  kViewYear,
  kViewMonth,
  kViewDay,
  kViewWeekday,
  kViewTimeZone,

  kChangeYear,
  kChangeMonth,
  kChangeDay,
  kChangeHour,
  kChangeMinute,
  kChangeSecond,

  kChangeTimeZoneOffset,
  kChangeTimeZoneDst,
  kChangeHourMode,
};

#endif
