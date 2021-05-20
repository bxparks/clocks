#ifndef LED_CLOCK_TINY_PERSISTENT_STORE_H
#define LED_CLOCK_TINY_PERSISTENT_STORE_H

#include "config.h"
#include "StoredInfo.h"

#if ENABLE_EEPROM

#include <AceUtilsCrcEeprom.h>

#if defined(EPOXY_DUINO)
  #include <EpoxyEepromEsp.h>
  using ace_utils::crc_eeprom::CrcEepromEsp;
#elif defined(ARDUINO_ARCH_AVR)
  #include <EEPROM.h>
  using ace_utils::crc_eeprom::CrcEepromAvr;
#elif defined(ESP32) || defined(ESP8266)
  #include <EEPROM.h>
  using ace_utils::crc_eeprom::CrcEepromEsp;
#elif defined(ARDUINO_ARCH_STM32)
  #include <AceUtilsBufferedEepromStm32.h>
  using ace_utils::crc_eeprom::CrcEepromEsp;
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
    PersistentStore(uint32_t contextId, uint16_t address) :
      mAddress(address),
      #if defined(EPOXY_DUINO)
        mCrcEeprom(EpoxyEepromEspInstance, contextId)
      #elif defined(ARDUINO_ARCH_AVR)
        mCrcEeprom(EEPROM, contextId)
      #elif defined(ESP32) || defined(ESP8266)
        mCrcEeprom(EEPROM, contextId)
      #elif defined(ARDUINO_ARCH_STM32)
        mCrcEeprom(BufferedEEPROM, contextId)
      #endif
    {}

    void setup() {
    #if defined(EPOXY_DUINO)
      EpoxyEepromEspInstance.begin(
          ace_utils::crc_eeprom::toSavedSize(sizeof(StoredInfo)));
    #elif defined(ARDUINO_ARCH_AVR)
      // no setup required
    #elif defined(ESP32) || defined(ESP8266)
      EEPROM.begin(
          ace_utils::crc_eeprom::toSavedSize(sizeof(StoredInfo)));
    #elif defined(ARDUINO_ARCH_STM32)
      BufferedEEPROM.begin();
    #endif
    }

    bool readStoredInfo(StoredInfo& storedInfo) const {
      return mCrcEeprom.readWithCrc(mAddress, storedInfo);
    }

    uint16_t writeStoredInfo(const StoredInfo& storedInfo) {
      return mCrcEeprom.writeWithCrc(mAddress, storedInfo);
    }

  private:
    uint16_t const mAddress;

  #if defined(EPOXY_DUINO)
    CrcEepromEsp<EpoxyEepromEsp> mCrcEeprom;
  #elif defined(ARDUINO_ARCH_AVR)
    CrcEepromAvr<EEPROMClass> mCrcEeprom;
  #elif defined(ESP32) || defined(ESP8266)
    CrcEepromEsp<EEPROMClass> mCrcEeprom;
  #elif defined(ARDUINO_ARCH_STM32)
    CrcEepromEsp<BufferedEEPROMClass> mCrcEeprom;
  #endif
};

#else

/** A thin layer around a CrcEeprom object to handle controllers w/o EEPROM. */
class PersistentStore {
  public:
    PersistentStore(uint32_t, uint16_t) {}

    void setup() {}

    bool readStoredInfo(StoredInfo&) const { return false; }

    uint16_t writeStoredInfo(const StoredInfo&) { return 0; }
};

#endif // ENABLE_EEPROM

#endif
