#ifndef LED_CLOCK_CONTROLLER_H
#define LED_CLOCK_CONTROLLER_H

#include <AceCommon.h> // incrementModOffset()
#include <AceTime.h>
#include <AceSegment.h>
#include "config.h"
#include "ClockInfo.h"
#include "Presenter.h"
#include "StoredInfo.h"

using namespace ace_segment;
using namespace ace_time;
using ace_time::clock::Clock;
using ace_common::incrementModOffset;

class Controller {
  public:
    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    /** Constructor. */
    Controller(
        Clock& clock,
        PersistentStore& persistentStore,
        Presenter& presenter,
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        ManualZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        BasicZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        ExtendedZoneManager& zoneManager,
      #endif
        TimeZoneData initialTimeZoneData,
        uint8_t brightnessLevels,
        uint8_t brightnessMin,
        uint8_t brightnessMax
    ) :
        mClock(clock),
        mPersistentStore(persistentStore),
        mPresenter(presenter),
        mZoneManager(zoneManager),
        mInitialTimeZoneData(initialTimeZoneData),
        mBrightnessLevels(brightnessLevels),
        mBrightnessMin(brightnessMin),
        mBrightnessMax(brightnessMax)
    {
      mClockInfo.mode = Mode::kViewHourMinute;
    }

    void setup() {
      // Restore from EEPROM to retrieve time zone.
      StoredInfo storedInfo;
      bool isValid = mPersistentStore.readStoredInfo(storedInfo);
    #if ENABLE_SERIAL_DEBUG >= 1
      if (isValid) {
        Serial.println(F("Controller.setup(): persistentStore valid"));
      } else {
        Serial.println(F("Controller.setup(): persistentStore NOT valid"));
      }
    #endif

      if (isValid) {
        clockInfoFromStoredInfo(mClockInfo, storedInfo);
      } else {
        setupClockInfo();
        preserveClockInfo(mClockInfo);
      }

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

    void modeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("modeButtonPress()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kViewHourMinute:
          mClockInfo.mode = Mode::kViewSecond;
          break;
        case Mode::kViewSecond:
          mClockInfo.mode = Mode::kViewYear;
          break;
        case Mode::kViewYear:
          mClockInfo.mode = Mode::kViewMonth;
          break;
        case Mode::kViewMonth:
          mClockInfo.mode = Mode::kViewDay;
          break;
        case Mode::kViewDay:
          mClockInfo.mode = Mode::kViewWeekday;
          break;
        case Mode::kViewWeekday:
          mClockInfo.mode = Mode::kViewTimeZone;
          break;
        case Mode::kViewTimeZone:
          mClockInfo.mode = Mode::kViewBrightness;
          break;
        case Mode::kViewBrightness:
          mClockInfo.mode = Mode::kViewHourMinute;
          break;

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

        default:
          break;
      }

      mChangingClockInfo.mode = mClockInfo.mode;
    }

