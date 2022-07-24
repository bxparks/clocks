#ifndef CHRISTMAS_CLOCK_CONTROLLER_H
#define CHRISTMAS_CLOCK_CONTROLLER_H

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
      mMode = Mode::kViewCountdown;
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
      if (mMode == Mode::kUnknown) return;
      updateDateTime();
      updateBlinkState();
      updateRenderingInfo();
      mPresenter.display();
    }

    void modeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("modeButtonPress()"));
      }

      switch ((Mode) mMode) {
        case Mode::kViewCountdown:
          mMode = Mode::kViewHourMinute;
          break;
        case Mode::kViewHourMinute:
          mMode = Mode::kViewSecond;
          break;
        case Mode::kViewSecond:
          mMode = Mode::kViewYear;
          break;
        case Mode::kViewYear:
          mMode = Mode::kViewMonth;
          break;
        case Mode::kViewMonth:
          mMode = Mode::kViewDay;
          break;
        case Mode::kViewDay:
          mMode = Mode::kViewWeekday;
          break;
        case Mode::kViewWeekday:
          mMode = Mode::kViewBrightness;
          break;
        case Mode::kViewBrightness:
          mMode = Mode::kViewCountdown;
          break;

        case Mode::kChangeHour:
          mMode = Mode::kChangeMinute;
          break;
        case Mode::kChangeMinute:
          mMode = Mode::kChangeSecond;
          break;
        case Mode::kChangeSecond:
          mMode = Mode::kChangeYear;
          break;
        case Mode::kChangeYear:
          mMode = Mode::kChangeMonth;
          break;
        case Mode::kChangeMonth:
          mMode = Mode::kChangeDay;
          break;
        case Mode::kChangeDay:
          mMode = Mode::kChangeHour;
          break;

        default:
          break;
      }
    }

    void modeButtonLongPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("modeButtonLongPress()"));
      }

      switch ((Mode) mMode) {
        case Mode::kViewHourMinute:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mMode = Mode::kChangeHour;
          break;

        case Mode::kViewSecond:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mMode = Mode::kChangeSecond;
          break;

        case Mode::kViewYear:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mMode = Mode::kChangeYear;
          break;

        case Mode::kViewMonth:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mMode = Mode::kChangeMonth;
          break;

        case Mode::kViewDay:
          mChangingClockInfo = mClockInfo;
          initChangingClock();
          mSecondFieldCleared = false;
          mMode = Mode::kChangeDay;
          break;

        case Mode::kViewBrightness:
          mMode = Mode::kChangeBrightness;
          break;

        case Mode::kChangeYear:
          saveDateTime();
          mMode = Mode::kViewYear;
          break;

        case Mode::kChangeMonth:
          saveDateTime();
          mMode = Mode::kViewMonth;
          break;

        case Mode::kChangeDay:
          saveDateTime();
          mMode = Mode::kViewDay;
          break;

        case Mode::kChangeHour:
          saveDateTime();
          mMode = Mode::kViewHourMinute;
          break;

        case Mode::kChangeMinute:
          saveDateTime();
          mMode = Mode::kViewHourMinute;
          break;

        case Mode::kChangeSecond:
          saveDateTime();
          mMode = Mode::kViewSecond;
          break;

        case Mode::kChangeBrightness:
          preserveClockInfo(mClockInfo);
          mMode = Mode::kViewBrightness;
          break;

        default:
          break;
      }
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

      switch ((Mode) mMode) {
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
          mSecondFieldCleared = true;
          mChangingClockInfo.dateTime.second(0);
          break;

        case Mode::kChangeYear:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementYear(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMonth:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementMonth(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeDay:
          mSuppressBlink = true;
          zoned_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeBrightness:
          mSuppressBlink = true;
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

      switch ((Mode) mMode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
        case Mode::kChangeBrightness:
          mSuppressBlink = false;
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
      switch ((Mode) mMode) {
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

      switch ((Mode) mMode) {
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          clockInfo = &mChangingClockInfo;
          break;

        default:
          clockInfo = &mClockInfo;
      }

      mPresenter.setRenderingInfo(
          mMode, mSuppressBlink || mBlinkShowState, *clockInfo);
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
  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif
    TimeZoneData mInitialTimeZoneData;
    uint8_t const mBrightnessLevels;
    uint8_t const mBrightnessMin;
    uint8_t const mBrightnessMax;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    Mode mMode = Mode::kUnknown; // current mode

    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
