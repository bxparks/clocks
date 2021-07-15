#ifndef ONE_ZONE_CLOCK_CONTROLLER_H
#define ONE_ZONE_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod()
#include <AceTime.h>
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
    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    // Number of minutes to use for a DST offset.
    static const int16_t kDstOffsetMinutes = 60;

    /**
     * Constructor.
     * @param clock source of the current time
     * @param persistentStore stores objects into the EEPROM with CRC
     * @param presenter renders the date and time info to the screen
     * @param zoneManager optional zoneManager for TIME_ZONE_TYPE_BASIC or
     *        TIME_ZONE_TYPE_EXTENDED
     * @param displayZones array of TimeZoneData with NUM_TIME_ZONES elements
     * @param rootModeGroup poniter to the top level ModeGroup object
     */
    Controller(
        SystemClock& clock,
        PersistentStore& persistentStore,
        Presenter& presenter,
        ZoneManager& zoneManager,
        TimeZoneData initialTimeZoneData,
        ModeGroup const* rootModeGroup
    ) :
        mClock(clock),
        mPersistentStore(persistentStore),
        mPresenter(presenter),
        mZoneManager(zoneManager),
        mInitialTimeZoneData(initialTimeZoneData),
        mNavigator(rootModeGroup)
    {}

    void setup(bool factoryReset) {
      if (FORCE_INITIALIZE == 1) {
        factoryReset = true;
      }
      restoreClockInfo(factoryReset);
      mNavigator.setup();

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
      if (mNavigator.mode() == (uint8_t) Mode::kUnknown) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();
      mPresenter.display();
    }

    /**
     * Go to the next Mode, either the next screen or the next editable field.
     */
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

      #if TIME_ZONE_TYPE != TIME_ZONE_TYPE_MANUAL
      switch ((Mode) mNavigator.mode()) {
        case Mode::kChangeTimeZoneName:
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZoneData.zoneId);
          break;

        default:
          break;
      }
      #endif
    }

    void performLeavingModeAction() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeAction()"));
      }
    }

    /** Toggle edit mode. The editable field will start blinking. */
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

      switch ((Mode) mNavigator.mode()) {
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

      switch ((Mode) mNavigator.mode()) {
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
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
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

      switch ((Mode) mNavigator.mode()) {
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
          saveClockInfo();
          break;

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
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
      switch ((Mode) mNavigator.mode()) {
        // Switch 12/24 modes if in Mode::kDataTime
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
        case Mode::kChangeTimeZoneOffset: {
          mSuppressBlink = true;

          TimeZone tz = mZoneManager.createForTimeZoneData(
              mChangingClockInfo.timeZoneData);
          TimeOffset offset = tz.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          tz.setStdOffset(offset);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          break;
        }

        case Mode::kChangeTimeZoneDst: {
          mSuppressBlink = true;

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
        // Cycle through the zones in the registry
        case Mode::kChangeTimeZoneName: {
          mSuppressBlink = true;
          mZoneRegistryIndex++;
          if (mZoneRegistryIndex >= mZoneManager.zoneRegistrySize()) {
            mZoneRegistryIndex = 0;
          }
          TimeZone tz = mZoneManager.createForZoneIndex(mZoneRegistryIndex);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          mChangingClockInfo.dateTime =
              mChangingClockInfo.dateTime.convertToTimeZone(tz);
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
      // Ignore 12/24 changes from RepeatPressed events.
      //  * It doesn't make sense to repeatedly change the 12/24 mode when the
      //    button is held down.
      //  * Each change of 12/24 mode causes a write to the EEPPROM, which
      //    causes wear and tear.
      if (mNavigator.mode() != (uint8_t) Mode::kViewDateTime) {
        handleChangeButtonPress();
      }
    }

    void handleChangeButtonRelease() {
      switch ((Mode) mNavigator.mode()) {
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
    void updateDateTime() {
      acetime_t nowSeconds = mClock.getNow();
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(nowSeconds, tz);

      //acetime_t lastSync = mClock.getLastSyncTime();
      int32_t secondsSinceSyncAttempt = mClock.getSecondsSinceSyncAttempt();
      int32_t secondsToSyncAttempt = mClock.getSecondsToSyncAttempt();
      mClockInfo.prevSync = TimePeriod(secondsSinceSyncAttempt);
      mClockInfo.nextSync = TimePeriod(secondsToSyncAttempt);
      mClockInfo.clockSkew = TimePeriod(mClock.getClockSkew());
      mClockInfo.syncStatusCode = mClock.getSyncStatusCode();

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update the mChangingDateTime.second field with the current second.
      switch ((Mode) mNavigator.mode()) {
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

      switch ((Mode) mNavigator.mode()) {
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
          (Mode) mNavigator.mode(), blinkShowState, *clockInfo);
    }

    /** Save the current UTC dateTime to the RTC. */
    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    /** Transfer info from ChangingClockInfo to ClockInfo. */
    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      mClockInfo = mChangingClockInfo;
      preserveClockInfo(mClockInfo);
    }

    /** Save the clock info into EEPROM. */
    void preserveClockInfo(const ClockInfo& clockInfo) {
      if (ENABLE_SERIAL_DEBUG >= 2) {
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
      clockInfo.timeZoneData = storedInfo.timeZoneData;
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
      storedInfo.timeZoneData = clockInfo.timeZoneData;
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

    /** Set up the initial ClockInfo states. */
    void setupClockInfo() {
      mClockInfo.hourMode = ClockInfo::kTwentyFour;
      mClockInfo.timeZoneData = mInitialTimeZoneData;

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mClockInfo.backlightLevel = 0;
      mClockInfo.contrast = LCD_INITIAL_CONTRAST;
      mClockInfo.bias = LCD_INITIAL_BIAS;
    #else
      mClockInfo.contrastLevel = 5;
      mClockInfo.invertDisplay = ClockInfo::kInvertDisplayOff;
    #endif
    }

  private:
    SystemClock& mClock;
    PersistentStore& mPersistentStore;
    Presenter& mPresenter;
    ZoneManager& mZoneManager;
    TimeZoneData mInitialTimeZoneData;
    ModeNavigator mNavigator;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    uint16_t mZoneRegistryIndex;
    bool mSecondFieldCleared;

    // Handle blinking
    bool mSuppressBlink; // true if blinking should be suppressed
    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
