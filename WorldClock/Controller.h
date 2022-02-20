#ifndef WORLD_CLOCK_CONTROLLER_H
#define WORLD_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <crc_eeprom/crc_eeprom.h> // from AceUtils
#include <mode_group/mode_group.h> // from AceUtils
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
 * Maintains the internal state of the world clock, handling button inputs,
 * and calling out to the Presenter to display the clock. In an MVC
 * architecture, this would be the Controller. The Model would be the various
 * member variables in thic class. The View layer is the Presenter class.
 */
class Controller {
  public:
    /**
     * Constructor.
     * @param clock source of the current time
     * @param crcEeprom stores objects into the EEPROM with CRC
     * @param presenter renders the date and time info to the screen
     */
    Controller(
        Clock& clock,
        PersistentStore& persistentStore,
        const ModeGroup* rootModeGroup,
        Presenter& presenter0,
        Presenter& presenter1,
        Presenter& presenter2,
        const TimeZone& tz0,
        const TimeZone& tz1,
        const TimeZone& tz2,
        const char* name0,
        const char* name1,
        const char* name2
    ) :
        mClock(clock),
        mPersistentStore(persistentStore),
        mNavigator(rootModeGroup),
        mPresenter0(presenter0),
        mPresenter1(presenter1),
        mPresenter2(presenter2),
        mClockInfo0(tz0, name0),
        mClockInfo1(tz1, name1),
        mClockInfo2(tz2, name2)
    {}

    /** Initialize the controller with the various time zones of each clock. */
    void setup(bool factoryReset) {
      if (FORCE_INITIALIZE == 1) {
        factoryReset = true;
      }
      restoreClockInfo(factoryReset);
      updateDateTime();
    }

    /**
     * In other Controller::update() methods, this method not only updates
     * the RenderingInfo, but also synchronously calls the mPresenter.display(),
     * to render the display . But for WorldClock, it has 3 OLED displays, and
     * updating them synchronous takes too long, and interferes with the
     * double-click detection of AceButton.
     *
     * On a 16 MHz Pro Micro, calling 3 Presenter::display() takes an average of
     * 21-33 millis, but as much as 165 millis when all 3 need to re-render. The
     * solution is to decouple the rendering of the 3 displays, and rendering
     * each of them separately, with a "yield" functionality between each
     * display that other things, like AceButton, can do some of its own work.
     *
     * We could have used a finite state machine inside this Controller object
     * to implement the staggered rendering of the 3 displays. But it was a lot
     * easier to take advantage of the inherent advantages of a Coroutine,
     * and call the updatePresenterN() methods separately from the coroutine.
     */
    void update() {
      if (mNavigator.modeId() == (uint8_t) Mode::kUnknown) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();
    }

