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
//  * AVR: 10-bit analog pin
//  * ESP8266: 10-bit analog pin
//  * ESP32: 12-bit analog pin
#define BUTTON_TYPE_DIGITAL 0
#define BUTTON_TYPE_ANALOG 1

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
#define LED_DISPLAY_TYPE_MAX7219 2
#define LED_DISPLAY_TYPE_DIRECT 3
#define LED_DISPLAY_TYPE_HC595_SINGLE 4
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

#elif defined(AUNITER_MICRO_CUSTOM_DIRECT)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_DIRECT
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST

#elif defined(AUNITER_MICRO_CUSTOM_SINGLE)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_SINGLE
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_MICRO_CUSTOM_DUAL)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_DUAL
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK
  #define HC595_BYTE_ORDER ace_segment::kByteOrderDigitHighSegmentLow
  #define REMAP_ARRAY nullptr
  #define SEGMENT_ON_PATTERN LedMatrixBase::kActiveLowPattern
  #define DIGIT_ON_PATTERN LedMatrixBase::kActiveLowPattern

#elif defined(AUNITER_MICRO_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define CLK_PIN A0
  #define DIO_PIN 9
  #define BIT_DELAY 100

#elif defined(AUNITER_MICRO_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_MICRO_HC595)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_DUAL
  #define HC595_BYTE_ORDER ace_segment::kByteOrderSegmentHighDigitLow
  #define REMAP_ARRAY ace_segment::kDigitRemapArray8Hc595
  #define SEGMENT_ON_PATTERN LedMatrixBase::kActiveLowPattern
  #define DIGIT_ON_PATTERN LedMatrixBase::kActiveHighPattern

  #define INTERFACE_TYPE INTERFACE_TYPE_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_STM32_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN PA0
  #define CHANGE_BUTTON_PIN PA1

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_STM32F1RTC

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define CLK_PIN PB3
  #define DIO_PIN PB4
  #define BIT_DELAY 100

#elif defined(AUNITER_STM32_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN PA0
  #define CHANGE_BUTTON_PIN PA1

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_STM32F1RTC

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define LATCH_PIN SS
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_STM32_HC595)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN PA0
  #define CHANGE_BUTTON_PIN PA1

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_STM32F1RTC

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_DUAL
  #define HC595_BYTE_ORDER ace_segment::kByteOrderSegmentHighDigitLow
  #define REMAP_ARRAY ace_segment::kDigitRemapArray8Hc595
  #define SEGMENT_ON_PATTERN LedMatrixBase::kActiveLowPattern
  #define DIGIT_ON_PATTERN LedMatrixBase::kActiveHighPattern

  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define LATCH_PIN SS
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_D1MINI_LARGE_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define CLK_PIN D5
  #define DIO_PIN D7
  #define BIT_DELAY 100

#elif defined(AUNITER_D1MINI_LARGE_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define LATCH_PIN D8
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_D1MINI_LARGE_HC595)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 2
  #define ANALOG_BUTTON_COUNT 4
  #define ANALOG_BUTTON_PIN A0

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595_DUAL
  #define HC595_BYTE_ORDER ace_segment::kByteOrderSegmentHighDigitLow
  #define REMAP_ARRAY ace_segment::kDigitRemapArray8Hc595
  #define SEGMENT_ON_PATTERN LedMatrixBase::kActiveLowPattern
  #define DIGIT_ON_PATTERN LedMatrixBase::kActiveHighPattern

  #define INTERFACE_TYPE INTERFACE_TYPE_NORMAL
  #define LATCH_PIN D8
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK
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
  kViewSecond,
  kViewYear,
  kViewMonth,
  kViewDay,
  kViewWeekday,
  kViewTimeZone,
  kViewBrightness,

  kChangeYear,
  kChangeMonth,
  kChangeDay,
  kChangeHour,
  kChangeMinute,
  kChangeSecond,

  kChangeTimeZoneOffset,
  kChangeTimeZoneDst,
  kChangeHourMode,

  kChangeBrightness,
};

#endif
