#ifndef OLED_CLOCK_PERSISTENT_STORE_H
#define OLED_CLOCK_PERSISTENT_STORE_H

#include <AceTime.h>
#if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
  #include <AceUtilsCrcEeprom.h>
  using ace_utils::crc_eeprom::CrcEeprom;
#endif
#include "config.h"
#include "StoredInfo.h"

using namespace ace_time;

class PersistentStore {
  public:
  #if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
    PersistentStore()
      : mCrcEeprom(CrcEeprom::toContextId('o', 'c', 'l', 'k'))
    {}
  #endif

    void setup() {
    #if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
      mCrcEeprom.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    #endif
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
    #if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
      return mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
    #else
      (void) storedInfo; // disable compiler warning
      return false;
    #endif
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) const {
    #if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
      return mCrcEeprom.writeWithCrc(
          kStoredInfoEepromAddress,
          &storedInfo,
          sizeof(StoredInfo));
    #else
      (void) storedInfo; // disable compiler warning
      return 0;
    #endif
    }

  private:
  #if defined(ARDUINO_ARCH_AVR) || defined(ESP8266) || defined(ESP32)
    static const uint16_t kStoredInfoEepromAddress = 0;

    CrcEeprom mCrcEeprom;
  #endif
};

#endif
