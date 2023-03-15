#ifndef COMMAND_LINE_CLOCK_CONTROLLER_H
#define COMMAND_LINE_CLOCK_CONTROLLER_H

#include <AceRoutine.h>
#include <AceTimeClock.h>
#include "config.h"
#include "PersistentStore.h"
#include "StoredInfo.h"

using namespace ace_time;
using namespace ace_time::clock;

class Controller {
  public:
    Controller(SystemClock& systemClock, PersistentStore& persistentStore) :
        mSystemClock(systemClock),
        mPersistentStore(persistentStore)
      #if ENABLE_TIME_ZONE_TYPE_BASIC
        , mBasicZoneManager(
            kBasicZoneRegistrySize,
            kBasicZoneRegistry,
            mBasicZoneProcessorCache)
      #endif
      #if ENABLE_TIME_ZONE_TYPE_EXTENDED
        , mExtendedZoneManager(
            kExtendedZoneRegistrySize,
            kExtendedZoneRegistry,
            mExtendedZoneProcessorCache)
      #endif
    {}

    void setup() {
      mIsStoredInfoValid = mPersistentStore.readStoredInfo(mStoredInfo);

      if (mIsStoredInfoValid) {
        SERIAL_PORT_MONITOR.println(F("Found valid EEPROM info"));
        restoreInfo(mStoredInfo);
      } else {
      #if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
        mStoredInfo.ssid[0] = '\0';
        mStoredInfo.password[0] = '\0';
      #endif
        setBasicTimeZoneForIndex();
      }
    }

    /** Set the time zone to the given offset using kTypeManual. */
    void setManualTimeZone(TimeOffset stdOffset, TimeOffset dstOffset) {
      mTimeZone = TimeZone::forTimeOffset(stdOffset, dstOffset);
      preserveInfo();
    }

    /** Set the DST setting of Manual TimeZone. */
    void setDst(bool isDst) {
      mTimeZone = TimeZone::forTimeOffset(
        mTimeZone.getStdOffset(),
        TimeOffset::forHours(isDst ? 1 : 0));
      preserveInfo();
    }

  #if ENABLE_TIME_ZONE_TYPE_BASIC
    /** Set the time zone to America/Los_Angeles using BasicZoneProcessor. */
    void setBasicTimeZoneForIndex(uint16_t zoneIndex = 0) {
      SERIAL_PORT_MONITOR.print(F("setBasicTimeZoneForIndex(): "));
      SERIAL_PORT_MONITOR.println(zoneIndex);
      mTimeZone = mBasicZoneManager.createForZoneIndex(zoneIndex);
      validateAndSaveTimeZone();
    }
  #endif

  #if ENABLE_TIME_ZONE_TYPE_EXTENDED
    /** Set the time zone to America/Los_Angeles using ExtendedZoneProcessor. */
    void setExtendedTimeZoneForIndex(uint16_t zoneIndex = 0) {
      SERIAL_PORT_MONITOR.print(F("setExtendedTimeZoneForIndex(): "));
      SERIAL_PORT_MONITOR.println(zoneIndex);
      mTimeZone = mExtendedZoneManager.createForZoneIndex(zoneIndex);
      validateAndSaveTimeZone();
    }
  #endif

    /** Return the current time zone. */
    TimeZone& getTimeZone() { return mTimeZone; }

  #if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
    /** Set the wifi credentials and setup the NtpClock. */
    void setWiFi(const char* ssid, const char* password) {
      strncpy(mStoredInfo.ssid, ssid, StoredInfo::kSsidMaxLength);
      mStoredInfo.ssid[StoredInfo::kSsidMaxLength - 1] = '\0';
      strncpy(mStoredInfo.password, password, StoredInfo::kPasswordMaxLength);
      mStoredInfo.password[StoredInfo::kPasswordMaxLength - 1] = '\0';
      preserveInfo();
    }
  #endif

    /** Set the current time of the system clock. */
    void setNow(acetime_t now) {
      mSystemClock.setNow(now);
    }

    /** Return the current time from the system clock. */
    ZonedDateTime getCurrentDateTime() const {
      return ZonedDateTime::forEpochSeconds(
          mSystemClock.getNow(), mTimeZone);
    }