    // These are exposed as public methods so that the
    // COROUTINE(updateController) can update each Presenter separately. They
    // should be called 5-10 times a second to support blinking mode and to
    // avoid noticeable drift against the RTC which has a 1 second resolution.
    void updatePresenter0() { mPresenter0.display(); }
    void updatePresenter1() { mPresenter1.display(); }
    void updatePresenter2() { mPresenter2.display(); }

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      mNavigator.changeMode();
      performEnteringModeAction();
    }

    /** Toggle edit mode. The editable field will start blinking. */
    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
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
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:

        case Mode::kChangeHourMode:
        case Mode::kChangeBlinkingColon:
        case Mode::kChangeContrast:
        case Mode::kChangeInvertDisplay:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
        case Mode::kChangeTimeZoneDst1:
        case Mode::kChangeTimeZoneDst2:
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

    void performEnteringModeAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeAction()"));
      }
    }

    void performLeavingModeAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeAction()"));
      }
    }

    void performEnteringModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeGroupAction()"));
      }

      // Update mChangingDateTime for all modes. Even when changing only the
      // Clock Info settings, the current date and time is required to make sure
      // that the display is updated correctly when invertDisplay is set to
      // kInvertDisplayAuto.
      mChangingDateTime = ZonedDateTime::forEpochSeconds(
          mClock.getNow(), mClockInfo0.timeZone);
      mSecondFieldCleared = false;
    }

    void performLeavingModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
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

        case Mode::kChangeHourMode:
        case Mode::kChangeBlinkingColon:
        case Mode::kChangeContrast:
        case Mode::kChangeInvertDisplay:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
        case Mode::kChangeTimeZoneDst1:
        case Mode::kChangeTimeZoneDst2:
      #endif
          saveClockInfo();
          break;

        default:
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
          mSuppressBlink = true;
        #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
          zoned_date_time_mutation::incrementYear(mChangingDateTime);
        #else
          {
            auto& dateTime = mChangingDateTime;
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
          zoned_date_time_mutation::incrementMonth(mChangingDateTime);
          break;
        case Mode::kChangeDay:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementDay(mChangingDateTime);
          break;
        case Mode::kChangeHour:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementHour(mChangingDateTime);
          break;
        case Mode::kChangeMinute:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMinute(mChangingDateTime);
          break;
        case Mode::kChangeSecond:
          mSuppressBlink = true;
          mChangingDateTime.second(0);
          mSecondFieldCleared = true;
          break;

        case Mode::kChangeHourMode:
          mSuppressBlink = true;
          mClockInfo0.hourMode ^= 0x1;
          mClockInfo1.hourMode ^= 0x1;
          mClockInfo2.hourMode ^= 0x1;
          break;

        case Mode::kChangeBlinkingColon:
          mSuppressBlink = true;
          mClockInfo0.blinkingColon = !mClockInfo0.blinkingColon;
          mClockInfo1.blinkingColon = !mClockInfo1.blinkingColon;
          mClockInfo2.blinkingColon = !mClockInfo2.blinkingColon;
          break;

        case Mode::kChangeContrast:
          mSuppressBlink = true;
          incrementMod(mClockInfo0.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo1.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo2.contrastLevel, (uint8_t) 10);
          break;

        case Mode::kChangeInvertDisplay:
          mSuppressBlink = true;
          incrementMod(mClockInfo0.invertDisplay, (uint8_t) 5);
          incrementMod(mClockInfo1.invertDisplay, (uint8_t) 5);
          incrementMod(mClockInfo2.invertDisplay, (uint8_t) 5);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
          mSuppressBlink = true;
          mClockInfo0.timeZone.setDst(!mClockInfo0.timeZone.isDst());
          break;

        case Mode::kChangeTimeZoneDst1:
          mSuppressBlink = true;
          mClockInfo1.timeZone.setDst(!mClockInfo1.timeZone.isDst());
          break;

        case Mode::kChangeTimeZoneDst2:
          mSuppressBlink = true;
          mClockInfo2.timeZone.setDst(!mClockInfo2.timeZone.isDst());
          break;
      #endif

        default:
          break;
      }

      // Update Display0 right away to prevent jitters in the display when the
      // button is triggering RepeatPressed events. Display1 and Display2 will
      // follow, perhaps slightly behind the Display0, but that's ok.
      update();
      updatePresenter0();
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
        case Mode::kChangeHourMode:
        case Mode::kChangeBlinkingColon:
        case Mode::kChangeContrast:
        case Mode::kChangeInvertDisplay:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
        case Mode::kChangeTimeZoneDst1:
        case Mode::kChangeTimeZoneDst2:
      #endif
          mSuppressBlink = false;
          break;

        default:
          break;
      }
    }

  protected:
    void updateDateTime() {
      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update the mChangingDateTime.second field with the current second.
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          if (!mSecondFieldCleared) {
            ZonedDateTime dt = ZonedDateTime::forEpochSeconds(
                mClock.getNow(), TimeZone());
            mChangingDateTime.second(dt.second());
          }
          break;

        default:
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

    void updateRenderingInfo() {
      switch ((Mode) mNavigator.modeId()) {
        case Mode::kViewDateTime:
        case Mode::kViewSettings:
        case Mode::kViewAbout:
        {
          bool blinkShowState = mBlinkShowState || mSuppressBlink;
          Mode mode = (Mode) mNavigator.modeId();
          acetime_t now = mClock.getNow();
          mPresenter0.setRenderingInfo(
              mode, now, blinkShowState, mClockInfo0, mClockInfo0.timeZone);
          mPresenter1.setRenderingInfo(
              mode, now, blinkShowState, mClockInfo1, mClockInfo0.timeZone);
          mPresenter2.setRenderingInfo(
              mode, now, blinkShowState, mClockInfo2, mClockInfo0.timeZone);
          break;
        }

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
        case Mode::kChangeHourMode:
        case Mode::kChangeBlinkingColon:
        case Mode::kChangeContrast:
        case Mode::kChangeInvertDisplay:
        {
          acetime_t now = mChangingDateTime.toEpochSeconds();
          bool blinkShowState = mBlinkShowState || mSuppressBlink;
          Mode mode = (Mode) mNavigator.modeId();
          mPresenter0.setRenderingInfo(
              mode, now, blinkShowState, mClockInfo0, mClockInfo0.timeZone);
          mPresenter1.setRenderingInfo(
              mode, now, true, mClockInfo1, mClockInfo0.timeZone);
          mPresenter2.setRenderingInfo(
              mode, now, true, mClockInfo2, mClockInfo0.timeZone);
          break;
        }

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
          updateChangingDst(0);
          break;
        case Mode::kChangeTimeZoneDst1:
          updateChangingDst(1);
          break;
        case Mode::kChangeTimeZoneDst2:
          updateChangingDst(2);
          break;
      #endif

        default:
          break;
      }
    }

    void updateChangingDst(uint8_t clockId) {
      acetime_t now = mChangingDateTime.toEpochSeconds();
      mPresenter0.setRenderingInfo(
          (Mode) mNavigator.modeId(),
          now,
          mBlinkShowState || clockId!=0,
          mClockInfo0,
          mClockInfo0.timeZone);
      mPresenter1.setRenderingInfo(
          (Mode) mNavigator.modeId(),
          now,
          mBlinkShowState || clockId!=1,
          mClockInfo1,
          mClockInfo0.timeZone);
      mPresenter2.setRenderingInfo(
          (Mode) mNavigator.modeId(),
          now,
          mBlinkShowState || clockId!=2,
          mClockInfo2,
          mClockInfo0.timeZone);
    }

    /** Save the current UTC ZonedDateTime to the RTC. */
    void saveDateTime() {
      mChangingDateTime.normalize();
      mClock.setNow(mChangingDateTime.toEpochSeconds());
    }

    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      preserveClockInfo();
    }

    /** Save to EEPROM. */
    void preserveClockInfo() {
      StoredInfo storedInfo;
      storedInfoFromClockInfo(storedInfo);
      mPersistentStore.writeStoredInfo(storedInfo);
    }

    /** Restore from EEPROM. If that fails, set initial states. */
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
        clockInfoFromStoredInfo(storedInfo);
      } else {
        setupClockInfo();
        preserveClockInfo();
      }
    }

    /** Set various mClockInfo to their initial states. */
    void setupClockInfo() {
      mClockInfo0.hourMode = ClockInfo::kTwelve;
      mClockInfo1.hourMode = ClockInfo::kTwelve;
      mClockInfo2.hourMode = ClockInfo::kTwelve;

      mClockInfo0.blinkingColon = false;
      mClockInfo1.blinkingColon = false;
      mClockInfo2.blinkingColon = false;

      mClockInfo0.contrastLevel = 5;
      mClockInfo1.contrastLevel = 5;
      mClockInfo2.contrastLevel = 5;

      mClockInfo0.invertDisplay = 0;
      mClockInfo1.invertDisplay = 0;
      mClockInfo2.invertDisplay = 0;
    }

    /** Set the various mClockInfo{N} from the given storedInfo. */
    void clockInfoFromStoredInfo(const StoredInfo& storedInfo) {
      mClockInfo0.hourMode = storedInfo.hourMode;
      mClockInfo1.hourMode = storedInfo.hourMode;
      mClockInfo2.hourMode = storedInfo.hourMode;

      mClockInfo0.blinkingColon = storedInfo.blinkingColon;
      mClockInfo1.blinkingColon = storedInfo.blinkingColon;
      mClockInfo2.blinkingColon = storedInfo.blinkingColon;

      mClockInfo0.contrastLevel = storedInfo.contrastLevel;
      mClockInfo1.contrastLevel = storedInfo.contrastLevel;
      mClockInfo2.contrastLevel = storedInfo.contrastLevel;

      mClockInfo0.invertDisplay = storedInfo.invertDisplay;
      mClockInfo1.invertDisplay = storedInfo.invertDisplay;
      mClockInfo2.invertDisplay = storedInfo.invertDisplay;

    #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
      mClockInfo0.timeZone.setDst(storedInfo.isDst0);
      mClockInfo1.timeZone.setDst(storedInfo.isDst1);
      mClockInfo2.timeZone.setDst(storedInfo.isDst2);
    #endif
    }

    /**
     * Set the given storedInfo from the various mClockInfo{N}. Currently, only
     * the mClockInfo0 is used, but this can change in the future if we allow
     * the user to set the time zone of each clock at run time.
     */
    void storedInfoFromClockInfo(StoredInfo& storedInfo) {
      // Create hourMode and blinkingColon from clock0. The others will be
      // identical.
      storedInfo.hourMode = mClockInfo0.hourMode;
      storedInfo.blinkingColon = mClockInfo0.blinkingColon;
      storedInfo.contrastLevel = mClockInfo0.contrastLevel;
      storedInfo.invertDisplay = mClockInfo0.invertDisplay;

    #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
      storedInfo.isDst0 = mClockInfo0.timeZone.isDst();
      storedInfo.isDst1 = mClockInfo1.timeZone.isDst();
      storedInfo.isDst2 = mClockInfo2.timeZone.isDst();
    #endif
    }

  private:
    // Disable copy-constructor and assignment operator
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    Clock& mClock;
    PersistentStore& mPersistentStore;
    ModeNavigator mNavigator;

    Presenter& mPresenter0;
    Presenter& mPresenter1;
    Presenter& mPresenter2;
    ClockInfo mClockInfo0;
    ClockInfo mClockInfo1;
    ClockInfo mClockInfo2;

    ZonedDateTime mChangingDateTime; // source of now() in "Change" modes
    bool mSecondFieldCleared = false;

    // Handle blinking
    bool mSuppressBlink = false; // true if blinking should be suppressed
    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
