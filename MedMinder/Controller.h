#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include <AceUtilsModeGroup.h>
#include <SSD1306AsciiWire.h>
#include "config.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"
#include "ClockInfo.h"

using ace_common::incrementMod;
using namespace ace_time;
using namespace ace_time::clock;
using ace_utils::crc_eeprom::CrcEeprom;
using ace_utils::mode_group::ModeGroup;
using ace_utils::mode_group::ModeNavigator;

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
    Controller(
        SystemClock& clock,
        CrcEeprom& crcEeprom,
        ModeGroup const* rootModeGroup,
        Presenter& presenter
    ) :
        mClock(clock),
        mCrcEeprom(crcEeprom),
        mPresenter(presenter),
        mNavigator(rootModeGroup)
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        , mZoneManager(kZoneRegistrySize, kZoneRegistry)
      #endif
    {}

    void setup() {
      // Retrieve current time from Clock.
      acetime_t nowSeconds = mClock.getNow();

      // Restore state from EEPROM if valid.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(
          kStoredInfoEepromAddress, storedInfo);
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

      mNavigator.setup();
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

    void handleModeButtonPress()  {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      mNavigator.changeMode();
      performEnteringModeAction();
    }

    void performEnteringModeAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeAction()"));
      }

      #if TIME_ZONE_TYPE != TIME_ZONE_TYPE_MANUAL
      switch (mNavigator.mode()) {
        case MODE_CHANGE_TIME_ZONE_NAME:
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZone.getZoneId());
          break;
      }
      #endif
    }

    void performLeavingModeAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeAction()"));
      }
    }

    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      performLeavingModeAction();
      performLeavingModeGroupAction();
      mNavigator.changeGroup();
      performEnteringModeGroupAction();
      performEnteringModeAction();
    }

    void performEnteringModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeGroupAction()"));
      }

      switch (mNavigator.mode()) {
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
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
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          break;
      }
    }

    void performLeavingModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeGroupAction()"));
      }

      switch (mNavigator.mode()) {
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          saveMedInterval();
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          saveDateTime();
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          saveClockInfo();
          break;
      }
    }

    void saveMedInterval() {
      if (mClockInfo.medInterval != mChangingClockInfo.medInterval) {
        mClockInfo.medInterval = mChangingClockInfo.medInterval;
        mClockInfo.medInterval.second(0);
        preserveInfo();
      }
    }

    void saveDateTime() {
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    void saveClockInfo() {
      mClockInfo = mChangingClockInfo;
      preserveInfo();
    }

    void saveTimeZone() {
      mClockInfo.timeZone = mChangingClockInfo.timeZone;
      mClockInfo.dateTime =
          mClockInfo.dateTime.convertToTimeZone(mClockInfo.timeZone);
      preserveInfo();
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch (mNavigator.mode()) {
        case MODE_CHANGE_MED_HOUR:
          mSuppressBlink = true;
          time_period_mutation::incrementHour(
              mChangingClockInfo.medInterval, MAX_MED_INTERVAL_HOURS);
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
          incrementMod(mZoneRegistryIndex, kZoneRegistrySize);
          mChangingClockInfo.timeZone =
              mZoneManager.createForZoneIndex(mZoneRegistryIndex);
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

    void handleChangeButtonRepeatPress() {
      handleChangeButtonPress();
    }

    void handleChangeButtonRelease() {
      switch (mNavigator.mode()) {
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

    void handleChangeButtonLongPress() {
      switch (mNavigator.mode()) {
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
      if (mNavigator.mode() == MODE_UNKNOWN) return;
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
      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, storedInfo);
    }

    void updateDateTime() {
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          mClock.getNow(), mClockInfo.timeZone);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update mChangingClockInfo.dateTime with the current second.
      switch (mNavigator.mode()) {
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
      switch (mNavigator.mode()) {
        case MODE_VIEW_DATE_TIME:
        case MODE_VIEW_TIME_ZONE:
        case MODE_VIEW_ABOUT:
          mPresenter.setRenderingInfo(
              mNavigator.mode(), mSuppressBlink || mBlinkShowState,
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
          mPresenter.setRenderingInfo(
              mNavigator.mode(), mSuppressBlink || mBlinkShowState,
              mChangingClockInfo);
          break;

        case MODE_VIEW_MED: {
          // Overload ChangingClockInfo.medInterval to hold the 'time
          // remaining' TimePeriod information.
          mChangingClockInfo = mClockInfo;
          mChangingClockInfo.medInterval = getRemainingTimePeriod();

          mPresenter.setRenderingInfo(
              mNavigator.mode(), mSuppressBlink || mBlinkShowState,
              mChangingClockInfo);
          break;
        }

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          mPresenter.setRenderingInfo(
              mNavigator.mode(), mSuppressBlink || mBlinkShowState,
              mChangingClockInfo);
          break;
      }
    }

    /**
     * Calculate time remaining. Return TimePeriod::forError() if
     * unable to calculate.
     */
    TimePeriod getRemainingTimePeriod() {
      if (mClockInfo.dateTime.isError()) {
        return TimePeriod::forError();
      }

      int32_t now = mClockInfo.dateTime.toEpochSeconds();
      int32_t remainingSeconds = mClockInfo.medStartTime
          + mClockInfo.medInterval.toSeconds() - now;
      if (remainingSeconds > MAX_MED_INTERVAL_HOURS * (int32_t)3600
          || remainingSeconds < -MAX_MED_INTERVAL_HOURS * (int32_t)3600) {
        return TimePeriod::forError();
      }

      return TimePeriod(remainingSeconds);
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
        clockInfo.timeZone = TimeZone::forMinutes(
            storedInfo.timeZoneData.stdOffsetMinutes,
            storedInfo.timeZoneData.dstOffsetMinutes);
      } else {
        clockInfo.timeZone = TimeZone::forMinutes(kDefaultOffsetMinutes);
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
      storedInfo.timeZoneData.zoneId = zonedb::kZoneIdAmerica_Los_Angeles;
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
    ModeNavigator mNavigator;
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager<kCacheSize> mZoneManager;
  #endif

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // target clock (in change mode)

    uint16_t mZoneRegistryIndex;
    bool mSecondFieldCleared;

    // Handle blinking
    bool mSuppressBlink; // true if blinking should be suppressed
    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start

    bool mIsPreparingToSleep = false;
};

#endif
