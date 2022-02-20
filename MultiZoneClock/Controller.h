#ifndef MULTI_ZONE_CLOCK_CONTROLLER_H
#define MULTI_ZONE_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <mode_group/mode_group.h> // from AceUtils
#include "ClockInfo.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_common::incrementMod;
using ace_utils::mode_group::ModeGroup;
using ace_utils::mode_group::ModeNavigator;

/**
 * Class responsible for rendering the RenderingInfo to the indicated display.
 * Different subclasses output to different types of displays. In an MVC
 * architecture, this would be the Controller. The Model would be the various
 * member variables in this class. The View layer are the various Presenter
 * classes.
 */
class Controller {
  public:
    static const uint16_t kStoredInfoEepromAddress = 0;

    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    // Number of minutes to use for a DST offset.
    static const int16_t kDstOffsetMinutes = 60;

    /**
     * Constructor.
     * @param persistentStore stores objects into the EEPROM with CRC
     * @param clock source of the current time
     * @param presenter renders the date and time info to the screen
     * @param zoneManager optional zoneManager for TIME_ZONE_TYPE_BASIC or
     *        TIME_ZONE_TYPE_EXTENDED
     * @param displayZones array of TimeZoneData with NUM_TIME_ZONES elements
     * @param rootModeGroup poniter to the top level ModeGroup object
     */
    Controller(
        PersistentStore& persistentStore,
        SystemClock& clock,
        Presenter& presenter,
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        ManualZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        BasicZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        ExtendedZoneManager& zoneManager,
      #endif
        TimeZoneData const* displayZones,
        ModeGroup const* rootModeGroup
    ) :
        mPersistentStore(persistentStore),
        mClock(clock),
        mPresenter(presenter),
        mZoneManager(zoneManager),
        mDisplayZones(displayZones),
        mNavigator(rootModeGroup)
      {}

