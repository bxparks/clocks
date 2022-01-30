#ifndef LED_CLOCK_TINY_CONFIG_H
#define LED_CLOCK_TINY_CONFIG_H

#include <stdint.h> // uint8_t

//------------------------------------------------------------------
// Configuration parameters.
//------------------------------------------------------------------

// Set >= 1 to print debugging info to SERIAL_PORT_MONITOR
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// PersistentStore
#define ENABLE_EEPROM 0

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
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_MANUAL

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

// Type of LED display.
#define LED_DISPLAY_TYPE_TM1637 0
#define LED_DISPLAY_TYPE_MAX7219 1
#define LED_DISPLAY_TYPE_HT16K33 2
#define LED_DISPLAY_TYPE_HC595 3
#define LED_DISPLAY_TYPE_DIRECT 4
#define LED_DISPLAY_TYPE_HYBRID 5
#define LED_DISPLAY_TYPE_FULL 6

// Communication interfaces for LED and DS3231 RTC.
// Used by LED_INTERFACE_TYPE and DS3231_INTERFACE_TYPE macros.
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

//-----------------------------------------------------------------------------

// Define an environment when compiling/uploading using Arduino IDE.
#if ! defined(AUNITER) && ! defined(EPOXY_DUINO)
  #define AUNITER_ATTINY_HC595
#endif

#if defined(EPOXY_DUINO)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_TMI_FAST
  #define CLK_PIN 10
  #define DIO_PIN 9
  #define BIT_DELAY 100

#elif defined(AUNITER_ATTINY_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  // Resistor ladder must remain above 90% VCC because they are connected to
  // the RESET button. We have 3 resistors (1k, 10k, 22k):
  //    * 933: 10k/(11k+1k) = 90.9%
  //    * 979: 22k/23k = 95.6%
  //    * 1023: 100%, open
  // Numbers then adjusted using LadderButtonCalibrator.
  #define ANALOG_BUTTON_LEVELS {933, 980, 1024}

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_TMI_FAST
  #define CLK_PIN 3
  #define DIO_PIN 1
  #define BIT_DELAY 100

#elif defined(AUNITER_ATTINY_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  // Resistor ladder must remain above 90% VCC because they are connected to
  // the RESET button. We have 3 resistors (1k, 10k, 22k):
  //    * 933: 10k/(11k+1k) = 90.9%
  //    * 979: 22k/23k = 95.6%
  //    * 1023: 100%, open
  // Numbers then adjusted using LadderButtonCalibrator.
  #define ANALOG_BUTTON_LEVELS {933, 980, 1024}

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_SPI_FAST
  #define LATCH_PIN 4
  #define DATA_PIN 1
  #define CLOCK_PIN 3

#elif defined(AUNITER_ATTINY_HT16K33)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  // Resistor ladder must remain above 90% VCC because they are connected to
  // the RESET button. We have 3 resistors (1k, 10k, 22k):
  //    * 933: 10k/(11k+1k) = 90.9%
  //    * 979: 22k/23k = 95.6%
  //    * 1023: 100%, open
  // Numbers then adjusted using LadderButtonCalibrator.
  #define ANALOG_BUTTON_LEVELS {933, 980, 1024}

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HT16K33
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define HT16K33_I2C_ADDRESS 0x70
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define BIT_DELAY 4

#elif defined(AUNITER_ATTINY_HC595)
  #define BUTTON_TYPE BUTTON_TYPE_ANALOG
  #define MODE_BUTTON_PIN 0
  #define CHANGE_BUTTON_PIN 1
  #define ANALOG_BUTTON_PIN A0
  // Resistor ladder must remain above 90% VCC because they are connected to
  // the RESET button. We have 3 resistors (1k, 10k, 22k):
  //    * 933: 10k/(11k+1k) = 90.9%
  //    * 979: 22k/23k = 95.6%
  //    * 1023: 100%, open
  // Numbers then adjusted using LadderButtonCalibrator.
  #define ANALOG_BUTTON_LEVELS {933, 980, 1024}

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_SPI_FAST
  #define LATCH_PIN 4
  #define DATA_PIN 1
  #define CLOCK_PIN 3

#elif defined(AUNITER_MICRO_TM1637)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_TM1637
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_TMI_FAST
  #define CLK_PIN A0
  #define DIO_PIN 9
  #define BIT_DELAY 100

#elif defined(AUNITER_MICRO_MAX7219)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_MAX7219
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_SPI_FAST
  #define LATCH_PIN 10
  #define DATA_PIN MOSI
  #define CLOCK_PIN SCK

#elif defined(AUNITER_MICRO_HT16K33)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HT16K33
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define HT16K33_I2C_ADDRESS 0x70
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define BIT_DELAY 4

#elif defined(AUNITER_MICRO_HC595)
  #define BUTTON_TYPE BUTTON_TYPE_DIGITAL
  #define MODE_BUTTON_PIN A2
  #define CHANGE_BUTTON_PIN A3

  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define DS3231_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #define SDA_PIN SDA
  #define SCL_PIN SCL
  #define WIRE_BIT_DELAY 4

  #define LED_DISPLAY_TYPE LED_DISPLAY_TYPE_HC595
  #define LED_INTERFACE_TYPE INTERFACE_TYPE_SIMPLE_SPI_FAST
  #define LATCH_PIN 10
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
