#ifndef WORLD_CLOCK_LCD_PERSISTENT_STORE_H
#define WORLD_CLOCK_LCD_PERSISTENT_STORE_H

#include <AceTime.h>
#if ! defined(ARDUINO_ARCH_SAMD)
  #include <CrcEeprom.h>
#endif
#include "config.h"
#include "StoredInfo.h"

using namespace ace_time;
using crc_eeprom::CrcEeprom;

class PersistentStore {
  public:
    void setup() {
    #if ! defined(ARDUINO_ARCH_SAMD)
      // Needed for ESP32
      mCrcEeprom.begin(kEepromSize);
    #endif
    }

  #if defined(ARDUINO_ARCH_SAMD)
    bool readStoredInfo(StoredInfo& /*storedInfo*/) const {
      return false;
    }
  #else
    bool readStoredInfo(StoredInfo& storedInfo) const {
      return mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
    }
  #endif

  #if defined(ARDUINO_ARCH_SAMD)
    uint16_t writeStoredInfo(const StoredInfo& /*storedInfo*/) const {
      return 0;
    }
  #else
    uint16_t writeStoredInfo(const StoredInfo& storedInfo) const {
      return mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, &storedInfo,
          sizeof(StoredInfo));
    }
  #endif

  private:
  #if ! defined(ARDUINO_ARCH_SAMD)
    static const uint16_t kStoredInfoEepromAddress = 0;

    // Must be >= (sizeof(StoredInfo) + 4).
    static const uint8_t kEepromSize = sizeof(StoredInfo) + 4;

    CrcEeprom mCrcEeprom;
  #endif
};

#endif
