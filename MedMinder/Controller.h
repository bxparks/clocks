#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <SSD1306AsciiWire.h>
#include "ClockInfo.h"
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"

using ace_common::incrementMod;
using namespace ace_time;
using namespace ace_time::clock;

/**
 * Class responsible for handling user button presses, and determining
 * how various member variables are updated. The rendering of the various
 * variables are handled by the Presenter. If analyzed using an MVC
 * architecture, this is the Controller, the Presenter is the View, and the
 * various member variables of this class are the Model objects.
 */
class Controller {
  public:
    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    // Number of minutes to use for a DST offset.
    static const int16_t kDstOffsetMinutes = 60;

    /** Constructor. */
    Controller(
        SystemClock& clock
        , PersistentStore& persistentStore
        , Presenter& presenter
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        , ManualZoneManager& zoneManager
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        , BasicZoneManager& zoneManager
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        , ExtendedZoneManager& zoneManager
      #endif
        , TimeZoneData initialTimeZoneData
    ) :
        mClock(clock)
        , mPersistentStore(persistentStore)
        , mPresenter(presenter)
        , mZoneManager(zoneManager)
        , mInitialTimeZoneData(initialTimeZoneData)
    {
      mClockInfo.mode = Mode::kViewDateTime;
    }

    void setup() {
      // Retrieve current time from Clock.
      acetime_t nowSeconds = mClock.getNow();

      // Restore state from EEPROM if valid.
      StoredInfo storedInfo;
      bool isValid = mPersistentStore.readStoredInfo(storedInfo);
      if (isValid) {
        if (ENABLE_SERIAL_DEBUG >= 1) {
          SERIAL_PORT_MONITOR.println(F("setup(): valid StoredInfo"));
        }
        restoreClockInfo(mClockInfo, storedInfo);
      } else {
        if (ENABLE_SERIAL_DEBUG >= 1) {
          SERIAL_PORT_MONITOR.println(
              F("setup(): invalid StoredInfo; initializing"));
        }
        setupClockInfo(nowSeconds);
      }
    }

    void syncClock() {
      mClock.forceSync();
      acetime_t nowSeconds = mClock.getNow();
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(nowSeconds, tz);
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
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }

      switch (mClockInfo.mode) {
        // View modes
        case Mode::kViewMed:
          mClockInfo.mode = Mode::kViewDateTime;
          break;
        case Mode::kViewDateTime:
          mClockInfo.mode = Mode::kViewTimeZone;
          break;
        case Mode::kViewTimeZone:
          mClockInfo.mode = Mode::kViewSettings;
          break;
        case Mode::kViewSettings:
          mClockInfo.mode = Mode::kViewAbout;
          break;
        case Mode::kViewAbout:
          mClockInfo.mode = Mode::kViewMed;
          break;

        // Change Med Hour/Minute
        case Mode::kChangeMedHour:
          mClockInfo.mode = Mode::kChangeMedMinute;
          break;
        case Mode::kChangeMedMinute:
          mClockInfo.mode = Mode::kChangeMedHour;
          break;

        // Change Date/Time
        case Mode::kChangeHour:
          mClockInfo.mode = Mode::kChangeMinute;
          break;
        case Mode::kChangeMinute:
          mClockInfo.mode = Mode::kChangeSecond;
          break;
        case Mode::kChangeSecond:
          mClockInfo.mode = Mode::kChangeYear;
          break;
        case Mode::kChangeYear:
          mClockInfo.mode = Mode::kChangeMonth;
          break;
        case Mode::kChangeMonth:
          mClockInfo.mode = Mode::kChangeDay;
          break;
        case Mode::kChangeDay:
          mClockInfo.mode = Mode::kChangeHour;
          break;

        // Change TimeZone
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
          mClockInfo.mode = Mode::kChangeTimeZoneDst;
          break;
        case Mode::kChangeTimeZoneDst:
          mClockInfo.mode = Mode::kChangeTimeZoneOffset;
          break;
      #else
        case Mode::kChangeTimeZoneName:
          break;
      #endif

        default:
          break;
      }

