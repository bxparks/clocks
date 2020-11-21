#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <CrcEeprom.h>
#include "config.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"
#include "ClockInfo.h"

using namespace ace_time;
using namespace ace_time::clock;
using crc_eeprom::CrcEeprom;

/**
 * Class responsible for handling user button presses, and determining
 * how various member variables are updated. The rendering of the various
 * variables are handled by the Presenter. If analyzed using an MVC
 * architecture, this is the Controller, the Presenter is the View, and the
 * various member variables of this class are the Model objects.
 */
class Controller {
  public:
    static const uint16_t kStoredInfoEepromAddress = 0;

    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    // Number of minutes to use for a DST offset.
    static const int16_t kDstOffsetMinutes = 60;

    /** Constructor. */
    Controller(SystemClock& clock, CrcEeprom& crcEeprom,
          Presenter& presenter):
        mClock(clock),
        mCrcEeprom(crcEeprom),
        mPresenter(presenter)
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        , mZoneManager(kZoneRegistrySize, kZoneRegistry)
      #endif
        , mMode(MODE_UNKNOWN) {}

    void setup() {
      // Retrieve current time from Clock.
      acetime_t nowSeconds = mClock.getNow();

      // Restore state from EEPROM if valid.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
      if (isValid) {
        if (ENABLE_SERIAL_DEBUG == 1) {
          SERIAL_PORT_MONITOR.println(F("setup(): valid StoredInfo"));
        }
        restoreClockInfo(mClockInfo, storedInfo);
      } else {
        if (ENABLE_SERIAL_DEBUG == 1) {
          SERIAL_PORT_MONITOR.println(
              F("setup(): invalid StoredInfo; initializing"));
        }
        setupClockInfo(nowSeconds);
      }

      // Set the current date time using the mClockInfo.timeZone.
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          nowSeconds, mClockInfo.timeZone);

