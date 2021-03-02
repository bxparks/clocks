#ifndef COMMAND_LINE_CLOCK_PERSISTENT_STORE_H
#define COMMAND_LINE_CLOCK_PERSISTENT_STORE_H

#include <AceTime.h>
#if ENABLE_EEPROM
  #include <AceUtilsCrcEeprom.h>
#endif
#include "config.h"
#include "StoredInfo.h"

using namespace ace_time;
#if ENABLE_EEPROM
using ace_utils::crc_eeprom::CrcEeprom;
#endif

class PersistentStore {
  public:
  #if ENABLE_EEPROM
    PersistentStore()
      : mCrcEeprom(CrcEeprom::toContextId('c', 'c', 'l', 'k'))
    {}
  #endif

    void setup() {
    #if ENABLE_EEPROM
      mCrcEeprom.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    #endif
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
    #if ENABLE_EEPROM
      bool isValid = mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
      #if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
        storedInfo.ssid[StoredInfo::kSsidMaxLength - 1] = '\0';
        storedInfo.password[StoredInfo::kPasswordMaxLength - 1] = '\0';
      #endif
      return isValid;
    #else
      (void) storedInfo; // disable compiler warning
      return false;
    #endif
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) const {
    #if ENABLE_EEPROM
      return mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, &storedInfo,
          sizeof(StoredInfo));
    #else
      (void) storedInfo; // disable compiler warning
      return 0;
    #endif
    }

  private:
  #if ENABLE_EEPROM
    static const uint16_t kStoredInfoEepromAddress = 0;

    CrcEeprom mCrcEeprom;
  #endif
};

#endif