      mChangingClockInfo.mode = mClockInfo.mode;
    }

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      // Throw away the changes and just go back to the various View modes.
      switch (mClockInfo.mode) {
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          mClockInfo.mode = Mode::kViewDateTime;
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
          mClockInfo.mode = Mode::kViewTimeZone;
          break;

        case Mode::kChangeSettingsContrast:
          mClockInfo.mode = Mode::kViewSettings;
          break;

        default:
          break;
      }
    }

    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kViewMed:
          mChangingClockInfo = mClockInfo;
          mClockInfo.mode = Mode::kChangeMedHour;
          break;

        // Long Press in Change Med info mode, so save the changed info.
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
          saveMedInterval();
          mClockInfo.mode = Mode::kViewMed;
          break;

        // Long Press in View modes changes to Change modes.
        case Mode::kViewDateTime:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          initChangingClock();
          mClockInfo.mode = Mode::kChangeYear;
          break;

        // Long Press in Change date/time mode, so save the changed info.
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          saveDateTime();
          mClockInfo.mode = Mode::kViewDateTime;
          break;

        // Long Press in ViewTimeZone mode.
        case Mode::kViewTimeZone:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZoneData.zoneId);
        #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
          mClockInfo.mode = Mode::kChangeTimeZoneOffset;
        #else
          mClockInfo.mode = Mode::kChangeTimeZoneName;
        #endif
          break;

        // Long Press in Change time zone mode.
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
          saveChangingClockInfo();
          mClockInfo.mode = Mode::kViewTimeZone;
          break;

        // Long Press in ViewSettings mode.
        case Mode::kViewSettings:
          mClockInfo.mode = Mode::kChangeSettingsContrast;
          break;

        // Long Press in Change Settings mode.
        case Mode::kChangeSettingsContrast:
          preserveClockInfo();
          mClockInfo.mode = Mode::kViewSettings;
          break;

        default:
          break;
      }

      mChangingClockInfo.mode = mClockInfo.mode;
    }

    // If the system clock hasn't been initialized, set the initial
    // clock to epoch 0, which is 2000-01-01T00:00:00 UTC.
    void initChangingClock() {
      if (mChangingClockInfo.dateTime.isError()) {
        mChangingClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
            0, mChangingClockInfo.dateTime.timeZone());
      }
    }

    void saveMedInterval() {
      if (mClockInfo.medInterval != mChangingClockInfo.medInterval) {
        mClockInfo.medInterval = mChangingClockInfo.medInterval;
        mClockInfo.medInterval.second(0);
        preserveClockInfo();
      }
    }

    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    void saveChangingClockInfo() {
      mClockInfo = mChangingClockInfo;
      preserveClockInfo();
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      mClockInfo.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch (mClockInfo.mode) {
        case Mode::kChangeMedHour:
          time_period_mutation::incrementHour(
              mChangingClockInfo.medInterval, MAX_MED_INTERVAL_HOURS);
          break;

        case Mode::kChangeMedMinute:
          time_period_mutation::incrementMinute(
              mChangingClockInfo.medInterval);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset: {
          TimeZone tz = mZoneManager.createForTimeZoneData(
              mChangingClockInfo.timeZoneData);
          TimeOffset offset = tz.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          tz.setStdOffset(offset);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          break;
        }

        case Mode::kChangeTimeZoneDst: {
          TimeZone tz = mZoneManager.createForTimeZoneData(
              mChangingClockInfo.timeZoneData);
          TimeOffset dstOffset = tz.getDstOffset();
          dstOffset = (dstOffset.isZero())
              ? TimeOffset::forMinutes(kDstOffsetMinutes)
              : TimeOffset();
          tz.setDstOffset(dstOffset);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          break;
        }

      #else
        case Mode::kChangeTimeZoneName: {
          incrementMod(mZoneRegistryIndex, mZoneManager.zoneRegistrySize());
          TimeZone tz = mZoneManager.createForZoneIndex(mZoneRegistryIndex);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          mChangingClockInfo.dateTime =
              mChangingClockInfo.dateTime.convertToTimeZone(tz);
          break;
        }
      #endif

        case Mode::kChangeYear:
          zoned_date_time_mutation::incrementYear(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMonth:
          zoned_date_time_mutation::incrementMonth(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeDay:
          zoned_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeHour:
          zoned_date_time_mutation::incrementHour(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMinute:
          zoned_date_time_mutation::incrementMinute(
              mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeSecond:
          mChangingClockInfo.dateTime.second(0);
          mSecondFieldCleared = true;
          break;

        case Mode::kChangeSettingsContrast: {
          incrementMod(mClockInfo.contrastLevel, (uint8_t) 10);
          break;
        }

        default:
          break;
      }

      // Update the display right away to prevent jitters during RepeatPress.
      update();
    }

    void handleChangeButtonRepeatPress() {
      handleChangeButtonPress();
    }

    void handleChangeButtonRelease() {
      switch (mClockInfo.mode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
        case Mode::kChangeSettingsContrast:
          mClockInfo.suppressBlink = false;
          mChangingClockInfo.suppressBlink = false;
          break;

        default:
          break;
      }
    }

    void handleChangeButtonLongPress() {
      switch (mClockInfo.mode) {
        case Mode::kViewMed:
          mClockInfo.medStartTime = mClockInfo.dateTime.toEpochSeconds();
          preserveClockInfo();
          break;

        default:
          break;
      }
    }

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void update() {
      if (mClockInfo.mode == Mode::kUnknown) return;
      if (mIsPreparingToSleep) return;
      updateDateTime();
      updatePresenter();
      mPresenter.updateDisplay();
    }

    void updateBlinkState () {
      mClockInfo.blinkShowState = !mClockInfo.blinkShowState;
      mChangingClockInfo.blinkShowState = !mChangingClockInfo.blinkShowState;
      updatePresenter();
    }

  private:
    /** Save the clock info into EEPROM. */
    void preserveClockInfo() {
      StoredInfo storedInfo;
      storedInfo.timeZoneData = mClockInfo.timeZoneData;
      storedInfo.medStartTime = mClockInfo.medStartTime;
      storedInfo.medInterval = mClockInfo.medInterval;
      storedInfo.contrastLevel = mClockInfo.contrastLevel;
      mPersistentStore.writeStoredInfo(storedInfo);
    }

    void updateDateTime() {
      acetime_t nowSeconds = mClock.getNow();
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(nowSeconds, tz);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update mChangingClockInfo.dateTime with the current second.
      switch (mClockInfo.mode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          if (!mSecondFieldCleared) {
            mChangingClockInfo.dateTime.second(mClockInfo.dateTime.second());
          }
          break;

        default:
          break;
      }
    }

    /** Update the rendering info for the Presenter. */
    void updatePresenter() {
      switch (mClockInfo.mode) {
        case Mode::kViewDateTime:
        case Mode::kViewTimeZone:
        case Mode::kViewSettings:
        case Mode::kViewAbout:
        case Mode::kChangeSettingsContrast:
          mPresenter.setClockInfo(mClockInfo);
          break;

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
          mPresenter.setClockInfo(mChangingClockInfo);
          break;

        case Mode::kViewMed: {
          // Overload ChangingClockInfo.medInterval to hold the 'time
          // remaining' TimePeriod information.
          mChangingClockInfo = mClockInfo;
          mChangingClockInfo.medInterval = getRemainingTimePeriod();
          mPresenter.setClockInfo(mChangingClockInfo);
          break;
        }

        default:
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

    void restoreClockInfo(ClockInfo& clockInfo, const StoredInfo& storedInfo) {
      clockInfo.timeZoneData = storedInfo.timeZoneData;
      clockInfo.medInterval = storedInfo.medInterval;
      clockInfo.medStartTime = storedInfo.medStartTime;
      clockInfo.contrastLevel = storedInfo.contrastLevel;
    }

    void setupClockInfo(acetime_t nowSeconds) {
      StoredInfo storedInfo;
      storedInfo.timeZoneData = mInitialTimeZoneData;
      storedInfo.medInterval = TimePeriod(86400); // one day
      storedInfo.medStartTime = nowSeconds;
      mClockInfo.contrastLevel = OLED_INITIAL_CONTRAST;

      preserveClockInfo();
      restoreClockInfo(mClockInfo, storedInfo);
    }

  protected:
    SystemClock& mClock;
    PersistentStore& mPersistentStore;
    Presenter& mPresenter;

  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif

    TimeZoneData mInitialTimeZoneData;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // target clock (in change mode)

    uint16_t mZoneRegistryIndex;
    bool mSecondFieldCleared;
    bool mIsPreparingToSleep = false;
};

#endif
