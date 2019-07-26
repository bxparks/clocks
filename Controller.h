#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <ace_time/hw/CrcEeprom.h>
#include "config.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"

namespace med_minder {

using namespace ace_time;

/**
 * Class responsible for handling user button presses, and determining
 * how various member variables are updated. The rendering of the various
 * variables are handled by the Presenter. If analyzed using an MVC
 * architecture, this is the Controller, the Presenter is the View, and the
 * various member variables of this class are the Model objects.
 */
class Controller {
  public:
    static const uint16_t STORED_INFO_EEPROM_ADDRESS = 0;
    static const int8_t DEFAULT_TZ_CODE = -32; // Pacific Standard Time, -08:00

    /** Constructor. */
    Controller(TimeKeeper& timeKeeper, hw::CrcEeprom& crcEeprom,
            Presenter& presenter):
        mTimeKeeper(timeKeeper),
        mCrcEeprom(crcEeprom),
        mPresenter(presenter),
        mMode(MODE_UNKNOWN),
        mTimeZone(0) {}

    void setup() {
      // Retrieve current time from TimeKeeper.
      uint32_t nowSeconds = mTimeKeeper.getNow();

      // Restore state from EEPROM.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(STORED_INFO_EEPROM_ADDRESS,
          &storedInfo, sizeof(StoredInfo));
      if (isValid) {
        mTimeZone = storedInfo.timeZone;
        mMedInfo = storedInfo.medInfo;
      } else {
        mTimeZone = TimeZone(DEFAULT_TZ_CODE);
        mMedInfo.interval = TimePeriod(86400); // one day
        mMedInfo.startTime = nowSeconds;
      }

      // Set the current date time using the mTimeZone.
      mCurrentDateTime = DateTime(nowSeconds, mTimeZone);

      // Start with the medInfo view mode.
      mMode = MODE_VIEW_MED;
    }

    /** Wake from sleep. */
    void wakeup() {
      mIsPreparingToSleep = false;
      // TODO: might need to replace this with a special wakeup() method
      //mTimeKeeper.setup();
      setup();
    }

    /** Prepare to sleep. */
    void prepareToSleep() {
      mIsPreparingToSleep = true;
      mPresenter.clearDisplay();
    }

    void modeButtonPress()  {
      switch (mMode) {
        case MODE_VIEW_MED:
          mMode = MODE_DATE_TIME;
          break;

        case MODE_DATE_TIME:
          mMode = MODE_TIME_ZONE;
          break;
        case MODE_TIME_ZONE:
          mMode = MODE_VIEW_MED;
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

        case MODE_CHANGE_MED_HOUR:
          mMode = MODE_CHANGE_MED_MINUTE;
          break;
        case MODE_CHANGE_MED_MINUTE:
          mMode = MODE_CHANGE_MED_HOUR;
          break;

        case MODE_CHANGE_TIME_ZONE_HOUR:
          mMode = MODE_CHANGE_TIME_ZONE_MINUTE;
          break;

        case MODE_CHANGE_TIME_ZONE_MINUTE:
          mMode = MODE_CHANGE_TIME_ZONE_DST;
          break;

        case MODE_CHANGE_TIME_ZONE_DST:
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
          mChangingDateTime.timeZone(mTimeZone);
          mMode = MODE_CHANGE_TIME_ZONE_HOUR;
          break;

        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
          saveTimeZone();
          mMode = MODE_TIME_ZONE;
          break;

        case MODE_VIEW_MED:
          mChangingMedInfo = mMedInfo;
          mMode = MODE_CHANGE_MED_HOUR;
          break;

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          saveMedInterval();
          mMode = MODE_VIEW_MED;
          break;
      }
    }

    void saveDateTime() {
      mCurrentDateTime = mChangingDateTime;
      mTimeKeeper.setNow(mCurrentDateTime.toSecondsSinceEpoch());
    }

    void saveMedInterval() {
      if (mMedInfo.interval != mChangingMedInfo.interval) {
        mMedInfo.interval = mChangingMedInfo.interval;
        //mMedInfo.startTime = mCurrentDateTime.toSecondsSinceEpoch();
        mMedInfo.interval.second(0);
        preserveInfo(); // save mMedInfo in EEPROM
      }
    }

    void saveTimeZone() {
      mTimeZone = mChangingDateTime.timeZone();
      mCurrentDateTime = mCurrentDateTime.convertToTimeZone(mTimeZone);
      preserveInfo(); // save mTimeZone in EEPROM
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

        case MODE_CHANGE_MED_HOUR:
          mSuppressBlink = true;
          mChangingMedInfo.interval.incrementHour(36);
          break;
        case MODE_CHANGE_MED_MINUTE:
          mSuppressBlink = true;
          mChangingMedInfo.interval.incrementMinute();
          break;
      }

      // Update the display right away to prevent jitters during RepeatPress.
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
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
          mSuppressBlink = false;
          break;
      }
    }

    void changeButtonLongPress() {
      switch (mMode) {
        case MODE_VIEW_MED:
          mMedInfo.startTime = mCurrentDateTime.toSecondsSinceEpoch();
          preserveInfo();
          break;
      }
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

  protected:
    void preserveInfo() {
      StoredInfo storedInfo;
      storedInfo.timeZone = mTimeZone;
      storedInfo.medInfo = mMedInfo;
      mCrcEeprom.writeWithCrc(STORED_INFO_EEPROM_ADDRESS, &storedInfo,
          sizeof(StoredInfo));
    }

    void updateDateTime() {
      mCurrentDateTime = DateTime(mTimeKeeper.getNow(), mTimeZone);

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

    /** Update the internal rendering info. */
    void updateRenderingInfo() {
      mPresenter.setMode(mMode);
      mPresenter.setSuppressBlink(mSuppressBlink);
      mPresenter.setBlinkShowState(mBlinkShowState);

      switch (mMode) {
        case MODE_DATE_TIME:
        case MODE_TIME_ZONE:
          mPresenter.setDateTime(mCurrentDateTime);
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
          mPresenter.setDateTime(mChangingDateTime);
          break;

        case MODE_VIEW_MED: {
          if (mCurrentDateTime.isError()) {
            mPresenter.setTimePeriod(TimePeriod(0));
          } else {
            int32_t now = mCurrentDateTime.toSecondsSinceEpoch();
            int32_t remainingSeconds = mMedInfo.startTime
                + mMedInfo.interval.toSeconds() - now;
            TimePeriod remainingTime(remainingSeconds);
            mPresenter.setTimePeriod(remainingTime);
          }
          break;
        }

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          mPresenter.setTimePeriod(mChangingMedInfo.interval);
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

  protected:
    TimeKeeper& mTimeKeeper;
    hw::CrcEeprom& mCrcEeprom;
    Presenter& mPresenter;

    uint8_t mMode; // current mode
    TimeZone mTimeZone; // current time zone of clock
    DateTime mCurrentDateTime; // DateTime from the TimeKeeper
    DateTime mChangingDateTime; // DateTime set by user in "Change" modes
    MedInfo mMedInfo; // current med info
    MedInfo mChangingMedInfo; // med info in "Change" modes
    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
    bool mIsPreparingToSleep = false;
};

}

#endif
