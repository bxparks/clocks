#ifndef MED_MINDER_CONFIG_H
#define MED_MINDER_CONFIG_H

//------------------------------------------------------------------
// OLED display configuration
//------------------------------------------------------------------

// OLED address: 0X3C+SA0 - 0x3C or 0x3D
#define OLED_I2C_ADDRESS 0x3C

//------------------------------------------------------------------
// Compile-time selectors and options
//------------------------------------------------------------------

// Options for the time sync provider.
#define TIME_PROVIDER_DS3231 0
#define TIME_PROVIDER_NTP 1
#define TIME_PROVIDER_SYSTEM 2

//------------------------------------------------------------------
// Configuration of target environment. The environment is defined in
// $HOME/.auniter.ini and the AUNITER_XXX macro is set by auniter.sh.
//------------------------------------------------------------------

#if ! defined(AUNITER)
  // Normal Arduino IDE
  #define ENABLE_SERIAL 0
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
#elif defined(AUNITER_MICRO_MINDER)
  #define ENABLE_SERIAL 0
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
#elif defined(AUNITER_MINI_MINDER)
  #define ENABLE_SERIAL 0
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
#elif defined(AUNITER_ESP_MINDER)
  #define ENABLE_SERIAL 1
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
#elif defined(AUNITER_ESP_MINDER2)
  #define ENABLE_SERIAL 1
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_REMAP true
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
#elif defined(AUNITER_ESP32_MINDER)
  #define ENABLE_SERIAL 1
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

static const uint8_t MODE_UNKNOWN = 0; // uninitialized
static const uint8_t MODE_DATE_TIME_MED = 1;
static const uint8_t MODE_TIME_ZONE = 2;
static const uint8_t MODE_ABOUT = 3;

static const uint8_t MODE_CHANGE_YEAR = 10;
static const uint8_t MODE_CHANGE_MONTH = 11;
static const uint8_t MODE_CHANGE_DAY = 12;
static const uint8_t MODE_CHANGE_HOUR = 13;
static const uint8_t MODE_CHANGE_MINUTE = 14;
static const uint8_t MODE_CHANGE_SECOND = 15;

static const uint8_t MODE_CHANGE_MED_HOUR = 20;
static const uint8_t MODE_CHANGE_MED_MINUTE = 21;

static const uint8_t MODE_CHANGE_TIME_ZONE_NAME = 30;

#endif
