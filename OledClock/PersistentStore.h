#ifndef OLED_CLOCK_PERSISTENT_STORE_H
#define OLED_CLOCK_PERSISTENT_STORE_H

#include "config.h"
#include "StoredInfo.h"

#if ENABLE_EEPROM

#include <AceUtilsCrcEeprom.h>
using ace_utils::crc_eeprom::CrcEeprom;
using ace_utils::crc_eeprom::IEepromAdapter;

/** A thin layer around a CrcEeprom object to handle controllers w/o EEPROM. */
class PersistentStore {
  public:
    PersistentStore(IEepromAdapter& eepromAdapter)
      : mCrcEeprom(eepromAdapter, CrcEeprom::toContextId('o', 'c', 'l', 'k'))
    {}

    void setup() {
      mCrcEeprom.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
      return mCrcEeprom.readWithCrc(kStoredInfoEepromAddress, storedInfo);
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) {
      return mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, storedInfo);
    }

  private:
    static const uint16_t kStoredInfoEepromAddress = 0;

    CrcEeprom mCrcEeprom;
};

#else

/** A thin layer around a CrcEeprom object to handle controllers w/o EEPROM. */
class PersistentStore {
  public:
    PersistentStore(IEepromAdapter& eepromAdapter) {
      (void) eepromAdapter; // disable compiler warning
    }

    void setup() {}

    bool readStoredInfo(StoredInfo& storedInfo) const {
      (void) storedInfo; // disable compiler warning
      return false;
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) {
      (void) storedInfo; // disable compiler warning
      return 0;
    }
};

#endif // ENABLE_EEPROM

#endif