    /** Return true if the initial setup() retrieved a valid storedInfo. */
    bool isStoredInfoValid() const { return mIsStoredInfoValid; }

    /** Return the stored info. */
    const StoredInfo& getStoredInfo() const { return mStoredInfo; }

    /** Return DST mode. */
    bool isDst() const { return mTimeZone.isDst(); }

    /** Force SystemClock to sync. */
    void forceSync() {
      mSystemClock.forceSync();
    }

  #if ENABLE_TIME_ZONE_TYPE_BASIC
    /** Print list of supported zones. */
    void printBasicZonesTo(Print& printer) {
      for (uint16_t i = 0; i < mBasicZoneManager.zoneRegistrySize(); i++) {
        printer.print('[');
        printer.print(i);
        printer.print(']');
        printer.print(' ');
        TimeZone tz = mBasicZoneManager.createForZoneIndex(i);
        tz.printTo(printer);
        printer.println();
      }
    }
  #endif

  #if ENABLE_TIME_ZONE_TYPE_EXTENDED
    /** Print list of supported zones. */
    void printExtendedZonesTo(Print& printer) {
      for (uint16_t i = 0; i < mExtendedZoneManager.zoneRegistrySize(); i++) {
        printer.print('[');
        printer.print(i);
        printer.print(']');
        printer.print(' ');
        TimeZone tz = mExtendedZoneManager.createForZoneIndex(i);
        tz.printTo(printer);
        printer.println();
      }
    }
  #endif

  private:
  #if ENABLE_TIME_ZONE_TYPE_BASIC
    static const basic::ZoneInfo* const kBasicZoneRegistry[] ACE_TIME_PROGMEM;
    static const uint16_t kBasicZoneRegistrySize;
  #endif
  #if ENABLE_TIME_ZONE_TYPE_EXTENDED
    static const extended::ZoneInfo* const kExtendedZoneRegistry[]
        ACE_TIME_PROGMEM;
    static const uint16_t kExtendedZoneRegistrySize;
  #endif

    void validateAndSaveTimeZone() {
      if (mTimeZone.isError()) {
        mTimeZone = mBasicZoneManager.createForZoneInfo(
            &zonedb::kZoneAmerica_Los_Angeles);
      }
      preserveInfo();
    }

    uint16_t preserveInfo() {
      SERIAL_PORT_MONITOR.println(F("preserveInfo()"));
      mIsStoredInfoValid = true;
      mStoredInfo.timeZoneData = mTimeZone.toTimeZoneData();
      return mPersistentStore.writeStoredInfo(mStoredInfo);
    }

    void restoreInfo(const StoredInfo& storedInfo) {
      SERIAL_PORT_MONITOR.print(F("restoreInfo(): type="));
      SERIAL_PORT_MONITOR.println(storedInfo.timeZoneData.type);
      #if ENABLE_TIME_ZONE_TYPE_BASIC
        mTimeZone = mBasicZoneManager.createForTimeZoneData(
            storedInfo.timeZoneData);
      #elif ENABLE_TIME_ZONE_TYPE_EXTENDED
        mTimeZone = mExtendedZoneManager.createForTimeZoneData(
            storedInfo.timeZoneData);
      #else
        mTimeZone = mManualZoneManager.createForTimeZoneData(
            storedInfo.timeZoneData);
      #endif

      validateAndSaveTimeZone();
    }

    SystemClock& mSystemClock;
    PersistentStore& mPersistentStore;

  #if ENABLE_TIME_ZONE_TYPE_BASIC
    BasicZoneProcessorCache<1> mBasicZoneProcessorCache;
    BasicZoneManager mBasicZoneManager;
  #endif
  #if ENABLE_TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneProcessorCache<1> mExtendedZoneProcessorCache;
    ExtendedZoneManager mExtendedZoneManager;
  #endif
  #if ! ENABLE_TIME_ZONE_TYPE_BASIC && ! ENABLE_TIME_ZONE_TYPE_EXTENDED
    ManualZoneManager mManualZoneManager;
  #endif

    TimeZone mTimeZone;
    StoredInfo mStoredInfo;
    bool mIsStoredInfoValid = false;
};

#endif
