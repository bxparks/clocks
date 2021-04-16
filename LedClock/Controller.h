#ifndef LED_CLOCK_CONTROLLER_H
#define LED_CLOCK_CONTROLLER_H

#include <AceCommon.h> // incrementModOffset()
#include <AceTime.h>
#include <AceSegment.h>
#include <AceUtilsCrcEeprom.h>
#include "config.h"
#include "ClockInfo.h"
#include "Presenter.h"
#include "StoredInfo.h"

using namespace ace_segment;
using namespace ace_time;
using ace_time::clock::Clock;
using ace_utils::crc_eeprom::CrcEeprom;
using ace_common::incrementModOffset;

class Controller {
  public:
    static const uint16_t kStoredInfoEepromAddress = 0;

    static const int16_t kDefaultOffsetMinutes = -8*60; // UTC-08:00

    /** Constructor. */
    Controller(
        Clock& clock,
        CrcEeprom& crcEeprom,
        Presenter& presenter,
        ZoneManager& zoneManager,
        TimeZoneData initialTimeZoneData
    ) :
        mClock(clock),
        mCrcEeprom(crcEeprom),
        mPresenter(presenter),
        mZoneManager(zoneManager),
        mInitialTimeZoneData(initialTimeZoneData)
    {
      mMode = Mode::kViewHourMinute;
    }

    void setup() {
      // Restore from EEPROM to retrieve time zone.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(
          kStoredInfoEepromAddress, storedInfo);

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
        case Mode::kViewHourMinute:
          mMode = Mode::kViewMinuteSecond;
          break;
        case Mode::kViewMinuteSecond:
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
          mMode = Mode::kViewHourMinute;
          break;

        case Mode::kChangeHour:
          mMode = Mode::kChangeMinute;
          break;
        case Mode::kChangeMinute:
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
          mSecondFieldCleared = false;
          mMode = Mode::kChangeHour;
          break;

        case Mode::kViewMinuteSecond:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mMode = Mode::kChangeMinute;
          break;

        case Mode::kViewYear:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mMode = Mode::kChangeYear;
          break;

        case Mode::kViewMonth:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mMode = Mode::kChangeMonth;
          break;

        case Mode::kViewDay:
          mChangingClockInfo = mClockInfo;
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

        case Mode::kChangeBrightness:
          preserveClockInfo(mClockInfo);
          mMode = Mode::kViewBrightness;
          break;

        default:
          break;
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
          incrementModOffset(mClockInfo.brightness, (uint8_t) 7, (uint8_t) 1);
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
        case Mode::kChangeTimeZoneOffset:
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
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
        case Mode::kChangeHourMode:
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
      mClock.setNow(mChangingClockInfo.dateTime.toEpochSeconds());
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
      clockInfo.timeZoneData = storedInfo.timeZoneData;
    }

    /** Set up the initial ClockInfo states. */
    void setupClockInfo() {
      mClockInfo.hourMode = ClockInfo::kTwentyFour;
      mClockInfo.timeZoneData = mInitialTimeZoneData;
    }

    /** Save the clock info into EEPROM. */
    void preserveClockInfo(const ClockInfo& clockInfo) {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("preserveClockInfo()"));
      }
      StoredInfo storedInfo;
      storedInfoFromClockInfo(storedInfo, clockInfo);
      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, storedInfo);
    }

    /** Convert ClockInfo to StoredInfo. */
    static void storedInfoFromClockInfo(
        StoredInfo& storedInfo, const ClockInfo& clockInfo) {
      storedInfo.hourMode = clockInfo.hourMode;
      storedInfo.brightness = clockInfo.brightness;
      storedInfo.timeZoneData = clockInfo.timeZoneData;
    }

  private:
    Clock& mClock;
    CrcEeprom& mCrcEeprom;
    Presenter& mPresenter;
    ZoneManager& mZoneManager;
    TimeZoneData mInitialTimeZoneData;

    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // the target clock

    Mode mMode = Mode::kUnknown; // current mode

    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
