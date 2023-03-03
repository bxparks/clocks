#ifndef WORLD_CLOCK_CONTROLLER_H
#define WORLD_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <crc_eeprom/crc_eeprom.h> // from AceUtils
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_common::incrementMod;

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
        mPresenter0(presenter0),
        mPresenter1(presenter1),
        mPresenter2(presenter2)
    {
      mClockInfo0.mode = Mode::kViewDateTime;

      mClockInfo0.timeZone = tz0;
      mClockInfo0.name = name0;
      mClockInfo1.timeZone = tz1;
      mClockInfo1.name = name1;
      mClockInfo2.timeZone = tz2;
      mClockInfo2.name = name2;
    }

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
     * the ClockInfo, but also synchronously calls the mPresenter.display(),
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
      if (mClockInfo0.mode == Mode::kUnknown) return;
      updateDateTime();
      updatePresenter();
      updateInvertState();
    }

    void updateBlinkState () {
      mClockInfo0.blinkShowState = !mClockInfo0.blinkShowState;
      mClockInfo1.blinkShowState = !mClockInfo1.blinkShowState;
      mClockInfo2.blinkShowState = !mClockInfo2.blinkShowState;
      mChangingClockInfo.blinkShowState = !mChangingClockInfo.blinkShowState;

      updatePresenter();
    }

    // These are exposed as public methods so that the
    // COROUTINE(updateController) can update each Presenter separately,
    // interspersed with calls to COROUTINE_YIELD(). They should be called 5-10
    // times a second to support blinking mode and to avoid noticeable drift
    // against the RTC which has a 1 second resolution.
    void updatePresenter0() { mPresenter0.display(); }
    void updatePresenter1() { mPresenter1.display(); }
    void updatePresenter2() { mPresenter2.display(); }

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }

      switch (mClockInfo0.mode) {
        // View modes
        case Mode::kViewDateTime:
          mClockInfo0.mode = Mode::kViewSettings;
          break;
        case Mode::kViewSettings:
          mClockInfo0.mode = Mode::kViewAbout;
          break;
        case Mode::kViewAbout:
          mClockInfo0.mode = Mode::kViewDateTime;
          break;

        // Change Date/Time
        case Mode::kChangeHour:
          mClockInfo0.mode = Mode::kChangeMinute;
          break;
        case Mode::kChangeMinute:
        #if 0
          mClockInfo0.mode = Mode::kChangeSecond;
          break;
        case Mode::kChangeSecond:
        #endif
          mClockInfo0.mode = Mode::kChangeYear;
          break;
        case Mode::kChangeYear:
          mClockInfo0.mode = Mode::kChangeMonth;
          break;
        case Mode::kChangeMonth:
          mClockInfo0.mode = Mode::kChangeDay;
          break;
        case Mode::kChangeDay:
          mClockInfo0.mode = Mode::kChangeHour;
          break;

        // Change Settings
        case Mode::kChangeHourMode:
          mClockInfo0.mode = Mode::kChangeBlinkingColon;
          break;
        case Mode::kChangeBlinkingColon:
          mClockInfo0.mode = Mode::kChangeContrast;
          break;
        case Mode::kChangeContrast:
          mClockInfo0.mode = Mode::kChangeInvertDisplay;
          break;
        case Mode::kChangeInvertDisplay:
          mClockInfo0.mode = Mode::kChangeHourMode;
          break;

        default:
          break;
      }

      mChangingClockInfo.mode = mClockInfo0.mode;
      mClockInfo1.mode = mClockInfo0.mode;
      mClockInfo2.mode = mClockInfo0.mode;
    }

    /** Toggle edit mode. The editable field will start blinking. */
    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      switch (mClockInfo0.mode) {
        // Long Press in View modes changes to Change modes.
        case Mode::kViewDateTime:
          mClockInfo0.mode = Mode::kChangeYear;
          mChangingClockInfo = mClockInfo0;
          initChangingClock();
          mSecondFieldCleared = false;
          break;

        case Mode::kViewSettings:
          mClockInfo0.mode = Mode::kChangeHourMode;
          mChangingClockInfo = mClockInfo0;
          initChangingClock();
          break;

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          saveDateTime();
          mClockInfo0.mode = Mode::kViewDateTime;
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
          mClockInfo0.mode = Mode::kViewSettings;
          break;

        default:
          break;
      }

      mChangingClockInfo.mode = mClockInfo0.mode;
      mClockInfo1.mode = mClockInfo0.mode;
      mClockInfo2.mode = mClockInfo0.mode;
    }

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch (mClockInfo0.mode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          mClockInfo0.mode = Mode::kViewDateTime;
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
          mClockInfo0.mode = Mode::kViewSettings;
          break;

        default:
          break;
      }
    }

    /**
     * Update mChangingClockInfo after entering one of the change modes (e.g.
     * kChangeHour or kChangeHourMode). Even when changing only the Clock Info
     * settings, the current date and time is required to make sure that the
     * display is updated correctly when invertDisplay is set to
     * kInvertDisplayAuto.
     */
    void initChangingClock() {
      if (mChangingClockInfo.dateTime.isError()) {
        mChangingClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
            0, mChangingClockInfo.dateTime.timeZone());
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      mClockInfo0.suppressBlink = true;
      mClockInfo1.suppressBlink = true;
      mClockInfo2.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch (mClockInfo0.mode) {
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

        case Mode::kChangeHourMode:
          mClockInfo0.hourMode ^= 0x1;
          mClockInfo1.hourMode ^= 0x1;
          mClockInfo2.hourMode ^= 0x1;
          break;

        case Mode::kChangeBlinkingColon:
          mClockInfo0.blinkingColon = !mClockInfo0.blinkingColon;
          mClockInfo1.blinkingColon = !mClockInfo1.blinkingColon;
          mClockInfo2.blinkingColon = !mClockInfo2.blinkingColon;
          break;

        case Mode::kChangeContrast:
          incrementMod(mClockInfo0.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo1.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo2.contrastLevel, (uint8_t) 10);
          break;

        case Mode::kChangeInvertDisplay:
          incrementMod(mClockInfo0.invertDisplay, (uint8_t) 5);
          incrementMod(mClockInfo1.invertDisplay, (uint8_t) 5);
          incrementMod(mClockInfo2.invertDisplay, (uint8_t) 5);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
          mClockInfo0.timeZone.setDst(!mClockInfo0.timeZone.isDst());
          break;

        case Mode::kChangeTimeZoneDst1:
          mClockInfo1.timeZone.setDst(!mClockInfo1.timeZone.isDst());
          break;

        case Mode::kChangeTimeZoneDst2:
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
      switch (mClockInfo0.mode) {
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
          mClockInfo0.suppressBlink = false;
          mClockInfo1.suppressBlink = false;
          mClockInfo2.suppressBlink = false;
          mChangingClockInfo.suppressBlink = false;
          break;

        default:
          break;
      }
    }

  protected:
    void updateDateTime() {
      acetime_t now = mClock.getNow();
      mClockInfo0.dateTime = ZonedDateTime::forEpochSeconds(
          now, mClockInfo0.timeZone);
      mClockInfo1.dateTime = ZonedDateTime::forEpochSeconds(
          now, mClockInfo1.timeZone);
      mClockInfo2.dateTime = ZonedDateTime::forEpochSeconds(
          now, mClockInfo2.timeZone);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update the mChangingClockInfo.second field with the current second.
      switch (mClockInfo0.mode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          if (!mSecondFieldCleared) {
            mChangingClockInfo.dateTime.second(mClockInfo0.dateTime.second());
          }
          break;

        default:
          break;
      }
    }

    void updateInvertState() {
    }

    /**
     * Calculate the next actual invertDisplay setting. Automatically
     * alternating inversion is an attempt to extend the life-time of these
     * OLED devices which seem to suffer from burn-in after about 6-12 months.
     */
    static uint8_t calculateInvertState(ClockInfo& clockInfo) {
      uint8_t invertState;
      if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayMinutely
          || clockInfo.invertDisplay == ClockInfo::kInvertDisplayDaily
          || clockInfo.invertDisplay == ClockInfo::kInvertDisplayHourly) {

        const LocalDateTime& ldt = clockInfo.dateTime.localDateTime();

        // The XOR alternates the pattern of on/off to smooth out the wear level
        // on specific digits. For example, if kInvertDisplayMinutely is
        // selected, and if last bit of only the minute is used, then the "1" on
        // the minute segment will always be inverted, which will cause uneven
        // wearning. By XOR'ing with the hour(), we invert the on/off cycle
        // every hour.
        if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayMinutely) {
          invertState = ((ldt.minute() & 0x1) ^ (ldt.hour() & 0x1))
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        } else if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayHourly) {
          invertState = ((ldt.hour() & 0x1) ^ (ldt.day() & 0x1))
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        } else {
          invertState = (7 <= ldt.hour() && ldt.hour() < 19)
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        }
      } else {
        invertState = clockInfo.invertDisplay;
      }

      return invertState;
    }

    void updatePresenter() {
      ClockInfo* clockInfo;

      switch (mClockInfo0.mode) {
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
          clockInfo = &mChangingClockInfo;
          break;

        default:
          clockInfo = &mClockInfo0;
          break;

      }

      mClockInfo0.mode = clockInfo->mode;
      mClockInfo1.mode = clockInfo->mode;
      mClockInfo2.mode = clockInfo->mode;

      mClockInfo0.dateTime = clockInfo->dateTime;
      mClockInfo1.dateTime = ZonedDateTime::forEpochSeconds(
          clockInfo->dateTime.toEpochSeconds(), mClockInfo1.timeZone);
      mClockInfo2.dateTime = ZonedDateTime::forEpochSeconds(
          clockInfo->dateTime.toEpochSeconds(), mClockInfo2.timeZone);

      uint8_t invertState = calculateInvertState(*clockInfo);
      mClockInfo0.invertState = invertState;
      mClockInfo1.invertState = invertState;
      mClockInfo2.invertState = invertState;

      mPresenter0.setClockInfo(mClockInfo0);
      mPresenter1.setClockInfo(mClockInfo1);
      mPresenter2.setClockInfo(mClockInfo2);
    }

    /** Save the current UTC ZonedDateTime to the RTC. */
    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
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

    Presenter& mPresenter0;
    Presenter& mPresenter1;
    Presenter& mPresenter2;
    ClockInfo mClockInfo0;
    ClockInfo mClockInfo1;
    ClockInfo mClockInfo2;
    ClockInfo mChangingClockInfo;
    bool mSecondFieldCleared = false;
};

#endif
