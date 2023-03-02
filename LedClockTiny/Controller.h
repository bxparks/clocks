#ifndef LED_CLOCK_TINY_CONTROLLER_H
#define LED_CLOCK_TINY_CONTROLLER_H

#include <AceCommon.h> // incrementModOffset()
#include <AceTime.h>
#include <AceSegment.h>
#include "config.h"
#include "ClockInfo.h"
#include "Presenter.h"
#include "StoredInfo.h"
#include "PersistentStore.h"

using namespace ace_segment;
using namespace ace_time;
using ace_time::clock::Clock;
using ace_common::incrementModOffset;
using ace_common::incrementMod;

class Controller {
  public:
    /** Constructor. */
    Controller(
        Clock& clock,
        PersistentStore& persistentStore,
        Presenter& presenter
    ) :
        mClock(clock),
        mPersistentStore(persistentStore),
        mPresenter(presenter)
    {
      mClockInfo.mode = Mode::kViewHourMinute;
    }

    void setup() {
      // Restore from EEPROM to retrieve time zone.
      StoredInfo storedInfo;
      bool isValid = mPersistentStore.readStoredInfo(storedInfo);

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

    /** Should be called every 0.5 seconds to toggle the blinking state. */
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
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeHour;
          break;

        case Mode::kViewSecond:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeSecond;
          break;

        case Mode::kViewYear:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeYear;
          break;

        case Mode::kViewMonth:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeMonth;
          break;

        case Mode::kViewDay:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeDay;
          break;

        case Mode::kViewWeekday:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mClockInfo.mode = Mode::kChangeWeekday;
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

        case Mode::kChangeWeekday:
          saveDateTime();
          mClockInfo.mode = Mode::kViewWeekday;
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

    void changeButtonPress() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("changeButtonPress()"));
      }

      mClockInfo.suppressBlink = true;
      mChangingClockInfo.suppressBlink = true;

      switch (mClockInfo.mode) {
        case Mode::kChangeHour:
          offset_date_time_mutation::incrementHour(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMinute:
          offset_date_time_mutation::incrementMinute(
              mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeSecond:
          mSecondFieldCleared = true;
          mChangingClockInfo.dateTime.second(0);
          break;

        case Mode::kChangeYear:
          offset_date_time_mutation::incrementYear(mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeMonth:
          offset_date_time_mutation::incrementMonth(
              mChangingClockInfo.dateTime);
          break;

        case Mode::kChangeDay:
          offset_date_time_mutation::incrementDay(mChangingClockInfo.dateTime);
          break;

        /*
        case Mode::kChangeWeekday:
          incrementModOffset(
              mChangingClockInfo.dateTime.dayOfWeek(),
              (uint8_t) 7,
              (uint8_t) 1);
          break;
        */

        case Mode::kChangeBrightness:
          incrementMod(mClockInfo.brightness, (uint8_t) 8);
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
        case Mode::kChangeWeekday:
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
      int16_t totalOffset = TIME_STD_OFFSET_MINUTES;
      totalOffset += mClockInfo.isDst ? TIME_DST_OFFSET_MINUTES : 0;
      TimeOffset offset = TimeOffset::forMinutes(totalOffset);

      mClockInfo.dateTime = OffsetDateTime::forEpochSeconds(
          mClock.getNow(), offset);

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
        case Mode::kChangeHourMode:
        case Mode::kChangeWeekday:
          clockInfo = &mChangingClockInfo;
          break;

        default:
          clockInfo = &mClockInfo;
      }

      mPresenter.setRenderingInfo(*clockInfo);
    }

    /** Save the current UTC dateTime to the RTC. */
    void saveDateTime() {
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
    static void clockInfoFromStoredInfo(
        ClockInfo& clockInfo, const StoredInfo& storedInfo) {
      clockInfo.hourMode = storedInfo.hourMode;
      clockInfo.brightness = storedInfo.brightness;
    }

    /** Set up the initial ClockInfo states. */
    void setupClockInfo() {
      mClockInfo.hourMode = ClockInfo::kTwentyFour;
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
    }

  private:
    Clock& mClock;
    PersistentStore& mPersistentStore;
    Presenter& mPresenter;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    bool mSecondFieldCleared;
};

#endif
