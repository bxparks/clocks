#ifndef LED_CLOCK_CONFIG_H
#define LED_CLOCK_CONFIG_H

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

// Options that determine that type of LED display.
#define LED_MODULE_DIRECT 0
#define LED_MODULE_SEGMENT_SERIAL 1
#define LED_MODULE_DUAL_SERIAL 2

#if defined(EPOXY_DUINO)
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define LATCH_PIN SS
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK
  #define LED_DIGIT_PINS {4, 5, 6, 7}
  #define LED_SEGMENT_PINS {8, 9, 10, 11, 12, 14, 15, 13}
  #define LED_MODULE_TYPE LED_MODULE_DIRECT
#elif !defined(AUNITER)
  #warning Arduino IDE detected. Check config parameters.
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define LATCH_PIN SS
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK
  #define LED_DIGIT_PINS {4, 5, 6, 7}
  #define LED_SEGMENT_PINS {8, 9, 10, 11, 12, 14, 15, 13}
  #define LED_MODULE_TYPE LED_MODULE_DIRECT
#elif defined(AUNITER_LED_CLOCK)
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK
  #define LED_DIGIT_PINS {4, 5, 6, 7}
  #define LED_SEGMENT_PINS {8, 9, 10, 16, 14, 18, 19, 15}
  #define LED_MODULE_TYPE LED_MODULE_DUAL_SERIAL
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
