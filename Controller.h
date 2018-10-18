#ifndef WORLD_CLOCK_CONTROLLER_H
#define WORLD_CLOCK_CONTROLLER_H

#include <AceTime.h>
#include <ace_time/hw/CrcEeprom.h>
#include <ace_time/common/DateStrings.h>
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"

using namespace ace_time;
using namespace ace_time::common;

/**
 * Maintains the internal state of the world clock, handling button inputs,
 * and calling out to the Presenter to display the clock. In an MVC
 * architecture, this would be the Controller. The Model would be the various
 * member variables in thic class. The View layer is the Presenter class.
 */
class Controller {
  public:
    static const uint16_t kStoredInfoEepromAddress = 0;
    static const int8_t kDefaultTzCode = -32; // Pacific Standard Time, -08:00

    /**
     * Constructor.
     * @param timeKeeper source of the current time
     * @param crcEeprom stores objects into the EEPROM with CRC
     * @param presenter renders the date and time info to the screen
     */
    Controller(TimeKeeper& timeKeeper, hw::CrcEeprom& crcEeprom,
            Presenter& presenter0, Presenter& presenter1,
            Presenter& presenter2):
        mTimeKeeper(timeKeeper),
        mCrcEeprom(crcEeprom),
        mPresenter0(presenter0),
        mPresenter1(presenter1),
        mPresenter2(presenter2),
        mMode(MODE_DATE_TIME) {}

