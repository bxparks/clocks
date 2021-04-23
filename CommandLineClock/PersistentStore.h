#ifndef COMMAND_LINE_CLOCK_PERSISTENT_STORE_H
#define COMMAND_LINE_CLOCK_PERSISTENT_STORE_H

#include "config.h"
#include "StoredInfo.h"

#if ENABLE_EEPROM

#include <AceUtilsCrcEeprom.h>
using ace_utils::crc_eeprom::EepromInterface;
using ace_utils::crc_eeprom::CrcEeprom;

#if defined(EPOXY_DUINO)
  #include <EpoxyEepromEsp.h>
  using ace_utils::crc_eeprom::EspStyleEeprom;

#elif defined(ESP32) || defined(ESP8266)
  #include <EEPROM.h>
  using ace_utils::crc_eeprom::EspStyleEeprom;

#elif defined(ARDUINO_ARCH_AVR)
  #include <EEPROM.h>
  using ace_utils::crc_eeprom::AvrStyleEeprom;

#elif defined(ARDUINO_ARCH_STM32)
  #include <AceUtilsBufferedEepromStm32.h>
  using ace_utils::crc_eeprom::EspStyleEeprom;

#else
  #error Unsupported platform for EEPROM

#endif

/**
 * A abstraction that knows how to store a StoreInfo object into a platform's
 * EEPROM, using the CrcEeprom class to validate its CRC. This class knows how
 * to wrap and configure different collaborating objects on different platforms.
 */
class PersistentStore {
  public:
    PersistentStore() :
      #if defined(EPOXY_DUINO)
        mEepromInterface(EpoxyEepromEspInstance),
      #elif defined(ESP32) || defined(ESP8266) 
        mEepromInterface(EEPROM),
      #elif defined(ARDUINO_ARCH_AVR)
        mEepromInterface(EEPROM),
      #elif defined(ARDUINO_ARCH_STM32)
        mEepromInterface(BufferedEEPROM),
      #endif
        mCrcEeprom(mEepromInterface, CrcEeprom::toContextId('c', 'c', 'l', 'k'))
    {}

    void setup() {
    #if defined(EPOXY_DUINO)
      EpoxyEepromEspInstance.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    #elif defined(ESP32) || defined(ESP8266)
      EEPROM.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
    #elif defined(ARDUINO_ARCH_AVR)
      // no setup required
    #elif defined(ARDUINO_ARCH_STM32)
      BufferedEEPROM.begin();
    #endif
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

  #if defined(EPOXY_DUINO)
    EspStyleEeprom<EpoxyEepromEsp> mEepromInterface;
  #elif defined(ESP32) || defined(ESP8266) 
    EspStyleEeprom<EEPROMClass> mEepromInterface;
  #elif defined(ARDUINO_ARCH_AVR)
    AvrStyleEeprom<EEPROMClass> mEepromInterface;
  #elif defined(ARDUINO_ARCH_STM32)
    EspStyleEeprom<BufferedEEPROMClass> mEepromInterface;
  #endif

    CrcEeprom mCrcEeprom;
};

#else

/** A thin layer around a CrcEeprom object to handle controllers w/o EEPROM. */
class PersistentStore {
  public:
    PersistentStore() = default;

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
