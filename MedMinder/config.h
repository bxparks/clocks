#ifndef MED_MINDER_CONFIG_H
#define MED_MINDER_CONFIG_H

//------------------------------------------------------------------
// Compile-time selectors and options
//------------------------------------------------------------------

#define MED_MINDER_VERSION_STRING "0.1"

// Set to 1 to print debugging info to SERIAL_PORT_MONITOR
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 0
#endif

// OLED address: 0X3C+SA0 - 0x3C or 0x3D
#define OLED_I2C_ADDRESS 0x3C

// Options for the time sync provider.
#define TIME_PROVIDER_DS3231 0
#define TIME_PROVIDER_NTP 1
#define TIME_PROVIDER_SYSTEM 2

// Determine type of TimeZone to use: Manual, Basic or Extended.
//    * Manual will work on all microcontrollers
//    * Basic is too big for the Pro Micro
//    * Extended is too big for the Pro Micro and Nano
#define TIME_ZONE_TYPE_MANUAL 0
#define TIME_ZONE_TYPE_BASIC 1
#define TIME_ZONE_TYPE TIME_ZONE_TYPE_BASIC

// Maximum Medication interval in hours
#define MAX_MED_INTERVAL_HOURS 36

//------------------------------------------------------------------
// Configuration of target environment. The environment is defined in
// $HOME/.auniter.ini and the AUNITER_XXX macro is set by auniter.sh.
//------------------------------------------------------------------

#if ! defined(AUNITER) // Arduino IDE in interactive mode
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9

#elif defined(EPOXY_DUINO)
  // These are sensitive information. DO NOT UPLOAD TO PUBLIC REPOSITORY.
  #define WIFI_SSID "your wifi ssid here"
  #define WIFI_PASSWORD "your wifi password here"

  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9

#elif defined(AUNITER_NANO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
#elif defined(AUNITER_MICRO)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #undef TIME_ZONE_TYPE
  #define TIME_ZONE_TYPE TIME_ZONE_TYPE_MANUAL
  #define ENABLE_LOW_POWER 1
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP true
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
#elif defined(AUNITER_MED_MINDER8)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_LOW_POWER 1
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
#elif defined(AUNITER_ESP8266)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
#elif defined(AUNITER_ESP32)
  // Defined by auniter.ini
  //#define WIFI_SSID
  //#define WIFI_PASSWORD

  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 4
#else
  #error Unsupported AUNITER environment
#endif

//------------------------------------------------------------------
// Constants for button and UI states.
//------------------------------------------------------------------

// View modes
static const uint8_t MODE_UNKNOWN = 0; // uninitialized
static const uint8_t MODE_VIEW_MED = 1;
static const uint8_t MODE_VIEW_DATE_TIME = 2;
static const uint8_t MODE_VIEW_TIME_ZONE = 3;
static const uint8_t MODE_VIEW_ABOUT = 4;

// Change Med info
static const uint8_t MODE_CHANGE_MED_HOUR = 10;
static const uint8_t MODE_CHANGE_MED_MINUTE = 11;

// Change date/time modes
static const uint8_t MODE_CHANGE_YEAR = 20;
static const uint8_t MODE_CHANGE_MONTH = 21;
static const uint8_t MODE_CHANGE_DAY = 22;
static const uint8_t MODE_CHANGE_HOUR = 23;
static const uint8_t MODE_CHANGE_MINUTE = 24;
static const uint8_t MODE_CHANGE_SECOND = 25;


#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
static const uint8_t MODE_CHANGE_TIME_ZONE_OFFSET = 30;
static const uint8_t MODE_CHANGE_TIME_ZONE_DST = 31;
#else
static const uint8_t MODE_CHANGE_TIME_ZONE_NAME = 30;
#endif

#endif
