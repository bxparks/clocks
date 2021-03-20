#ifndef WORLD_CLOCK_CONTROLLER_H
#define WORLD_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include <AceUtilsModeGroup.h>
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_common::incrementMod;
using ace_utils::crc_eeprom::CrcEeprom;
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
    static const uint16_t kStoredInfoEepromAddress = 0;

    /**
     * Constructor.
     * @param clock source of the current time
     * @param crcEeprom stores objects into the EEPROM with CRC
     * @param presenter renders the date and time info to the screen
     */
    Controller(
        Clock& clock,
        CrcEeprom& crcEeprom,
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
        mCrcEeprom(crcEeprom),
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
      mNavigator.setup();
      updateDateTime();
    }

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void update() {
      if (mNavigator.mode() == MODE_UNKNOWN) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();

      // On a 16 MHz Pro Micro, calling Presenter::display() on 3 Presenters
      // takes an average of 21-33 millis, but sometimes as much as 165 millis,
      // probably when the 3 displays need to be redrawn. Blocking the
      // global loop() function for such a long time interferes with the
      // double-click detection of AceButton. The solution is to break up the
      // update of the 3 displays into 3 separate parts, so that other things,
      // like AceButton, can do some of its own work.
      //
      // One implementation is to use a somewhat complicated finite state
      // machine inside the Controller, where each of the 3 displays updates
      // upon different calls to the update(), splitting the cost of updating
      // over multiple calls to update(). This allows other things, like
      // AceButton to perform some of its tasks.
      //
      // The second implementation that seems far easier, and is implemented
      // currently, is to use take advantage of the inherent features of a
      // coroutine, modify COROUTINE(updateController) in WorldClock.ino to
      // split up the rendering of the 3 displays into 3 parts within that
      // coroutine. Essentially, this solution implements the FSM of the first
      // solution, but in a way that is simpler to understand.
      /*
      mPresenter0.display();
      mPresenter1.display();
      mPresenter2.display();
      */
    }

    // These are exposed as public methods so that the
    // COROUTINE(updateController) can update each Presenter separately.
    void updatePresenter0() { mPresenter0.display(); }
    void updatePresenter1() { mPresenter1.display(); }
    void updatePresenter2() { mPresenter2.display(); }

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      mNavigator.changeMode();
      performEnteringModeAction();
    }

    /** Toggle edit mode. The editable field will start blinking. */
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

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch (mNavigator.mode()) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:

        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
        case MODE_CHANGE_TIME_ZONE_DST1:
        case MODE_CHANGE_TIME_ZONE_DST2:
      #endif
          // Throw away the changes and just change the mode group.
          //performLeavingModeAction();
          //performLeavingModeGroupAction();
          mNavigator.changeGroup();
          performEnteringModeGroupAction();
          performEnteringModeAction();
          break;
      }
    }

    void performEnteringModeAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeAction()"));
      }
    }

    void performLeavingModeAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeAction()"));
      }
    }

    void performEnteringModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeGroupAction()"));
      }

      switch (mNavigator.mode()) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          mChangingDateTime = ZonedDateTime::forEpochSeconds(
              mClock.getNow(), mClockInfo0.timeZone);
          mSecondFieldCleared = false;
          break;
      }
    }

    void performLeavingModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performLeavingModeGroupAction()"));
      }

      switch (mNavigator.mode()) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          saveDateTime();
          break;

        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
        case MODE_CHANGE_TIME_ZONE_DST1:
        case MODE_CHANGE_TIME_ZONE_DST2:
      #endif
          saveClockInfo();
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch (mNavigator.mode()) {
        case MODE_CHANGE_YEAR:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementYear(mChangingDateTime);
          break;
        case MODE_CHANGE_MONTH:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMonth(mChangingDateTime);
          break;
        case MODE_CHANGE_DAY:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementDay(mChangingDateTime);
          break;
        case MODE_CHANGE_HOUR:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementHour(mChangingDateTime);
          break;
        case MODE_CHANGE_MINUTE:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMinute(mChangingDateTime);
          break;
        case MODE_CHANGE_SECOND:
          mSuppressBlink = true;
          mChangingDateTime.second(0);
          mSecondFieldCleared = true;
          break;

        case MODE_CHANGE_HOUR_MODE:
          mSuppressBlink = true;
          mClockInfo0.hourMode ^= 0x1;
          mClockInfo1.hourMode ^= 0x1;
          mClockInfo2.hourMode ^= 0x1;
          break;

        case MODE_CHANGE_BLINKING_COLON:
          mSuppressBlink = true;
          mClockInfo0.blinkingColon = !mClockInfo0.blinkingColon;
          mClockInfo1.blinkingColon = !mClockInfo1.blinkingColon;
          mClockInfo2.blinkingColon = !mClockInfo2.blinkingColon;
          break;

        case MODE_CHANGE_CONTRAST:
          mSuppressBlink = true;
          incrementMod(mClockInfo0.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo1.contrastLevel, (uint8_t) 10);
          incrementMod(mClockInfo2.contrastLevel, (uint8_t) 10);
          break;

        case MODE_CHANGE_INVERT_DISPLAY:
          mSuppressBlink = true;
          incrementMod(mClockInfo0.invertDisplay, (uint8_t) 2);
          incrementMod(mClockInfo1.invertDisplay, (uint8_t) 2);
          incrementMod(mClockInfo2.invertDisplay, (uint8_t) 2);
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
          mSuppressBlink = true;
          mClockInfo0.timeZone.setDst(!mClockInfo0.timeZone.isDst());
          break;

        case MODE_CHANGE_TIME_ZONE_DST1:
          mSuppressBlink = true;
          mClockInfo1.timeZone.setDst(!mClockInfo1.timeZone.isDst());
          break;

        case MODE_CHANGE_TIME_ZONE_DST2:
          mSuppressBlink = true;
          mClockInfo2.timeZone.setDst(!mClockInfo2.timeZone.isDst());
          break;
      #endif
      }

      // Update the display right away to prevent jitters in the display when
      // the button is triggering RepeatPressed events.
      update();
      updatePresenter0();
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
        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
        case MODE_CHANGE_TIME_ZONE_DST1:
        case MODE_CHANGE_TIME_ZONE_DST2:
      #endif
          mSuppressBlink = false;
          break;
      }
    }

  protected:
    void updateDateTime() {
      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update the mChangingDateTime.second field with the current second.
      switch (mNavigator.mode()) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          if (!mSecondFieldCleared) {
            ZonedDateTime dt = ZonedDateTime::forEpochSeconds(
                mClock.getNow(), TimeZone());
            mChangingDateTime.second(dt.second());
          }
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
      switch (mNavigator.mode()) {
        case MODE_DATE_TIME:
        case MODE_SETTINGS:
        case MODE_ABOUT:
        {
          bool blinkShowState = mBlinkShowState || mSuppressBlink;
          uint8_t mode = mNavigator.mode();
          acetime_t now = mClock.getNow();
          mPresenter0.update(mode, now, blinkShowState, mClockInfo0);
          mPresenter1.update(mode, now, blinkShowState, mClockInfo1);
          mPresenter2.update(mode, now, blinkShowState, mClockInfo2);
          break;
        }

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
        {
          acetime_t now = mChangingDateTime.toEpochSeconds();
          bool blinkShowState = mBlinkShowState || mSuppressBlink;
          uint8_t mode = mNavigator.mode();
          mPresenter0.update(mode, now, blinkShowState, mClockInfo0);
          mPresenter1.update(mode, now, true, mClockInfo1);
          mPresenter2.update(mode, now, true, mClockInfo2);
          break;
        }

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
          updateChangingDst(0);
          break;
        case MODE_CHANGE_TIME_ZONE_DST1:
          updateChangingDst(1);
          break;
        case MODE_CHANGE_TIME_ZONE_DST2:
          updateChangingDst(2);
          break;
      #endif
      }
    }

    void updateChangingDst(uint8_t clockId) {
      acetime_t now = mChangingDateTime.toEpochSeconds();
      mPresenter0.update(
          mNavigator.mode(), now, mBlinkShowState || clockId!=0, mClockInfo0);
      mPresenter1.update(
          mNavigator.mode(), now, mBlinkShowState || clockId!=1, mClockInfo1);
      mPresenter2.update(
          mNavigator.mode(), now, mBlinkShowState || clockId!=2, mClockInfo2);
    }

    /** Save the current UTC ZonedDateTime to the RTC. */
    void saveDateTime() {
      mClock.setNow(mChangingDateTime.toEpochSeconds());
    }

    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      preserveClockInfo();
    }

    /** Save to EEPROM. */
    void preserveClockInfo() {
      StoredInfo storedInfo;
      storedInfoFromClockInfo(storedInfo);
      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, storedInfo);
    }

    /** Restore from EEPROM. If that fails, set initial states. */
    void restoreClockInfo(bool factoryReset) {
      StoredInfo storedInfo;
      bool isValid;

      if (factoryReset) {
        if (ENABLE_SERIAL_DEBUG == 1) {
          SERIAL_PORT_MONITOR.println(F("restoreClockInfo(): FACTORY RESET"));
        }
        isValid = false;
      } else {
        isValid = mCrcEeprom.readWithCrc(kStoredInfoEepromAddress, storedInfo);
        if (ENABLE_SERIAL_DEBUG == 1) {
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
    CrcEeprom& mCrcEeprom;
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
