#ifndef ONE_ZONE_CLOCK_CONTROLLER_H
#define ONE_ZONE_CLOCK_CONTROLLER_H

#include "config.h"
#if ENABLE_DHT22
  #include <DHT.h>
#else
  class DHT;
#endif
#include <AceCommon.h> // incrementMod()
#include <AceTime.h>
#include <AceUtils.h>
#include "ClockInfo.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "PersistentStore.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::clock;
using ace_common::incrementMod;

/**
 * Class responsible for updating the ClockInfo with the latest information:
 *
 *    * current date and time,
 *    * current timezone,
 *    * handling user-selected options,
 *    * temperature and humidity.
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
     * @param dht pointer to DHT22 object if enabled
     */
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
      #if ENABLE_DHT22
        , DHT* dht
      #endif
    ) :
        mClock(clock)
        , mPersistentStore(persistentStore)
        , mPresenter(presenter)
        , mZoneManager(zoneManager),
        mInitialTimeZoneData(initialTimeZoneData)
      #if ENABLE_DHT22
        , mDht(dht)
      #endif
    {
      mClockInfo.mode = Mode::kViewDateTime;
    }

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
      if (mClockInfo.mode == Mode::kUnknown) return;
      updateDateTime();
      updatePresenter();
      mPresenter.updateDisplay();
    }

    void updateBlinkState () {
      mClockInfo.blinkShowState = !mClockInfo.blinkShowState;
      mChangingClockInfo.blinkShowState = !mChangingClockInfo.blinkShowState;
      updatePresenter();
    }

    /**
     * Go to the next Mode, either the next screen or the next editable field.
     */
    void handleModeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonPress()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kViewDateTime:
          mClockInfo.mode = Mode::kViewTimeZone;
          break;
        case Mode::kViewTimeZone:
      #if ENABLE_DHT22
          mClockInfo.mode = Mode::kViewTemperature;
          break;
        case Mode::kViewTemperature:
      #endif
          mClockInfo.mode = Mode::kViewSettings;
          break;
        case Mode::kViewSettings:
          mClockInfo.mode = Mode::kViewSysclock;
          break;
        case Mode::kViewSysclock:
          mClockInfo.mode = Mode::kViewAbout;
          break;
        case Mode::kViewAbout:
          mClockInfo.mode = Mode::kViewDateTime;
          break;

        // Date/Time
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

        // TimeZone
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

        // Display settings
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
          mClockInfo.mode = Mode::kChangeSettingsContrast;
          break;
        case Mode::kChangeSettingsContrast:
          mClockInfo.mode = Mode::kChangeSettingsBias;
          break;
        case Mode::kChangeSettingsBias:
          mClockInfo.mode = Mode::kChangeSettingsBacklight;
          break;
      #else
        case Mode::kChangeSettingsContrast:
          mClockInfo.mode = Mode::kChangeInvertDisplay;
          break;
        case Mode::kChangeInvertDisplay:
          mClockInfo.mode = Mode::kChangeSettingsContrast;
          break;
      #endif
      // TODO: I think this needs to be merged into the above somehow.
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsLedOnOff:
          mClockInfo.mode = Mode::kChangeSettingsLedBrightness;
          break;
        case Mode::kChangeSettingsLedBrightness:
          mClockInfo.mode = Mode::kChangeSettingsLedOnOff;
          break;
      #endif

        default:
          break;
      }

      #if TIME_ZONE_TYPE != TIME_ZONE_TYPE_MANUAL
      switch (mClockInfo.mode) {
        case Mode::kChangeTimeZoneName:
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZoneData.zoneId);
          break;

        default:
          break;
      }
      #endif
    }

    /** Toggle edit mode. The editable field will start blinking. */
    void handleModeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonLongPress()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kViewDateTime:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeYear;
          break;

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

        case Mode::kViewSettings:
        #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
          mClockInfo.mode = Mode::kChangeSettingsBacklight;
        #else
          mClockInfo.mode = Mode::kChangeSettingsContrast;
        #endif
          break;

        // Long Press in Change mode, so save the changed info.
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          saveDateTime();
          mClockInfo.mode = Mode::kViewDateTime;
          break;

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
          saveChangingClockInfo();
          mClockInfo.mode = Mode::kViewTimeZone;
          break;

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
      #if ENABLE_LED_DISPLAY
        case Mode::kChangeSettingsLedOnOff:
        case Mode::kChangeSettingsLedBrightness:
      #endif
          saveClockInfo();
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

    /**
     * Exit the edit mode while throwing away all changes. Does nothing if not
     * already in edit mode.
     */
    void handleModeButtonDoubleClick() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleModeButtonDoubleClick()"));
      }

      switch (mClockInfo.mode) {
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

      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
      #if ENABLE_LED_DISPLAY
        case Mode::kChangeSettingsLedOnOff:
        case Mode::kChangeSettingsLedBrightness:
      #endif
          mClockInfo.mode = Mode::kViewSettings;
          break;

        default:
          break;
      }
    }

    void handleChangeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("handleChangeButtonPress()"));
      }
      mClockInfo.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch (mClockInfo.mode) {
        // Switch 12/24 modes if in Mode::kDataTime
        case Mode::kViewDateTime:
          mClockInfo.hourMode ^= 0x1;
          saveClockInfo();
          break;

      #if ENABLE_DHT22
        // Clicking 'Change' button in Temperature mode forces another update.
        case Mode::kViewTemperature:
          updateTemperature();
          break;
      #endif

        case Mode::kChangeYear:
          // Keep year between [2000,2100).
          zoned_date_time_mutation::incrementYear(
              mChangingClockInfo.dateTime);
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
        // Cycle through the zones in the registry
        case Mode::kChangeTimeZoneName: {
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
          incrementMod(mClockInfo.backlightLevel, (uint8_t) 10);
          break;
        }
        case Mode::kChangeSettingsContrast: {
          incrementMod(mClockInfo.contrast, (uint8_t) 128);
          break;
        }
        case Mode::kChangeSettingsBias: {
          incrementMod(mClockInfo.bias, (uint8_t) 8);
          break;
        }
      #else
        case Mode::kChangeSettingsContrast: {
          incrementMod(mClockInfo.contrastLevel, (uint8_t) 10);
          break;
        }
        case Mode::kChangeInvertDisplay: {
          incrementMod(mClockInfo.invertDisplay, (uint8_t) 5);
          break;
        }
      #endif

      #if ENABLE_LED_DISPLAY
        case Mode::kChangeSettingsLedOnOff: {
          mClockInfo.ledOnOff = ! mClockInfo.ledOnOff;
          break;
        }
        case Mode::kChangeSettingsLedBrightness: {
          mSuppressBlink = true;
          incrementMod(mClockInfo.ledBrightness, (uint8_t) 8);
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
      if (mClockInfo.mode != Mode::kViewDateTime) {
        handleChangeButtonPress();
      }
    }

    void handleChangeButtonRelease() {
      switch (mClockInfo.mode) {
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
      #if ENABLE_LED_DISPLAY
        case Mode::kChangeSettingsLedOnOff:
        case Mode::kChangeSettingsLedBrightness:
      #endif
          mClockInfo.suppressBlink = false;
          mChangingClockInfo.suppressBlink = false;
          break;

        default:
          break;
      }
    }

  #if ENABLE_DHT22
    void updateTemperature() {
      mClockInfo.temperatureC = mDht->readTemperature();
      mClockInfo.humidity = mDht->readHumidity();

    #if ENABLE_SERIAL_DEBUG >= 2
      SERIAL_PORT_MONITOR.print(F("updateTemperature(): "));
      SERIAL_PORT_MONITOR.print(mClockInfo.temperatureC);
      SERIAL_PORT_MONITOR.print(" C, ");
      SERIAL_PORT_MONITOR.print(mClockInfo.humidity);
      SERIAL_PORT_MONITOR.println(" %");
    #endif
    }
  #endif

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

    void updatePresenter() {
      ClockInfo* clockInfo;

      switch (mClockInfo.mode) {
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

      mPresenter.setRenderingInfo(*clockInfo);
    }

    /** Save the current UTC dateTime to the RTC. */
    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
    }

    /** Transfer info from ChangingClockInfo to ClockInfo. */
    void saveChangingClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("saveChangingClockInfo()"));
      }
      mClockInfo = mChangingClockInfo;
      saveClockInfo();
    }

    /** Save the clock info into EEPROM. */
    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("saveClockInfo()"));
      }
      StoredInfo storedInfo;
      storedInfoFromClockInfo(storedInfo, mClockInfo);
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
      #if ENABLE_LED_DISPLAY
        clockInfo.ledOnOff = storedInfo.ledOnOff;
        clockInfo.ledBrightness = storedInfo.ledBrightness;
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
      #if ENABLE_LED_DISPLAY
        storedInfo.ledOnOff = clockInfo.ledOnOff;
        storedInfo.ledBrightness = clockInfo.ledBrightness;
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
        saveClockInfo();
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
      mClockInfo.contrastLevel = OLED_INITIAL_CONTRAST;
      mClockInfo.invertDisplay = ClockInfo::kInvertDisplayOff;
    #endif
    #if ENABLE_LED_DISPLAY
      mClockInfo.ledOnOff = true;
      mClockInfo.ledBrightness = 1;
    #endif
    }

  private:
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
  #if ENABLE_DHT22
    DHT* const mDht;
  #endif

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    uint16_t mZoneRegistryIndex;
    bool mSecondFieldCleared;
};

#endif
