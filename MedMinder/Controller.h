#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <mode_group/mode_group.h> // from AceUtils
#include <SSD1306AsciiWire.h>
#include "ClockInfo.h"
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"

using ace_common::incrementMod;
using namespace ace_time;
using namespace ace_time::clock;
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
    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    // Number of minutes to use for a DST offset.
    static const int16_t kDstOffsetMinutes = 60;

    /** Constructor. */
    Controller(
        SystemClock& clock,
        PersistentStore& persistentStore,
        ModeGroup const* rootModeGroup,
        Presenter& presenter
    ) :
        mClock(clock),
        mPersistentStore(persistentStore),
        mPresenter(presenter),
        mNavigator(rootModeGroup)
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        , mZoneManager(kZoneRegistrySize, kZoneRegistry, mZoneProcessorCache)
      #endif
    {}

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

      // Set the current date time using the mClockInfo.timeZone.
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          nowSeconds, mClockInfo.timeZone);
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
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      // performModeButtonPressAction(); - does nothing in this app
      mNavigator.changeMode();
      performNewModeRecordAction();
    }

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
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
        case Mode::kChangeSettingsContrast:
          // Throw away the changes and just change the mode group.
          //performModeLongPressAction();
          mNavigator.changeGroup();
          performNewModeGroupAction();
          performNewModeRecordAction();
          break;

        default:
          break;
      }
    }

    /** Perform the action of the current ModeRecord. */
    void performNewModeRecordAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performNewModeRecordAction()"));
      }

      #if TIME_ZONE_TYPE != TIME_ZONE_TYPE_MANUAL
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeTimeZoneName:
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZone.getZoneId());
          break;

        default:
          break;
      }
      #endif
    }

    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      performModeLongPressAction();
      mNavigator.changeGroup();
      performNewModeGroupAction();
      performNewModeRecordAction();
    }

    /** Do action associated with entering a ModeGroup due to a LongPress. */
    void performNewModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performNewModeGroupAction()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
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
        case Mode::kChangeSettingsContrast:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          break;

        default:
          break;
      }
    }

    /** Do action associated with leaving a ModeGroup due to a LongPress. */
    void performModeLongPressAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performModeLongPressAction()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
          saveMedInterval();
          break;

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          saveDateTime();
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
          saveChangingClockInfo();
          break;

        case Mode::kChangeSettingsContrast:
          saveClockInfo();
          break;

        default:
          break;
      }
    }

    void saveMedInterval() {
      if (mClockInfo.medInterval != mChangingClockInfo.medInterval) {
        mClockInfo.medInterval = mChangingClockInfo.medInterval;
        mClockInfo.medInterval.second(0);
        saveClockInfo();
      }
    }

    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    void saveChangingClockInfo() {
      mClockInfo = mChangingClockInfo;
      saveClockInfo();
    }

    void saveTimeZone() {
      mClockInfo.timeZone = mChangingClockInfo.timeZone;
      mClockInfo.dateTime =
          mClockInfo.dateTime.convertToTimeZone(mClockInfo.timeZone);
      saveClockInfo();
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      mClockInfo.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeMedHour:
          time_period_mutation::incrementHour(
              mChangingClockInfo.medInterval, MAX_MED_INTERVAL_HOURS);
          break;

        case Mode::kChangeMedMinute:
          time_period_mutation::incrementMinute(
              mChangingClockInfo.medInterval);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        {
          TimeOffset offset = mChangingClockInfo.timeZone.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          mChangingClockInfo.timeZone.setStdOffset(offset);
          break;
        }

        case Mode::kChangeTimeZoneDst:
        {
          TimeOffset dstOffset = mChangingClockInfo.timeZone.getDstOffset();
          dstOffset = (dstOffset.isZero())
              ? TimeOffset::forMinutes(kDstOffsetMinutes)
              : TimeOffset();
          mChangingClockInfo.timeZone.setDstOffset(dstOffset);
          break;
        }

      #else
        case Mode::kChangeTimeZoneName:
          incrementMod(mZoneRegistryIndex, kZoneRegistrySize);
          mChangingClockInfo.timeZone =
              mZoneManager.createForZoneIndex(mZoneRegistryIndex);
          mChangingClockInfo.dateTime =
              mChangingClockInfo.dateTime.convertToTimeZone(
                  mChangingClockInfo.timeZone);
          break;
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
      switch ((Mode) mNavigator.modeId()) {
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
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kViewMed:
          mClockInfo.medStartTime = mClockInfo.dateTime.toEpochSeconds();
          saveClockInfo();
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
      if (mNavigator.modeId() == (uint8_t) Mode::kUnknown) return;
      if (mIsPreparingToSleep) return;
      updateDateTime();
      updatePresenter();
      mPresenter.display();
    }

    void updateBlinkState () {
      mClockInfo.blinkShowState = !mClockInfo.blinkShowState;
      mChangingClockInfo.blinkShowState = !mChangingClockInfo.blinkShowState;
      updatePresenter();
    }

  protected:
    void saveClockInfo() {
      StoredInfo storedInfo;
      storedInfo.timeZoneData = mClockInfo.timeZone.toTimeZoneData();
      storedInfo.medStartTime = mClockInfo.medStartTime;
      storedInfo.medInterval = mClockInfo.medInterval;
      storedInfo.contrastLevel = mClockInfo.contrastLevel;
      mPersistentStore.writeStoredInfo(storedInfo);
    }

    void updateDateTime() {
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          mClock.getNow(), mClockInfo.timeZone);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update mChangingClockInfo.dateTime with the current second.
      switch ((Mode) mNavigator.modeId()) {
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
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kViewDateTime:
        case Mode::kViewTimeZone:
        case Mode::kViewSettings:
        case Mode::kViewAbout:
        case Mode::kChangeSettingsContrast:
          mClockInfo.mode = (Mode) mNavigator.modeId();
          mPresenter.setRenderingInfo(mClockInfo);
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
          mChangingClockInfo.mode = (Mode) mNavigator.modeId();
          mPresenter.setRenderingInfo(mChangingClockInfo);
          break;

        case Mode::kViewMed: {
          // Overload ChangingClockInfo.medInterval to hold the 'time
          // remaining' TimePeriod information.
          mChangingClockInfo = mClockInfo;
          mChangingClockInfo.medInterval = getRemainingTimePeriod();

          mChangingClockInfo.mode = (Mode) mNavigator.modeId();
          mPresenter.setRenderingInfo(mChangingClockInfo);
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
      clockInfo.contrastLevel = storedInfo.contrastLevel;
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
      mClockInfo.contrastLevel = OLED_INITIAL_CONTRAST;

      saveClockInfo();
      restoreClockInfo(mClockInfo, storedInfo);
    }


  protected:
    static const uint16_t kCacheSize = 2;
    static const basic::ZoneInfo* const kZoneRegistry[];
    static const uint16_t kZoneRegistrySize;

    SystemClock& mClock;
    PersistentStore& mPersistentStore;
    Presenter& mPresenter;
    ModeNavigator mNavigator;
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneProcessorCache<kCacheSize> mZoneProcessorCache;
    BasicZoneManager mZoneManager;
  #endif

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // target clock (in change mode)

    uint16_t mZoneRegistryIndex;
    bool mSecondFieldCleared;
    bool mIsPreparingToSleep = false;
};

#endif
