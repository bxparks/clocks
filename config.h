#ifndef MED_MINDER_CONFIG_H
#define MED_MINDER_CONFIG_H

//------------------------------------------------------------------
// Compile-time selectors and options
//------------------------------------------------------------------

// Needed by ESP32 chips. Has no effect on other chips.
// Should be bigger than (sizeof(crc32) + sizeof(StoredInfo)).
#define EEPROM_SIZE 32

// Options for the time sync provider.
#define TIME_PROVIDER_DS3231 0
#define TIME_PROVIDER_NTP 1
#define TIME_PROVIDER_SYSTEM 2

// Options for the display: Adafruit128x64 or Adafruit128x32
#define OLED_DIPLAY_ADA32 0
#define OLED_DIPLAY_ADA64 1

//------------------------------------------------------------------
// Configuration of target environment. The environment is defined in
// $HOME/.auniter.ini and the AUNITER_XXX macro is set by auniter.sh.
//------------------------------------------------------------------

#if !defined(AUNITER)
  #define AUNITER_MICRO_MINDER
  #warning Defaulting to AUNITER_MICRO_MINDER
#endif

#if defined(AUNITER_MICRO_MINDER)
  #define ENABLE_SERIAL 0
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_DISPLAY OLED_DIPLAY_ADA32
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 8
  #define CHANGE_BUTTON_PIN 9
#elif defined(AUNITER_MINI_MINDER)
  #define ENABLE_SERIAL 0
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_DS3231
  #define OLED_DISPLAY OLED_DIPLAY_ADA64
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 3
#elif defined(AUNITER_ESP_MINDER)
  #define ENABLE_SERIAL 1
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_DISPLAY OLED_DIPLAY_ADA64
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
#elif defined(AUNITER_ESP_MINDER2)
  #define ENABLE_SERIAL 1
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_DISPLAY OLED_DIPLAY_ADA64
  #define OLED_REMAP true
  #define MODE_BUTTON_PIN D4
  #define CHANGE_BUTTON_PIN D3
#elif defined(AUNITER_ESP32_MINDER)
  #define ENABLE_SERIAL 1
  #define ENABLE_LOW_POWER 0
  #define TIME_PROVIDER TIME_PROVIDER_NTP
  #define OLED_DISPLAY OLED_DIPLAY_ADA64
  #define OLED_REMAP false
  #define MODE_BUTTON_PIN 2
  #define CHANGE_BUTTON_PIN 4
#else
  #error Unsupported AUNITER environment
#endif

#endif
