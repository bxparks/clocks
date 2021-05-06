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

// Button options: either digital buttons using ButtonConfig, 2 analog buttons
// using LadderButtonConfig, or 4 analog buttons using LadderButtonConfig:
//  * AVR: 8-bit analog pin
//  * ESP8266: 10-bit analog pin
#define BUTTON_TYPE_DIGITAL 0
#define BUTTON_TYPE_ANALOG 1

// Determine whether "auto" time zone uses Basic or Extended. Extended is too
// big for a Nano or Pro Micro, but will work on an ESP8266 or ESP32.
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE_EXTENDED 2
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_MANUAL

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
#define LED_DISPLAY_TYPE_MAX7219 2
#define LED_DISPLAY_TYPE_HC595_DUAL 5

// Communication interface to the LED module (e.g. SoftSpi, SoftSpiFast, etc)
#define INTERFACE_TYPE_NORMAL 0
#define INTERFACE_TYPE_FAST 1

// SystemClock
#define SYSTEM_CLOCK_TYPE_LOOP 0
#define SYSTEM_CLOCK_TYPE_COROUTINE 1
#define SYSTEM_CLOCK_TYPE SYSTEM_CLOCK_TYPE_COROUTINE
#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  #define SYSTEM_CLOCK SystemClockLoop
#else
  #define SYSTEM_CLOCK SystemClockCoroutine
#endif

#if defined(EPOXY_DUINO)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define CLK_PIN 10
  #define DIO_PIN 9
  #define BIT_DELAY 100

#elif ! defined(AUNITER)
  #warning Arduino IDE detected. Check config parameters.

  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_SCANNING
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST

#elif defined(AUNITER_LED_CLOCK_HC595_DUAL)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_DUAL
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_LED_CLOCK_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define CLK_PIN 10
  #define DIO_PIN 9
  #define BIT_DELAY 100

#elif defined(AUNITER_LED_CLOCK_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN A0
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_LED_CLOCK_TINY_TM1637)
  #define SERIAL_PORT_MONITOR Serial

  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 0

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define CLK_PIN 3
  #define DIO_PIN 1
  #define BIT_DELAY 100

#else
  #error Unknown AUNITER environment
#endif

//------------------------------------------------------------------
// Button state transition nodes.
//------------------------------------------------------------------

enum class Mode : uint8_t {
  kUnknown = 0, // uninitialized

  kViewHourMinute,
  kViewSecond,
  kViewYear,
  kViewMonth,
  kViewDay,
  kViewWeekday,
  kViewBrightness,

  kChangeYear,
  kChangeMonth,
  kChangeDay,
  kChangeHour,
  kChangeMinute,
  kChangeSecond,
  kChangeWeekday,

  kChangeHourMode,
  kChangeBrightness,
};

#endif
