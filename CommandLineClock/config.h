#ifndef COMMAND_LINE_CLOCK_CONFIG_H
#define COMMAND_LINE_CLOCK_CONFIG_H

#include <stdint.h> // uint8_t

//---------------------------------------------------------------------------
// Configuration parameters.
//---------------------------------------------------------------------------

// List of clock types for the referenceClock and backupClock of the
// SystemClock. Used as the value of the TIME_SOURCE_TYPE and
// BACKUP_TIME_SOURCE_TYPE macros.
#define TIME_SOURCE_TYPE_NONE 0
#define TIME_SOURCE_TYPE_DS3231 1
#define TIME_SOURCE_TYPE_NTP 2
#define TIME_SOURCE_TYPE_ESP_SNTP 3
#define TIME_SOURCE_TYPE_STMRTC 4
#define TIME_SOURCE_TYPE_STM32F1RTC 5
#define TIME_SOURCE_TYPE_UNIX 6

// Determine the type of SystemClock.
#define SYNC_TYPE_LOOP 0
#define SYNC_TYPE_COROUTINE 1
#define SYNC_TYPE SYNC_TYPE_LOOP

// ENABLE_TIME_ZONE_TYPE_BASIC and ENABLE_TIME_ZONE_TYPE_EXTENDED determine
// which ZoneProcessor to support. Small boards like the Pro Micro (30kB
// flash/2.5kB RAM) cannot support both BasicZoneProcessor and
// ExtendedZoneProcessor at the same time.

#if defined(EPOXY_DUINO)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_UNIX
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_NONE
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif ! defined(AUNITER) // Arduino IDE in interactive mode
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_NANO)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_MICRO)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 0
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_MEGA)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_SAMD)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 0
#elif defined(AUNITER_STM32)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_ESP8266)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_NTP
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#elif defined(AUNITER_ESP32)
  #define TIME_SOURCE_TYPE TIME_SOURCE_TYPE_NTP
  #define BACKUP_TIME_SOURCE_TYPE TIME_SOURCE_TYPE_DS3231
  #define ENABLE_TIME_ZONE_TYPE_BASIC 1
  #define ENABLE_TIME_ZONE_TYPE_EXTENDED 1
  #define ENABLE_EEPROM 1
#else
  #error Unknown AUNITER environment
#endif

#endif
