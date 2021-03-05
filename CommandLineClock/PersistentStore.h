#ifndef COMMAND_LINE_CLOCK_PERSISTENT_STORE_H
#define COMMAND_LINE_CLOCK_PERSISTENT_STORE_H

#include "config.h"
#include "StoredInfo.h"

#if ENABLE_EEPROM

#include <AceUtilsCrcEeprom.h>
using ace_utils::crc_eeprom::IEepromAdapter;
using ace_utils::crc_eeprom::CrcEeprom;

class PersistentStore {
  public:
    PersistentStore(IEepromAdapter& eepromAdapter)
      : mCrcEeprom(eepromAdapter, CrcEeprom::toContextId('c', 'c', 'l', 'k'))
    {}

    void setup() {
      mCrcEeprom.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
      bool isValid = mCrcEeprom.readWithCrc(
          kStoredInfoEepromAddress, storedInfo);
      #if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
        storedInfo.ssid[StoredInfo::kSsidMaxLength - 1] = '\0';
        storedInfo.password[StoredInfo::kPasswordMaxLength - 1] = '\0';
      #endif
      return isValid;
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