    /**
     * @param factoryReset set to true to perform a reset to factory defaults
     */
    void setup(bool factoryReset) {
      if (FORCE_INITIALIZE == 1) {
        factoryReset = true;
      }
      restoreClockInfo(factoryReset);

      // TODO: Move to Presenter?
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
      #endif

      updateDateTime();
    }

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void update() {
      if (mNavigator.modeId() == (uint8_t) Mode::kUnknown) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();
      mPresenter.display();
    }

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      mNavigator.changeMode();
      performEnteringModeAction();
    }

    void performEnteringModeAction() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeAction()"));
      }

      switch ((Mode) mNavigator.modeId()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
          mCurrentZone = &mChangingClockInfo.zones[0];
          break;
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
          mCurrentZone = &mChangingClockInfo.zones[1];
          break;
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
          mCurrentZone = &mChangingClockInfo.zones[2];
          break;
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
          mCurrentZone = &mChangingClockInfo.zones[3];
          break;
      #else
        case Mode::kChangeTimeZone0Name:
          mCurrentZone = &mChangingClockInfo.zones[0];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case Mode::kChangeTimeZone1Name:
          mCurrentZone = &mChangingClockInfo.zones[1];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case Mode::kChangeTimeZone2Name:
          mCurrentZone = &mChangingClockInfo.zones[2];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case Mode::kChangeTimeZone3Name:
          mCurrentZone = &mChangingClockInfo.zones[3];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
      #endif

        default:
          break;
      }
    }

    void performLeavingModeAction() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeAction()"));
      }
    }

    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      performLeavingModeAction();
      performLeavingModeGroupAction();
      mNavigator.changeGroup();
      performEnteringModeGroupAction();
      performEnteringModeAction();
    }

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
          // Throw away the changes and just change the mode group.
          //performLeavingModeAction();
          //performLeavingModeGroupAction();
          mNavigator.changeGroup();
          performEnteringModeGroupAction();
          performEnteringModeAction();
          break;

        default:
          break;
      }
    }

    /** Do action associated with entering a ModeGroup due to a LongPress. */
    void performEnteringModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeGroupAction()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          // If the system clock hasn't been initialized, set the initial
          // clock to epoch 0, which is 2000-01-01T00:00:00 UTC.
          if (mChangingClockInfo.dateTime.isError()) {
            mChangingClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
                0, mChangingClockInfo.dateTime.timeZone());
          }
          break;

        default:
          break;
      }
    }

    /** Do action associated with leaving a ModeGroup due to a LongPress. */
    void performLeavingModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeGroupAction()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          saveDateTime();
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
          saveClockInfo();
          break;

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
      #endif
        case Mode::kChangeInvertDisplay:
          preserveClockInfo(mClockInfo);
          break;

        default:
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch ((Mode) mNavigator.modeId()) {
        // Change button causes toggling of 12/24 modes if in Mode::kViewDateTime
        case Mode::kViewDateTime:
          mClockInfo.hourMode ^= 0x1;
          preserveClockInfo(mClockInfo);
          break;

        case Mode::kChangeYear:
          mSuppressBlink = true;
          #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
            zoned_date_time_mutation::incrementYear(
                mChangingClockInfo.dateTime);
          #else
            {
              auto& dateTime = mChangingClockInfo.dateTime;
              int16_t year = dateTime.year();
              year++;
              // Keep the year within the bounds of zonedb or zonedbx files.
              #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
              if (year >= zonedb::kZoneContext.untilYear) {
                year = zonedb::kZoneContext.startYear;
              }
              #else
              if (year >= zonedbx::kZoneContext.untilYear) {
                year = zonedbx::kZoneContext.startYear;
              }
              #endif
              dateTime.year(year);
            }
          #endif
          break;
        case Mode::kChangeMonth:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMonth(mChangingClockInfo.dateTime);
          break;
        case Mode::kChangeDay:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;
        case Mode::kChangeHour:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementHour(mChangingClockInfo.dateTime);
          break;
        case Mode::kChangeMinute:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMinute(
              mChangingClockInfo.dateTime);
          break;
        case Mode::kChangeSecond:
          mSuppressBlink = true;
          mChangingClockInfo.dateTime.second(0);
          mSecondFieldCleared = true;
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone3Offset:
        {
          mSuppressBlink = true;
          if (ENABLE_SERIAL_DEBUG >= 1) {
            if (! mCurrentZone) {
              SERIAL_PORT_MONITOR.println("***CurrentZone is NULL***");
            }
          }
          TimeZone tz = mZoneManager.createForTimeZoneData(*mCurrentZone);
          TimeOffset offset = tz.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          tz.setStdOffset(offset);
          *mCurrentZone = tz.toTimeZoneData();
          break;
        }

        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Dst:
        {
          mSuppressBlink = true;
          TimeZone tz = mZoneManager.createForTimeZoneData(*mCurrentZone);
          TimeOffset dstOffset = tz.getDstOffset();
          dstOffset = (dstOffset.isZero())
              ? TimeOffset::forMinutes(kDstOffsetMinutes)
              : TimeOffset();
          tz.setDstOffset(dstOffset);
          *mCurrentZone = tz.toTimeZoneData();
          break;
        }

      #else
        // Cycle through the zones in the registry
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
        {
          mSuppressBlink = true;
          mZoneRegistryIndex++;
          if (mZoneRegistryIndex >= mZoneManager.zoneRegistrySize()) {
            mZoneRegistryIndex = 0;
          }
          TimeZone tz = mZoneManager.createForZoneIndex(mZoneRegistryIndex);
          *mCurrentZone = tz.toTimeZoneData();

          // TimeZone 0 is the default displayed timezone.
          if (mNavigator.modeId() == (uint8_t) Mode::kChangeTimeZone0Name) {
            mChangingClockInfo.dateTime =
                mChangingClockInfo.dateTime.convertToTimeZone(tz);
          }
          break;
        }
      #endif

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.backlightLevel, (uint8_t) 10);
          break;
        }
        case Mode::kChangeSettingsContrast: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.contrast, (uint8_t) 128);
          break;
        }
        case Mode::kChangeSettingsBias: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.bias, (uint8_t) 8);
          break;
        }
      #else
        case Mode::kChangeSettingsContrast: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.contrastLevel, (uint8_t) 10);
          break;
        }
        case Mode::kChangeInvertDisplay: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.invertDisplay, (uint8_t) 3);
          break;
        }
      #endif

        default:
          break;
      }

      // Update the display right away to prevent jitters in the display when
      // the button is triggering RepeatPressed events.
      update();
    }

    void handleChangeButtonRepeatPress() {
      // Ignore 12/24 changes from RepeatPressed events. 1) It doesn't make
      // sense to repeatedly change the 12/24 mode when the button is held
      // down. 2) Each change of 12/24 mode causes a write to the EEPPROM,
      // which causes wear and tear.
      if (mNavigator.modeId() != (uint8_t) Mode::kViewDateTime) {
        handleChangeButtonPress();
      }
    }

    void handleChangeButtonRelease() {
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
          mSuppressBlink = false;
          break;

        default:
          break;
      }
    }

  private:
    /**
     * Update the ClockInfo.dateTime using the SystemClock and the time zone
     * defined by ClockInfo.zones[0].
     */
    void updateDateTime() {
      // Update the current dateTime from the SystemClock epochSeconds, using
      // the timezone at zones[0].
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.zones[0]);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(mClock.getNow(), tz);

      //acetime_t lastSync = mClock.getLastSyncTime();
      int32_t secondsSinceSyncAttempt = mClock.getSecondsSinceSyncAttempt();
      int32_t secondsToSyncAttempt = mClock.getSecondsToSyncAttempt();
      mClockInfo.prevSync = TimePeriod(secondsSinceSyncAttempt);
      mClockInfo.nextSync = TimePeriod(secondsToSyncAttempt);
      mClockInfo.clockSkew = TimePeriod(mClock.getClockSkew());
      mClockInfo.syncStatusCode = mClock.getSyncStatusCode();

      // If the dateTime is currently being changed, don't update the
      // 'mChangingClockInfo.dateTime' with the SystemClock since that would
      // clobber the fields entered by the user with the SystemClock. The
      // exception is the 'second' field which will continue to be updated...
      // unless the second field has been explicitly cleared. Then we keep it
      // pegged at '00' second.
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

        // We can ignore the Mode::kChangeXxx for display time zones because
        // those modes do not display the time, so we don't need to update
        // mChangingClockInfo.dateTime.

        default:
          break;
      }
    }

    /**
     * The blinking clock is different than the SystemClock, so the blinking
     * will becomes slightly skewed from the changes to the 'second' field. If
     * the 'second' field is not displayed, then this is not a big deal. But if
     * the second field shown, people may notice that the two are not
     * synchronized. It would be nice to trigger the blinking from the
     * SystemClock. The tricky thing is that the SystemClock has only
     * one-second resolution, but blinking requires 0.5s resolution.
     */
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

    void updateRenderingInfo() {
      ClockInfo* clockInfo;

      switch ((Mode) mNavigator.modeId()) {
        // If we are changing the current date, time or time zones, render the
        // mChangingClockInfo instead, because changes are made to the copy,
        // instead of the current mClockInfo, and copied over to mClockInfo
        // upon LongPress.
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
          clockInfo = &mChangingClockInfo;
          break;

        // For all other modes render the normal mClockInfo instead. This
        // includes the "change settings" modes, because those are applied
        // directly to the current mClockInfo, not the mChangingClockInfo.
        default:
          clockInfo = &mClockInfo;
      }

      bool blinkShowState = mSuppressBlink || mBlinkShowState;
      mPresenter.setRenderingInfo(
          (Mode) mNavigator.modeId(), blinkShowState, *clockInfo
      );
    }

    /** Save the changed dateTime to the SystemClock. */
    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    /** Transfer info from ChangingClockInfo to ClockInfo. */
    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      mClockInfo = mChangingClockInfo;
      preserveClockInfo(mClockInfo);
    }

    /** Save the clock info into EEPROM. */
    void preserveClockInfo(const ClockInfo& clockInfo) {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("preserveClockInfo()"));
      }
      StoredInfo storedInfo;
      storedInfoFromClockInfo(storedInfo, clockInfo);
      mPersistentStore.writeStoredInfo(storedInfo);
    }

    /** Convert StoredInfo to ClockInfo. */
    static void clockInfoFromStoredInfo(
        ClockInfo& clockInfo, const StoredInfo& storedInfo) {
      clockInfo.hourMode = storedInfo.hourMode;
      memcpy(clockInfo.zones, storedInfo.zones,
          sizeof(TimeZoneData) * NUM_TIME_ZONES);
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        clockInfo.backlightLevel = storedInfo.backlightLevel;
        clockInfo.contrast = storedInfo.contrast;
        clockInfo.bias = storedInfo.bias;
      #else
        clockInfo.contrastLevel = storedInfo.contrastLevel;
        clockInfo.invertDisplay = storedInfo.invertDisplay;
      #endif
    }

    /** Convert ClockInfo to StoredInfo. */
    static void storedInfoFromClockInfo(
        StoredInfo& storedInfo, const ClockInfo& clockInfo) {
      storedInfo.hourMode = clockInfo.hourMode;
      memcpy(storedInfo.zones, clockInfo.zones,
          sizeof(TimeZoneData) * NUM_TIME_ZONES);
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        storedInfo.backlightLevel = clockInfo.backlightLevel;
        storedInfo.contrast = clockInfo.contrast;
        storedInfo.bias = clockInfo.bias;
      #else
        storedInfo.contrastLevel = clockInfo.contrastLevel;
        storedInfo.invertDisplay = clockInfo.invertDisplay;
      #endif
    }

    /** Attempt to restore from EEPROM, otherwise use factory defaults. */
    void restoreClockInfo(bool factoryReset) {
      StoredInfo storedInfo;
      bool isValid;
      if (factoryReset) {
        if (ENABLE_SERIAL_DEBUG >= 1) {
          SERIAL_PORT_MONITOR.println(F("restoreClockInfo(): FACTORY RESET"));
        }
        isValid = false;
      } else {
        isValid = mPersistentStore.readStoredInfo(storedInfo);
        if (ENABLE_SERIAL_DEBUG >= 1) {
          if (! isValid) {
            SERIAL_PORT_MONITOR.println(F(
                "restoreClockInfo(): EEPROM NOT VALID; "
                "Using factory defaults"));
          }
        }
      }

      if (isValid) {
        clockInfoFromStoredInfo(mClockInfo, storedInfo);
      } else {
        setupClockInfo();
        preserveClockInfo(mClockInfo);
      }
    }

    /** Set up the initial ClockInfo states using mDisplayZones[0]. */
    void setupClockInfo() {
      mClockInfo.hourMode = ClockInfo::kTwentyFour;
      memcpy(mClockInfo.zones, mDisplayZones,
          sizeof(TimeZoneData) * NUM_TIME_ZONES);

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mClockInfo.backlightLevel = 0;
      mClockInfo.contrast = LCD_INITIAL_CONTRAST;
      mClockInfo.bias = LCD_INITIAL_BIAS;
    #else
      mClockInfo.contrastLevel = OLED_INITIAL_CONTRAST;
      mClockInfo.invertDisplay = ClockInfo::kInvertDisplayOff;
    #endif
    }

  private:
    PersistentStore& mPersistentStore;
    SystemClock& mClock;
    Presenter& mPresenter;
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif
    TimeZoneData const* const mDisplayZones;
    ModeNavigator mNavigator;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    // Parameters for changing each of the display time zone. Upon entering the
    // mode for changing each time zone, this will point to the
    // ChangingClockInfo.displayZone[n] entry, and the mZoneRegistryIndex will
    // point to the corresponding ZoneManager registry index. Clicking the
    // 'Change' button will increment the mZoneRegistryIndex and update the
    // TimeZone pointed to by the mCurrentZone pointer.
    TimeZoneData* mCurrentZone = nullptr;
    uint16_t mZoneRegistryIndex = 0;

    bool mSecondFieldCleared = false;

    // Handle blinking.
    bool mSuppressBlink = false; // true if blinking should be suppressed
    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
