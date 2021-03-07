#ifndef ONE_ZONE_CLOCK_CONTROLLER_H
#define ONE_ZONE_CLOCK_CONTROLLER_H

#include "config.h"
#include <AceCommon.h> // incrementMod()
#include <AceTime.h>
#include <AceUtilsModeGroup.h>
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
        Clock& clock,
        Presenter& presenter,
        ZoneManager& zoneManager,
        TimeZoneData initialTimeZoneData,
        ModeGroup const* rootModeGroup
    ) :
        mPersistentStore(persistentStore),
        mClock(clock),
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

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
        updateBacklight();
        updateLcdContrast();
        updateBias();
      #else
        updateContrast();
      #endif
        invertDisplay();

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
      mPresenter.display();
    }

    void handleModeButtonPress() {
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
              mChangingClockInfo.timeZoneData.zoneId);
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

    /** Do action associated with entering a ModeGroup due to a LongPress. */
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

    /** Do action associated with leaving a ModeGroup due to a LongPress. */
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

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
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
        case MODE_CHANGE_INVERT_DISPLAY:
          preserveClockInfo(mClockInfo);
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      switch (mNavigator.mode()) {
        // Switch 12/24 modes if in MODE_DATA_TIME
        case MODE_DATE_TIME:
          mClockInfo.hourMode ^= 0x1;
          preserveClockInfo(mClockInfo);
          break;

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

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET: {
          mSuppressBlink = true;

          TimeZone tz = mZoneManager.createForTimeZoneData(
              mChangingClockInfo.timeZoneData);
          TimeOffset offset = tz.getStdOffset();
          time_offset_mutation::increment15Minutes(offset);
          tz.setStdOffset(offset);
          mChangingClockInfo.timeZoneData = tz.toTimeZoneData();
          break;
        }

        case MODE_CHANGE_TIME_ZONE_DST: {
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
        case MODE_CHANGE_TIME_ZONE_NAME: {
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

        case MODE_CHANGE_INVERT_DISPLAY:
        {
          mSuppressBlink = true;
          incrementMod(mClockInfo.invertDisplay, (uint8_t) 3);
          invertDisplay();
          break;
        }

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

    void invertDisplay() {
      mPresenter.invertDisplay(mClockInfo.invertDisplay);

      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.print(F("invertDisplay(): "));
        SERIAL_PORT_MONITOR.println(mClockInfo.invertDisplay);
      }
    }

    void handleChangeButtonRepeatPress() {
      // Ignore 12/24 changes from RepeatPressed events.
      //  * It doesn't make sense to repeatedly change the 12/24 mode when the
      //    button is held down.
      //  * Each change of 12/24 mode causes a write to the EEPPROM, which
      //    causes wear and tear.
      if (mNavigator.mode() != MODE_DATE_TIME) {
        handleChangeButtonPress();
      }
    }

    void handleChangeButtonRelease() {
      switch (mNavigator.mode()) {
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
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case MODE_CHANGE_SETTINGS_BACKLIGHT:
        case MODE_CHANGE_SETTINGS_CONTRAST:
        case MODE_CHANGE_SETTINGS_BIAS:
      #else
        case MODE_CHANGE_SETTINGS_CONTRAST:
      #endif
        case MODE_CHANGE_INVERT_DISPLAY:
          mSuppressBlink = false;
          break;
      }
    }

  private:
    void updateDateTime() {
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(mClock.getNow(), tz);

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
            mChangingClockInfo.dateTime.second(mClockInfo.dateTime.second());
          }
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

      switch (mNavigator.mode()) {
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
          clockInfo = &mChangingClockInfo;
          break;

        // For all other modes render the normal mClockInfo instead. This
        // includes the "change settings" modes, because those are applied
        // directly to the current mClockInfo, not the mChangingClockInfo.
        default:
          clockInfo = &mClockInfo;
      }

      mPresenter.setRenderingInfo(
          mNavigator.mode(), mSuppressBlink, mBlinkShowState, *clockInfo
      );
    }

    /** Save the current UTC dateTime to the RTC. */
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
      clockInfo.timeZoneData = storedInfo.timeZoneData;
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        clockInfo.backlightLevel = storedInfo.backlightLevel;
        clockInfo.contrast = storedInfo.contrast;
        clockInfo.bias = storedInfo.bias;
      #else
        clockInfo.contrastLevel = storedInfo.contrastLevel;
      #endif
        clockInfo.invertDisplay = storedInfo.invertDisplay;
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
      #endif
        storedInfo.invertDisplay = clockInfo.invertDisplay;
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
    #endif

      mClockInfo.invertDisplay = ClockInfo::kInvertDisplayOff;
    }

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    static const uint16_t kBacklightValues[];
    static const uint8_t kContrastValues[];
  #else
    static const uint8_t kContrastValues[];
  #endif

  private:
    PersistentStore& mPersistentStore;
    Clock& mClock;
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
