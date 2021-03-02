#ifndef WORLD_CLOCK_LCD_PERSISTENT_STORE_H
#define WORLD_CLOCK_LCD_PERSISTENT_STORE_H

#include <AceTime.h>
#if ! defined(ARDUINO_ARCH_SAMD)
  #include <AceUtilsCrcEeprom.h>
#endif
#include "config.h"
#include "StoredInfo.h"

using namespace ace_time;
using ace_utils::crc_eeprom::CrcEeprom;

/** A thin layer around a CrcEeprom object. */
class PersistentStore {
  public:
  #if ! defined(ARDUINO_ARCH_SAMD)
    PersistentStore()
      : mCrcEeprom(CrcEeprom::toContextId('w', 'l', 'c', 'd'))
    {}
  #endif

    void setup() {
    #if ! defined(ARDUINO_ARCH_SAMD)
      mCrcEeprom.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    #endif
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
    #if defined(ARDUINO_ARCH_SAMD)
      (void) storedInfo; // disable compiler warning
      return false;
    #else
      return mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
    #endif
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) const {
    #if defined(ARDUINO_ARCH_SAMD)
      (void) storedInfo; // disable compiler warning
      return 0;
    #else
      return mCrcEeprom.writeWithCrc(
          kStoredInfoEepromAddress,
          &storedInfo,
          sizeof(StoredInfo));
    #endif
    }

  private:
  #if ! defined(ARDUINO_ARCH_SAMD)
    static const uint16_t kStoredInfoEepromAddress = 0;

    CrcEeprom mCrcEeprom;
  #endif
};

#endif