    void setup() {
      // Restore from EEPROM to retrieve time zone.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
      if (!isValid) {
        strncpy(mClock0.name, "SFO - PDT", ClockInfo::kNameSize);
        mClock0.timeZone = TimeZone::forHour(-8).isDst(true);

        strncpy(mClock1.name, "PHL - EDT", ClockInfo::kNameSize);
        mClock1.timeZone = TimeZone::forHour(-5).isDst(true);

        strncpy(mClock2.name, "LHR - BST", ClockInfo::kNameSize);
        mClock2.timeZone = TimeZone::forHour(0).isDst(true);
      }

      // Retrieve current time from TimeKeeper.
      uint32_t nowSeconds = mTimeKeeper.getNow();

      // Set the current date time using the mTimeZone of clock0.
      mCurrentDateTime = DateTime(nowSeconds, mClock0.timeZone);
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

    void modeButtonPress() {
      switch (mMode) {
        case MODE_DATE_TIME:
          mMode = MODE_TIME_ZONE;
          break;
        case MODE_TIME_ZONE:
          mMode = MODE_DATE_TIME;
          break;

        case MODE_CHANGE_YEAR:
          mMode = MODE_CHANGE_MONTH;
          break;
        case MODE_CHANGE_MONTH:
          mMode = MODE_CHANGE_DAY;
          break;
        case MODE_CHANGE_DAY:
          mMode = MODE_CHANGE_HOUR;
          break;
        case MODE_CHANGE_HOUR:
          mMode = MODE_CHANGE_MINUTE;
          break;
        case MODE_CHANGE_MINUTE:
          mMode = MODE_CHANGE_SECOND;
          break;
        case MODE_CHANGE_SECOND:
          mMode = MODE_CHANGE_YEAR;
          break;

        case MODE_CHANGE_TIME_ZONE_HOUR:
          mMode = MODE_CHANGE_TIME_ZONE_MINUTE;
          break;
        case MODE_CHANGE_TIME_ZONE_MINUTE:
          mMode = MODE_CHANGE_TIME_ZONE_DST;
          break;
        case MODE_CHANGE_TIME_ZONE_DST:
          mMode = MODE_CHANGE_HOUR_MODE;
          break;
        case MODE_CHANGE_HOUR_MODE:
          mMode = MODE_CHANGE_TIME_ZONE_HOUR;
          break;
      }
    }

    void modeButtonLongPress() {
      switch (mMode) {
        case MODE_DATE_TIME:
          mChangingDateTime = mCurrentDateTime;
          mSecondFieldCleared = false;
          mMode = MODE_CHANGE_YEAR;
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          saveDateTime();
          mMode = MODE_DATE_TIME;
          break;

        case MODE_TIME_ZONE:
          mChangingDateTime.timeZone(mClock0.timeZone);
          mMode = MODE_CHANGE_TIME_ZONE_HOUR;
          break;

        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
        case MODE_CHANGE_HOUR_MODE:
          saveTimeZone();
          mMode = MODE_TIME_ZONE;
          break;
      }
    }

    void changeButtonPress() {
      switch (mMode) {
        case MODE_CHANGE_YEAR:
          mSuppressBlink = true;
          mChangingDateTime.incrementYear();
          break;
        case MODE_CHANGE_MONTH:
          mSuppressBlink = true;
          mChangingDateTime.incrementMonth();
          break;
        case MODE_CHANGE_DAY:
          mSuppressBlink = true;
          mChangingDateTime.incrementDay();
          break;
        case MODE_CHANGE_HOUR:
          mSuppressBlink = true;
          mChangingDateTime.incrementHour();
          break;
        case MODE_CHANGE_MINUTE:
          mSuppressBlink = true;
          mChangingDateTime.incrementMinute();
          break;
        case MODE_CHANGE_SECOND:
          mSuppressBlink = true;
          mChangingDateTime.second(0);
          mSecondFieldCleared = true;
          break;

        case MODE_CHANGE_TIME_ZONE_HOUR:
          mSuppressBlink = true;
          mChangingDateTime.timeZone().incrementHour();
          break;
        case MODE_CHANGE_TIME_ZONE_MINUTE:
          mSuppressBlink = true;
          mChangingDateTime.timeZone().increment15Minutes();
          break;
        case MODE_CHANGE_TIME_ZONE_DST:
          mSuppressBlink = true;
          mChangingDateTime.timeZone().isDst(
              !mChangingDateTime.timeZone().isDst());
          break;
        case MODE_CHANGE_HOUR_MODE:
          mSuppressBlink = true;
          mHourMode = 1 - mHourMode;
          break;
      }

      // Update the display right away to prevent jitters in the display when
      // the button is triggering RepeatPressed events.
      update();
    }

    void changeButtonRepeatPress() {
      changeButtonPress();
    }

    void changeButtonRelease() {
      switch (mMode) {
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
        case MODE_CHANGE_HOUR_MODE:
          mSuppressBlink = false;
          break;
      }
    }
  protected:
    void updateDateTime() {
      mCurrentDateTime = DateTime(mTimeKeeper.getNow());

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
            mChangingDateTime.second(mCurrentDateTime.second());
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
      updatePresenter(mPresenter0, mClock0);
      updatePresenter(mPresenter1, mClock1);
      updatePresenter(mPresenter2, mClock2);
    }

    void updatePresenter(Presenter& presenter, ClockInfo& clockInfo) {
      presenter.setMode(mMode);
      presenter.setSuppressBlink(mSuppressBlink);
      presenter.setBlinkShowState(mBlinkShowState);

      switch (mMode) {
        case MODE_DATE_TIME:
        case MODE_TIME_ZONE:
          presenter.setDateTime(
              mCurrentDateTime.convertToTimeZone(clockInfo.timeZone));
          presenter.setHourMode(clockInfo.hourMode);
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
        case MODE_CHANGE_HOUR_MODE:
          presenter.setDateTime(mChangingDateTime);
          presenter.setHourMode(clockInfo.hourMode);
          break;
      }
    }

    /** Save the current UTC DateTime to the RTC. */
    void saveDateTime() {
      mTimeKeeper.setNow(mChangingDateTime.toSecondsSinceEpoch());
    }

    void saveTimeZone() {
      mClock0.timeZone = mChangingDateTime.timeZone();
      mCurrentDateTime = mCurrentDateTime.convertToTimeZone(mClock0.timeZone);
      preserveInfo(); // save mTimeZone
    }

    /** Read the UTC DateTime from RTC and convert to current time zone. */
    void readDateTime(DateTime* dateTime) {
      uint32_t now = mTimeKeeper.getNow();
      *dateTime = DateTime(now, mClock0.timeZone);
    }

    void preserveInfo() {
      StoredInfo storedInfo;
      storedInfo.clock0 = mClock0;
      storedInfo.clock1 = mClock1;
      storedInfo.clock2 = mClock2;

      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, &storedInfo,
          sizeof(StoredInfo));
    }

  private:
    // Disable copy-constructor and assignment operator
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    TimeKeeper& mTimeKeeper;
    hw::CrcEeprom& mCrcEeprom;
    Presenter& mPresenter0;
    Presenter& mPresenter1;
    Presenter& mPresenter2;
    ClockInfo mClock0;
    ClockInfo mClock1;
    ClockInfo mClock2;

    uint8_t mMode; // current mode
    DateTime mCurrentDateTime; // DateTime from the TimeKeeper
    DateTime mChangingDateTime; // DateTime set by user in "Change" modes
    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed
    uint8_t mHourMode = 0; // 12/24 mode

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
};

#endif
