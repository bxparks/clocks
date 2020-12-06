#ifndef WORLD_CLOCK_CONTROLLER_H
#define WORLD_CLOCK_CONTROLLER_H

#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include <CrcEeprom.h>
#include "RenderingInfo.h"
#include "ModeGroup.h"
#include "StoredInfo.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::clock;
using crc_eeprom::CrcEeprom;
using ace_common::incrementMod;

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
        ModeGroup* rootModeGroup,
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
        mCurrentModeGroup(rootModeGroup),
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
      mMode = MODE_DATE_TIME;
      updateContrast();
      updateDateTime();
    }

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void update() {
      if (mMode == MODE_UNKNOWN) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();

      mPresenter0.display();
      mPresenter1.display();
      mPresenter2.display();
    }

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      changeSiblingMode();
      performEnteringModeAction();
    }

    // ModeGroup common code
    /** Navigate to sibling mode. */
    void changeSiblingMode() {
      mCurrentModeIndex++;
      if (mCurrentModeGroup->modes[mCurrentModeIndex] == 0) {
        mCurrentModeIndex = 0;
      }
      mMode = mCurrentModeGroup->modes[mCurrentModeIndex];
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

    // ModeGroup common code
    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      performLeavingModeAction();
      performLeavingModeGroupAction();
      changeModeGroup();
      performEnteringModeGroupAction();
      performEnteringModeAction();
    }

    // ModeGroup common code
    /** Navigate to Mode Group. */
    void changeModeGroup() {
      const ModeGroup* parentGroup = mCurrentModeGroup->parentGroup;

      if (parentGroup) {
        mCurrentModeGroup = parentGroup;
        mCurrentModeIndex = mTopLevelIndexSave;
      } else {
        const ModeGroup* const* childGroups = mCurrentModeGroup->childGroups;
        const ModeGroup* childGroup = childGroups
            ? childGroups[mCurrentModeIndex]
            : nullptr;
        if (childGroup) {
          mCurrentModeGroup = childGroup;
          // Save the current top level index so that we can go back to it.
          mTopLevelIndexSave = mCurrentModeIndex;
          mCurrentModeIndex = 0;
        }
      }

      mMode = mCurrentModeGroup->modes[mCurrentModeIndex];
    }

    void performEnteringModeGroupAction() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("performEnteringModeGroupAction()"));
      }

      switch (mMode) {
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

      switch (mMode) {
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
      switch (mMode) {
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
          updateContrast();
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
    }

    /**
     * Set the brightness of the OLEDs to their states. We assume the
     * contrastLevel of all displays are identical, but that can be changed if
     * we really wanted independent control.
     */
    void updateContrast() {
      uint8_t contrastValue = getContrastValue(mClockInfo0.contrastLevel);
      mPresenter0.setContrast(contrastValue);
      mPresenter1.setContrast(contrastValue);
      mPresenter2.setContrast(contrastValue);
    }

    static uint8_t getContrastValue(uint8_t level) {
      if (level > 9) level = 9;
      return kContrastValues[level];
    }

    void handleChangeButtonRepeatPress() {
      handleChangeButtonPress();
    }

    void handleChangeButtonRelease() {
      switch (mMode) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
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
      switch (mMode) {
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
      switch (mMode) {
        case MODE_DATE_TIME:
        case MODE_SETTINGS:
        case MODE_ABOUT: {
          acetime_t now = mClock.getNow();
          mPresenter0.update(mMode, now, mBlinkShowState, mSuppressBlink,
              mClockInfo0);
          mPresenter1.update(mMode, now, mBlinkShowState, mSuppressBlink,
              mClockInfo1);
          mPresenter2.update(mMode, now, mBlinkShowState, mSuppressBlink,
              mClockInfo2);
          break;
        }

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON: {
        case MODE_CHANGE_CONTRAST:
          acetime_t now = mChangingDateTime.toEpochSeconds();
          mPresenter0.update(mMode, now, mBlinkShowState, mSuppressBlink,
              mClockInfo0);
          mPresenter1.update(mMode, now, mBlinkShowState, true, mClockInfo1);
          mPresenter2.update(mMode, now, mBlinkShowState, true, mClockInfo2);
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
      mPresenter0.update(mMode, now, mBlinkShowState, clockId!=0, mClockInfo0);
      mPresenter1.update(mMode, now, mBlinkShowState, clockId!=1, mClockInfo1);
      mPresenter2.update(mMode, now, mBlinkShowState, clockId!=2, mClockInfo2);
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

      mCrcEeprom.writeWithCrc(
          kStoredInfoEepromAddress,
          &storedInfo,
          sizeof(StoredInfo)
      );
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
        isValid = mCrcEeprom.readWithCrc(
            kStoredInfoEepromAddress,
            &storedInfo,
            sizeof(StoredInfo)
        );
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

    static const uint8_t kContrastValues[];

    Clock& mClock;
    CrcEeprom& mCrcEeprom;
    ModeGroup const* mCurrentModeGroup;
    Presenter& mPresenter0;
    Presenter& mPresenter1;
    Presenter& mPresenter2;
    ClockInfo mClockInfo0;
    ClockInfo mClockInfo1;
    ClockInfo mClockInfo2;

    // Navigation
    uint8_t mTopLevelIndexSave = 0;
    uint8_t mCurrentModeIndex = 0;
    uint8_t mMode = MODE_UNKNOWN; // current mode

    ZonedDateTime mChangingDateTime; // source of now() in "Change" modes
    bool mSecondFieldCleared = false;

    // Handle blinking
    bool mSuppressBlink = false; // true if blinking should be suppressed
    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