    void modeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("modeButtonLongPress()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kViewHourMinute:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeHour;
          break;

        case Mode::kViewSecond:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeSecond;
          break;

        case Mode::kViewYear:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeYear;
          break;

        case Mode::kViewMonth:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeMonth;
          break;

        case Mode::kViewDay:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeDay;
          break;

        case Mode::kViewTimeZone:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mZoneRegistryIndex = mZoneManager.indexForZoneId(
              mChangingClockInfo.timeZoneData.zoneId);
          mClockInfo.mode = Mode::kChangeTimeZone;
          break;

        case Mode::kViewBrightness:
          mClockInfo.mode = Mode::kChangeBrightness;
          break;

        case Mode::kChangeYear:
          saveDateTime();
          mClockInfo.mode = Mode::kViewYear;
          break;

        case Mode::kChangeMonth:
          saveDateTime();
          mClockInfo.mode = Mode::kViewMonth;
          break;

        case Mode::kChangeDay:
          saveDateTime();
          mClockInfo.mode = Mode::kViewDay;
          break;

        case Mode::kChangeHour:
          saveDateTime();
          mClockInfo.mode = Mode::kViewHourMinute;
          break;

        case Mode::kChangeMinute:
          saveDateTime();
          mClockInfo.mode = Mode::kViewHourMinute;
          break;

        case Mode::kChangeSecond:
          saveDateTime();
          mClockInfo.mode = Mode::kViewSecond;
          break;

        case Mode::kChangeTimeZone:
          saveClockInfo();
          mClockInfo.mode = Mode::kViewTimeZone;
          break;

        case Mode::kChangeBrightness:
          preserveClockInfo(mClockInfo);
          mClockInfo.mode = Mode::kViewBrightness;
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

    void changeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("changeButtonPress()"));
      }

      mClockInfo.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch (mClockInfo.mode) {
        case Mode::kChangeHour:
          zoned_date_time_mutation::incrementHour(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMinute:
          zoned_date_time_mutation::incrementMinute(
              mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeSecond:
          mSecondFieldCleared = true;
          mChangingClockInfo.dateTime.second(0);
          break;

        case Mode::kChangeYear:
          zoned_date_time_mutation::incrementYear(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMonth:
          zoned_date_time_mutation::incrementMonth(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeDay:
          zoned_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeTimeZone: {
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

        case Mode::kChangeBrightness:
          incrementModOffset(
              mClockInfo.brightness,
              mBrightnessLevels,
              mBrightnessMin);
          mClockInfo.brightness = normalizeBrightness(mClockInfo.brightness);
          break;

        default:
          break;
      }

      // Update the display right away to prevent jitters in the display when
      // the button is triggering RepeatPressed events.
      update();
    }

    void changeButtonRepeatPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("changeButtonRepeatPress()"));
      }

      changeButtonPress();
    }

    void changeButtonRelease() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("changeButtonRelease()"));
      }

      switch (mClockInfo.mode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
        case Mode::kChangeTimeZone:
        case Mode::kChangeBrightness:
          mClockInfo.suppressBlink = false;
          mChangingClockInfo.suppressBlink = false;
          break;

        default:
          break;
      }
    }

  private:
    void updateDateTime() {
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(mClock.getNow(), tz);

      // If in CHANGE mode, and the 'second' field has not been cleared, update
      // the displayed time with the current second.
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
        case Mode::kChangeTimeZone:
          clockInfo = &mChangingClockInfo;
          break;

        default:
          clockInfo = &mClockInfo;
      }

      mPresenter.setClockInfo(*clockInfo);
    }

    /** Save the current UTC dateTime to the RTC. */
    void saveDateTime() {
      mChangingClockInfo.dateTime.normalize();
      acetime_t epochSeconds = mChangingClockInfo.dateTime.toEpochSeconds();
      if (ENABLE_SERIAL_DEBUG >= 1) {
        Serial.print(F("saveDateTime(): epochSeconds:"));
        Serial.println(epochSeconds);
        mChangingClockInfo.dateTime.printTo(Serial);
        Serial.println();
      }

      mClock.setNow(epochSeconds);
    }

    /** Save the time zone from Changing to current. */
    void saveClockInfo() {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        Serial.println(F("saveClockInfo():"));
      }
      mClockInfo = mChangingClockInfo;
      preserveClockInfo(mClockInfo);
    }

    /** Convert StoredInfo to ClockInfo. */
    void clockInfoFromStoredInfo(
        ClockInfo& clockInfo, const StoredInfo& storedInfo) {
      clockInfo.hourMode = storedInfo.hourMode;
    #if ENABLE_SERIAL_DEBUG >= 1
      Serial.print(F("clockInfoFromStoredInfo(): storedInfo.brightness:"));
      Serial.println(storedInfo.brightness);
    #endif
      clockInfo.brightness = normalizeBrightness(storedInfo.brightness);
    #if ENABLE_SERIAL_DEBUG >= 1
      Serial.print(F("clockInfoFromStoredInfo(): clockInfo.brightness:"));
      Serial.println(clockInfo.brightness);
    #endif
      clockInfo.timeZoneData = storedInfo.timeZoneData;
    }

    /** Set up the initial ClockInfo states. */
    void setupClockInfo() {
      mClockInfo.hourMode = ClockInfo::kTwentyFour;
      mClockInfo.timeZoneData = mInitialTimeZoneData;
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

    /** Convert ClockInfo to StoredInfo. */
    static void storedInfoFromClockInfo(
        StoredInfo& storedInfo, const ClockInfo& clockInfo) {
      storedInfo.hourMode = clockInfo.hourMode;
      storedInfo.brightness = clockInfo.brightness;
      storedInfo.timeZoneData = clockInfo.timeZoneData;
    }

    /** Normalize brightness, staying within bounds of the LED module. */
    uint8_t normalizeBrightness(uint8_t brightness) {
      if (brightness < mBrightnessMin) {
        brightness = mBrightnessMin;
      } else if (brightness > mBrightnessMax) {
        brightness = mBrightnessMax;
      }
      return brightness;
    }

  private:
    Clock& mClock;
    PersistentStore& mPersistentStore;
    Presenter& mPresenter;

    // TimeZone
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif
    TimeZoneData mInitialTimeZoneData;
    uint16_t mZoneRegistryIndex;

    // LED brightness
    uint8_t const mBrightnessLevels;
    uint8_t const mBrightnessMin;
    uint8_t const mBrightnessMax;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    bool mSecondFieldCleared;
};

#endif
