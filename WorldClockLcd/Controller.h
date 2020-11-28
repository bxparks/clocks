#ifndef WORLD_CLOCK_LCD_CONTROLLER_H
#define WORLD_CLOCK_LCD_CONTROLLER_H

#include <AceCommon.h> // incrementMod
#include <AceTime.h>
#include "config.h"
#include "ClockInfo.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"
#include "ModeGroup.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_common::incrementMod;

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
     * @param zones array of TimeZoneData with NUM_TIME_ZONES elements
     * @param rootModeGroup poniter to the top level ModeGroup object
     * @param zoneManager optional zoneManager for TIME_ZONE_TYPE_BASIC or
     *        TIME_ZONE_TYPE_EXTENDED
     */
    Controller(
        PersistentStore& persistentStore,
        Clock& clock,
        Presenter& presenter,
        ZoneManager& zoneManager,
        TimeZoneData const* displayZones,
        ModeGroup const* rootModeGroup
    ) :
        mPersistentStore(persistentStore),
        mClock(clock),
        mPresenter(presenter),
        mZoneManager(zoneManager),
        mDisplayZones(displayZones),
        mCurrentModeGroup(rootModeGroup)
      {}

    /**
     * @param factoryReset set to true to perform a reset to factory defaults
     */
    void setup(bool factoryReset) {
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
      #endif

      #if FORCE_INITIALIZE
        factoryReset = true;
      #endif
      restoreClockInfo(factoryReset);

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        updateBacklight();
        updateLcdContrast();
        updateBias();
      #else
        updateContrast();
      #endif

      updateDateTime();
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

    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }
      performLeavingModeAction();
      changeSiblingMode();
      performEnteringModeAction();
    }

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

      switch (mMode) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE0_DST:
          mCurrentZone = &mChangingClockInfo.zones[0];
          break;
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_DST:
          mCurrentZone = &mChangingClockInfo.zones[1];
          break;
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_DST:
          mCurrentZone = &mChangingClockInfo.zones[2];
          break;
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_DST:
          mCurrentZone = &mChangingClockInfo.zones[3];
          break;
      #else
        case MODE_CHANGE_TIME_ZONE0_NAME:
          mCurrentZone = &mChangingClockInfo.zones[0];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case MODE_CHANGE_TIME_ZONE1_NAME:
          mCurrentZone = &mChangingClockInfo.zones[1];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case MODE_CHANGE_TIME_ZONE2_NAME:
          mCurrentZone = &mChangingClockInfo.zones[2];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
        case MODE_CHANGE_TIME_ZONE3_NAME:
          mCurrentZone = &mChangingClockInfo.zones[3];
          mZoneRegistryIndex =
              mZoneManager.indexForZoneId(mCurrentZone->zoneId);
          break;
      #endif
      }
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
      changeModeGroup();
      performEnteringModeGroupAction();
      performEnteringModeAction();
    }

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

    /** Do action associated with entering a ModeGroup due to a LongPress. */
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
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE0_DST:
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_DST:
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_DST:
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_DST:
      #else
        case MODE_CHANGE_TIME_ZONE0_NAME:
        case MODE_CHANGE_TIME_ZONE1_NAME:
        case MODE_CHANGE_TIME_ZONE2_NAME:
        case MODE_CHANGE_TIME_ZONE3_NAME:
      #endif
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          break;
      }
    }

    /** Do action associated with leaving a ModeGroup due to a LongPress. */
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

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE0_DST:
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_DST:
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_DST:
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_DST:
      #else
        case MODE_CHANGE_TIME_ZONE0_NAME:
        case MODE_CHANGE_TIME_ZONE1_NAME:
        case MODE_CHANGE_TIME_ZONE2_NAME:
        case MODE_CHANGE_TIME_ZONE3_NAME:
      #endif
          saveClockInfo();
          break;

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case MODE_CHANGE_SETTINGS_BACKLIGHT:
        case MODE_CHANGE_SETTINGS_CONTRAST:
        case MODE_CHANGE_SETTINGS_BIAS:
      #else
        case MODE_CHANGE_SETTINGS_CONTRAST:
      #endif
          preserveClockInfo(mClockInfo);
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch (mMode) {
        // Change button causes toggling of 12/24 modes if in MODE_DATE_TIME
        case MODE_DATE_TIME:
          mClockInfo.hourMode ^= 0x1;
          preserveClockInfo(mClockInfo);
          break;

        case MODE_CHANGE_YEAR:
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

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        {
          mSuppressBlink = true;
          if (ENABLE_SERIAL_DEBUG == 1) {
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

        case MODE_CHANGE_TIME_ZONE0_DST:
        case MODE_CHANGE_TIME_ZONE1_DST:
        case MODE_CHANGE_TIME_ZONE2_DST:
        case MODE_CHANGE_TIME_ZONE3_DST:
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
        case MODE_CHANGE_TIME_ZONE0_NAME:
        case MODE_CHANGE_TIME_ZONE1_NAME:
        case MODE_CHANGE_TIME_ZONE2_NAME:
        case MODE_CHANGE_TIME_ZONE3_NAME:
        {
          mSuppressBlink = true;
          mZoneRegistryIndex++;
          if (mZoneRegistryIndex >= mZoneManager.registrySize()) {
            mZoneRegistryIndex = 0;
          }
          TimeZone tz = mZoneManager.createForZoneIndex(mZoneRegistryIndex);
          *mCurrentZone = tz.toTimeZoneData();

          // TimeZone 0 is the default displayed timezone.
          if (mMode == MODE_CHANGE_TIME_ZONE0_NAME) {
            mChangingClockInfo.dateTime =
                mChangingClockInfo.dateTime.convertToTimeZone(tz);
          }
          break;
        }
      #endif

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case MODE_CHANGE_SETTINGS_BACKLIGHT:
        {
          mSuppressBlink = true;
          incrementMod(mClockInfo.backlightLevel, (uint8_t) 10);
          updateBacklight();
          break;
        }
        case MODE_CHANGE_SETTINGS_CONTRAST:
        {
          mSuppressBlink = true;
          incrementMod(mClockInfo.contrast, (uint8_t) 128);
          updateLcdContrast();
          break;
        }
        case MODE_CHANGE_SETTINGS_BIAS:
        {
          mSuppressBlink = true;
          incrementMod(mClockInfo.bias, (uint8_t) 8);
          updateBias();
          break;
        }
      #else
        case MODE_CHANGE_SETTINGS_CONTRAST:
        {
          mSuppressBlink = true;
          incrementMod(mClockInfo.contrastLevel, (uint8_t) 10);
          updateContrast();
          break;
        }
      #endif

      }

      // Update the display right away to prevent jitters in the display when
      // the button is triggering RepeatPressed events.
      update();
    }

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    void updateBacklight() {
      uint16_t value = getBacklightValue(mClockInfo.backlightLevel);
      mPresenter.setBrightness(value);

      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.print(F("updateBacklight(): "));
        SERIAL_PORT_MONITOR.println(value);
      }
    }

    /**
     * Return the pwm value for the given level. Since the backlight is driven
     * LOW (low values of PWM means brighter light), subtract from 1024.
     * @param level brightness level in the range of [0, 9]
     */
    static uint16_t getBacklightValue(uint8_t level) {
      if (level > 9) level = 9;
      return 1023 - kBacklightValues[level];
    }

    void updateLcdContrast() {
      mPresenter.setContrast(mClockInfo.contrast);

      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.print(F("updateLcdContrast(): "));
        SERIAL_PORT_MONITOR.println(mClockInfo.contrast);
      }
    }

    void updateBias() {
      mPresenter.setBias(mClockInfo.bias);

      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.print(F("updateBias(): "));
        SERIAL_PORT_MONITOR.println(mClockInfo.bias);
      }
    }

  #else
    void updateContrast() {
      uint8_t contrastValue = getContrastValue(mClockInfo.contrastLevel);
      mPresenter.setContrast(contrastValue);
    }

    static uint8_t getContrastValue(uint8_t level) {
      if (level > 9) level = 9;
      return kContrastValues[level];
    }
  #endif

    void handleChangeButtonRepeatPress() {
      // Ignore 12/24 changes from RepeatPressed events. 1) It doesn't make
      // sense to repeatedly change the 12/24 mode when the button is held
      // down. 2) Each change of 12/24 mode causes a write to the EEPPROM,
      // which causes wear and tear.
      if (mMode != MODE_DATE_TIME) {
        handleChangeButtonPress();
      }
    }

    void handleChangeButtonRelease() {
      switch (mMode) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE0_DST:
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_DST:
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_DST:
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_DST:
      #else
        case MODE_CHANGE_TIME_ZONE0_NAME:
        case MODE_CHANGE_TIME_ZONE1_NAME:
        case MODE_CHANGE_TIME_ZONE2_NAME:
        case MODE_CHANGE_TIME_ZONE3_NAME:
      #endif
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case MODE_CHANGE_SETTINGS_BACKLIGHT:
        case MODE_CHANGE_SETTINGS_CONTRAST:
        case MODE_CHANGE_SETTINGS_BIAS:
      #else
        case MODE_CHANGE_SETTINGS_CONTRAST:
      #endif
          mSuppressBlink = false;
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

      // If the dateTime is currently being changed, don't update the
      // 'mChangingClockInfo.dateTime' with the SystemClock since that would
      // clobber the fields entered by the user with the SystemClock. The
      // exception is the 'second' field which will continue to be updated...
      // unless the second field has been explicitly cleared. Then we keep it
      // pegged at '00' second.
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

        // We can ignore the MODE_CHANGE_XXX for display time zones because
        // those modes do not display the time, so we don't need to update
        // mChangingClockInfo.dateTime.
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

      switch (mMode) {
        // If we are changing the current date, time or time zones, render the
        // mChangingClockInfo instead, because changes are made to the copy,
        // instead of the current mClockInfo, and copied over to mClockInfo
        // upon LongPress.
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE0_OFFSET:
        case MODE_CHANGE_TIME_ZONE0_DST:
        case MODE_CHANGE_TIME_ZONE1_OFFSET:
        case MODE_CHANGE_TIME_ZONE1_DST:
        case MODE_CHANGE_TIME_ZONE2_OFFSET:
        case MODE_CHANGE_TIME_ZONE2_DST:
        case MODE_CHANGE_TIME_ZONE3_OFFSET:
        case MODE_CHANGE_TIME_ZONE3_DST:
      #else
        case MODE_CHANGE_TIME_ZONE0_NAME:
        case MODE_CHANGE_TIME_ZONE1_NAME:
        case MODE_CHANGE_TIME_ZONE2_NAME:
        case MODE_CHANGE_TIME_ZONE3_NAME:
      #endif
          clockInfo = &mChangingClockInfo;
          break;

        // For all other modes render the normal mClockInfo instead. This
        // includes the "change settings" modes, because those are applied
        // directly to the current mClockInfo, not the mChangingClockInfo.
        default:
          clockInfo = &mClockInfo;
      }

      mPresenter.setRenderingInfo(mMode, mSuppressBlink, mBlinkShowState,
          *clockInfo);
    }

    /** Save the changed dateTime to the SystemClock. */
    void saveDateTime() {
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    /** Transfer info from ChangingClockInfo to ClockInfo. */
    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      mClockInfo = mChangingClockInfo;
      preserveClockInfo(mClockInfo);
    }

    /** Save the clock info into EEPROM. */
    void preserveClockInfo(const ClockInfo& clockInfo) {
      if (ENABLE_SERIAL_DEBUG == 1) {
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
      #endif
    }

    /** Attempt to restore from EEPROM, otherwise use factory defaults. */
    void restoreClockInfo(bool factoryReset) {
      StoredInfo storedInfo;
      bool isValid;
      if (factoryReset) {
        if (ENABLE_SERIAL_DEBUG == 1) {
          SERIAL_PORT_MONITOR.println(F("restoreClockInfo(): FACTORY RESET"));
        }
        isValid = false;
      } else {
        isValid = mPersistentStore.readStoredInfo(storedInfo);
        isValid = true;
        if (ENABLE_SERIAL_DEBUG == 1) {
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
      mClockInfo.hourMode = StoredInfo::kTwentyFour;
      memcpy(mClockInfo.zones, mDisplayZones,
          sizeof(TimeZoneData) * NUM_TIME_ZONES);

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mClockInfo.backlightLevel = 0;
      mClockInfo.contrast = LCD_INITIAL_CONTRAST;
      mClockInfo.bias = LCD_INITIAL_BIAS;
    #else
      mClockInfo.contrastLevel = 5;
    #endif
    }

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    static const uint16_t kBacklightValues[];
    static const uint8_t kContrastValues[];
  #else
    static const uint8_t kContrastValues[];
  #endif

    PersistentStore& mPersistentStore;
    Clock& mClock;
    Presenter& mPresenter;
    ZoneManager& mZoneManager;
    TimeZoneData const* const mDisplayZones;
    ModeGroup const* mCurrentModeGroup;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    // Navigation
    uint8_t mTopLevelIndexSave = 0;
    uint8_t mCurrentModeIndex = 0;
    uint8_t mMode = MODE_DATE_TIME;

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

    bool mIsPreparingToSleep = false;
};

#endif