      // Start with the medInfo view mode.
      mMode = MODE_VIEW_MED;
    }

    void syncClock() {
      mClock.forceSync();
      acetime_t nowSeconds = mClock.getNow();
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          nowSeconds, mClockInfo.timeZone);
    }

    /** Wake from sleep. */
    void wakeup() {
      mIsPreparingToSleep = false;
      mPresenter.wakeup();
      syncClock(); // sync from reference clock
    }

    /** Prepare to sleep. */
    void prepareToSleep() {
      mIsPreparingToSleep = true;
      mPresenter.prepareToSleep();
    }

    void modeButtonPress()  {
      switch (mMode) {
        case MODE_VIEW_MED:
          mMode = MODE_VIEW_DATE_TIME;
          break;
        case MODE_VIEW_DATE_TIME:
          mMode = MODE_VIEW_TIME_ZONE;
          break;
        case MODE_VIEW_TIME_ZONE:
          mMode = MODE_VIEW_ABOUT;
          break;
        case MODE_VIEW_ABOUT:
          mMode = MODE_VIEW_MED;
          break;

        case MODE_CHANGE_MED_HOUR:
          mMode = MODE_CHANGE_MED_MINUTE;
          break;
        case MODE_CHANGE_MED_MINUTE:
          mMode = MODE_CHANGE_MED_HOUR;
          break;

        case MODE_CHANGE_YEAR:
          mMode = MODE_CHANGE_MONTH;
          break;
        case MODE_CHANGE_MONTH:
          mMode = MODE_CHANGE_DAY;
          break;
        case MODE_CHANGE_DAY:
          mMode = MODE_CHANGE_HOUR;
          break;
        case MODE_CHANGE_HOUR:
          mMode = MODE_CHANGE_MINUTE;
          break;
        case MODE_CHANGE_MINUTE:
          mMode = MODE_CHANGE_SECOND;
          break;
        case MODE_CHANGE_SECOND:
          mMode = MODE_CHANGE_YEAR;
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        // kTypeManual
        case MODE_CHANGE_TIME_ZONE_OFFSET:
          mMode = MODE_CHANGE_TIME_ZONE_DST;
          break;
        case MODE_CHANGE_TIME_ZONE_DST:
          mMode = MODE_CHANGE_TIME_ZONE_OFFSET;
          break;
      #else
        // kTypeBasic
        case MODE_CHANGE_TIME_ZONE_NAME:
          mMode = MODE_CHANGE_YEAR;
          break;
      #endif
      }
    }

    void modeButtonLongPress() {
      switch (mMode) {
        case MODE_VIEW_MED:
          mChangingClockInfo = mClockInfo;
          mMode = MODE_CHANGE_MED_HOUR;
          break;
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          saveMedInterval();
          mMode = MODE_VIEW_MED;
          break;

        case MODE_VIEW_DATE_TIME:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mMode = MODE_CHANGE_YEAR;
          break;
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          saveChangingClockInfo();
          mMode = MODE_VIEW_DATE_TIME;
          break;

        case MODE_VIEW_TIME_ZONE:
          mChangingClockInfo = mClockInfo;
        #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
          mMode = MODE_CHANGE_TIME_ZONE_OFFSET;
        #else
          mMode = MODE_CHANGE_TIME_ZONE_NAME;
        #endif
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          saveChangingClockInfo();
          mMode = MODE_VIEW_TIME_ZONE;
          break;
      }
    }

    void saveChangingClockInfo() {
      mClockInfo = mChangingClockInfo;
      mClock.setNow(mClockInfo.dateTime.toEpochSeconds());
      preserveInfo();
    }

    void saveMedInterval() {
      if (mClockInfo.medInterval != mChangingClockInfo.medInterval) {
        mClockInfo.medInterval = mChangingClockInfo.medInterval;
        mClockInfo.medInterval.second(0);
        preserveInfo();
      }
    }

    void saveTimeZone() {
      mClockInfo.timeZone = mChangingClockInfo.timeZone;
      mClockInfo.dateTime =
          mClockInfo.dateTime.convertToTimeZone(mClockInfo.timeZone);
      preserveInfo();
    }

    void changeButtonPress() {
      switch (mMode) {
        case MODE_CHANGE_MED_HOUR:
          mSuppressBlink = true;
          time_period_mutation::incrementHour(
              mChangingClockInfo.medInterval, 36);
          break;
        case MODE_CHANGE_MED_MINUTE:
          mSuppressBlink = true;
          time_period_mutation::incrementMinute(
              mChangingClockInfo.medInterval);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        {
          mSuppressBlink = true;
          TimeOffset offset = mChangingClockInfo.timeZone.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          mChangingClockInfo.timeZone.setStdOffset(offset);
          break;
        }
        case MODE_CHANGE_TIME_ZONE_DST:
        {
          mSuppressBlink = true;
          TimeOffset dstOffset = mChangingClockInfo.timeZone.getDstOffset();
          dstOffset = (dstOffset.isZero())
              ? TimeOffset::forMinutes(kDstOffsetMinutes)
              : TimeOffset();
          mChangingClockInfo.timeZone.setDstOffset(dstOffset);
          break;
        }

      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
          mSuppressBlink = true;
          mZoneIndex++;
          if (mZoneIndex >= kZoneRegistrySize) {
            mZoneIndex = 0;
          }
          mChangingClockInfo.timeZone =
              mZoneManager.createForZoneIndex(mZoneIndex);
          mChangingClockInfo.dateTime =
              mChangingClockInfo.dateTime.convertToTimeZone(
                  mChangingClockInfo.timeZone);
          break;
      #endif

        case MODE_CHANGE_YEAR:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementYear(mChangingClockInfo.dateTime);
          break;
        case MODE_CHANGE_MONTH:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMonth(mChangingClockInfo.dateTime);
          break;
        case MODE_CHANGE_DAY:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;
        case MODE_CHANGE_HOUR:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementHour(mChangingClockInfo.dateTime);
          break;
        case MODE_CHANGE_MINUTE:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMinute(
              mChangingClockInfo.dateTime);
          break;
        case MODE_CHANGE_SECOND:
          mSuppressBlink = true;
          mChangingClockInfo.dateTime.second(0);
          mSecondFieldCleared = true;
          break;

      }

      // Update the display right away to prevent jitters during RepeatPress.
      update();
    }

    void changeButtonRepeatPress() {
      changeButtonPress();
    }

    void changeButtonRelease() {
      switch (mMode) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          mSuppressBlink = false;
          break;
      }
    }

    void changeButtonLongPress() {
      switch (mMode) {
        case MODE_VIEW_MED:
          mClockInfo.medStartTime = mClockInfo.dateTime.toEpochSeconds();
          preserveInfo();
          break;
      }
    }

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void update() {
      if (mMode == MODE_UNKNOWN) return;
      if (mIsPreparingToSleep) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();
      mPresenter.display();
    }

  protected:
    void preserveInfo() {
      StoredInfo storedInfo;
      storedInfo.timeZoneData = mClockInfo.timeZone.toTimeZoneData();
      storedInfo.medStartTime = mClockInfo.medStartTime;
      storedInfo.medInterval = mClockInfo.medInterval;
      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, &storedInfo,
          sizeof(StoredInfo));
    }

    void updateDateTime() {
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          mClock.getNow(), mClockInfo.timeZone);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update mChangingClockInfo.dateTime with the current second.
      switch (mMode) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          if (!mSecondFieldCleared) {
            mChangingClockInfo.dateTime.second(mClockInfo.dateTime.second());
          }
          break;
      }
    }

    /** Update the internal rendering info. */
    void updateRenderingInfo() {
      switch (mMode) {
        case MODE_VIEW_DATE_TIME:
        case MODE_VIEW_TIME_ZONE:
        case MODE_VIEW_ABOUT:
          mPresenter.setRenderingInfo(mMode, mSuppressBlink, mBlinkShowState,
              mClockInfo);
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          mPresenter.setRenderingInfo(mMode, mSuppressBlink, mBlinkShowState,
              mChangingClockInfo);
          break;

        case MODE_VIEW_MED: {
          TimePeriod period(0);
          if (! mClockInfo.dateTime.isError()) {
            int32_t now = mClockInfo.dateTime.toEpochSeconds();
            int32_t remainingSeconds = mClockInfo.medStartTime
                + mClockInfo.medInterval.toSeconds() - now;
            period = TimePeriod(remainingSeconds);
          }
          mChangingClockInfo = mClockInfo;
          mChangingClockInfo.medInterval = period;
          mPresenter.setRenderingInfo(mMode, mSuppressBlink, mBlinkShowState,
              mChangingClockInfo);
          break;
        }

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          mPresenter.setRenderingInfo(mMode, mSuppressBlink, mBlinkShowState,
              mChangingClockInfo);
          break;

      }
    }

    /** Update the blinkShowState. */
    void updateBlinkState () {
      uint16_t now = millis();
      uint16_t duration = now - mBlinkCycleStartMillis;
      if (duration < 500) {
        mBlinkShowState = true;
      } else if (duration < 1000) {
        mBlinkShowState = false;
      } else {
        mBlinkCycleStartMillis = now;
      }
    }

    void restoreClockInfo(ClockInfo& clockInfo, const StoredInfo& storedInfo) {
    #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
      if (storedInfo.timeZoneData.type == TimeZoneData::kTypeManual) {
        clockInfo.timeZone = TimeZone::forTimeOffset(
            TimeOffset::forMinutes(storedInfo.timeZoneData.stdOffsetMinutes),
            TimeOffset::forMinutes(storedInfo.timeZoneData.dstOffsetMinutes));
      } else {
        clockInfo.timeZone = TimeZone::forTimeOffset(
            TimeOffset::forMinutes(kDefaultOffsetMinutes),
            TimeOffset());
      }
    #else
      if (storedInfo.timeZoneData.type == TimeZoneData::kTypeZoneId) {
        clockInfo.timeZone = mZoneManager.createForTimeZoneData(
            storedInfo.timeZoneData);
      } else {
        clockInfo.timeZone = mZoneManager.createForZoneId(
            BasicZone(&zonedb::kZoneAmerica_Los_Angeles).zoneId());
      }
    #endif

      clockInfo.medInterval = storedInfo.medInterval;
      clockInfo.medStartTime = storedInfo.medStartTime;
    }

    void setupClockInfo(acetime_t nowSeconds) {
      StoredInfo storedInfo;

    #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
      storedInfo.timeZoneData.type = TimeZone::kTypeManual;
      storedInfo.timeZoneData.stdOffsetMinutes = kDefaultOffsetMinutes;
      storedInfo.timeZoneData.dstOffsetMinutes = 0;
    #else
      storedInfo.timeZoneData.type = TimeZoneData::kTypeZoneId;
      storedInfo.timeZoneData.zoneId =
          BasicZone(&zonedb::kZoneAmerica_Los_Angeles).zoneId();
    #endif

      storedInfo.medInterval = TimePeriod(86400); // one day
      storedInfo.medStartTime = nowSeconds;

      preserveInfo();
      restoreClockInfo(mClockInfo, storedInfo);
    }


  protected:
    static const uint16_t kCacheSize = 2;
    static const basic::ZoneInfo* const kZoneRegistry[];
    static const uint16_t kZoneRegistrySize;

    SystemClock& mClock;
    CrcEeprom& mCrcEeprom;
    Presenter& mPresenter;
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager<kCacheSize> mZoneManager;;
  #endif

    uint8_t mMode; // current mode
    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // target clock (in change mode)

    uint16_t mZoneIndex;
    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
    bool mIsPreparingToSleep = false;
};

#endif
