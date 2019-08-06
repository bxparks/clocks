#ifndef MED_MINDER_CONTROLLER_H
#define MED_MINDER_CONTROLLER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <ace_time/hw/CrcEeprom.h>
#include "config.h"
#include "RenderingInfo.h"
#include "StoredInfo.h"
#include "Presenter.h"
#include "ClockInfo.h"

using namespace ace_time;
using namespace ace_time::clock;

/**
 * Class responsible for handling user button presses, and determining
 * how various member variables are updated. The rendering of the various
 * variables are handled by the Presenter. If analyzed using an MVC
 * architecture, this is the Controller, the Presenter is the View, and the
 * various member variables of this class are the Model objects.
 */
class Controller {
  public:
    static const uint16_t kStoredInfoEepromAddress = 0;

    /** Constructor. */
    Controller(Clock& Clock, hw::CrcEeprom& crcEeprom, Presenter& presenter):
        mClock(Clock),
        mCrcEeprom(crcEeprom),
        mPresenter(presenter),
        mZoneManager(kZoneRegistrySize, kZoneRegistry),
        mMode(MODE_UNKNOWN) {}

    void setup() {
      // Retrieve current time from Clock.
      acetime_t nowSeconds = mClock.getNow();

      // Restore state from EEPROM.
      StoredInfo storedInfo;
      bool isValid = mCrcEeprom.readWithCrc(kStoredInfoEepromAddress,
          &storedInfo, sizeof(StoredInfo));
      if (isValid) {
      #if ENABLE_SERIAL == 1
        SERIAL_PORT_MONITOR.println(F("setup(): valid StoredInfo"));
      #endif
        mClockInfo.timeZone = mZoneManager.createForTimeZoneData(
            storedInfo.timeZoneData);
        mMedInfo = storedInfo.medInfo;
      } else {
      #if ENABLE_SERIAL == 1
        SERIAL_PORT_MONITOR.println(
            F("setup(): invalid StoredInfo; initialiing"));
      #endif
        mClockInfo.timeZone = mZoneManager.createForZoneInfo(
            &zonedb::kZoneAmerica_Los_Angeles);
        mMedInfo.interval = TimePeriod(86400); // one day
        mMedInfo.startTime = nowSeconds;
      }

      // Set the current date time using the mClockInfo.timeZone.
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          nowSeconds, mClockInfo.timeZone);

      // Start with the medInfo view mode.
      mMode = MODE_VIEW_MED;
    }

    /** Wake from sleep. */
    void wakeup() {
      mIsPreparingToSleep = false;
      // TODO: might need to replace this with a special wakeup() method
      //mClock.setup();
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
          mMode = MODE_VIEW_DATE_TIME;
          break;
        case MODE_VIEW_DATE_TIME:
          mMode = MODE_VIEW_ABOUT;
          break;
        case MODE_VIEW_ABOUT:
          mMode = MODE_VIEW_MED;
          break;

        case MODE_CHANGE_MED_HOUR:
          mMode = MODE_CHANGE_MED_MINUTE;
          break;
        case MODE_CHANGE_MED_MINUTE:
          mMode = MODE_CHANGE_MED_HOUR;
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
          mMode = MODE_CHANGE_TIME_ZONE_NAME;
          break;
        case MODE_CHANGE_TIME_ZONE_NAME:
          mMode = MODE_CHANGE_YEAR;
          break;
      }
    }

    void modeButtonLongPress() {
      switch (mMode) {
        case MODE_VIEW_MED:
          mChangingMedInfo = mMedInfo;
          mMode = MODE_CHANGE_MED_HOUR;
          break;
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          saveMedInterval();
          mMode = MODE_CHANGE_YEAR;
          break;

        case MODE_VIEW_DATE_TIME:
          mChangingClockInfo = mClockInfo;
          mSecondFieldCleared = false;
          mMode = MODE_CHANGE_YEAR;
          break;
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_TIME_ZONE_NAME:
          saveDateTime();
          mMode = MODE_VIEW_DATE_TIME;
          break;
      }
    }

    void saveDateTime() {
      mClockInfo = mChangingClockInfo;
      mClock.setNow(mClockInfo.dateTime.toEpochSeconds());
      preserveInfo();
    }

    void saveMedInterval() {
      if (mMedInfo.interval != mChangingMedInfo.interval) {
        mMedInfo.interval = mChangingMedInfo.interval;
        mMedInfo.interval.second(0);
        preserveInfo();
      }
    }

    void saveTimeZone() {
      mClockInfo.timeZone = mChangingClockInfo.timeZone;
      mClockInfo.dateTime =
          mClockInfo.dateTime.convertToTimeZone(mClockInfo.timeZone);
      preserveInfo();
    }

    void changeButtonPress() {
      switch (mMode) {
        case MODE_CHANGE_MED_HOUR:
          mSuppressBlink = true;
          time_period_mutation::incrementHour(mChangingMedInfo.interval, 36);
          break;
        case MODE_CHANGE_MED_MINUTE:
          mSuppressBlink = true;
          time_period_mutation::incrementMinute(mChangingMedInfo.interval);
          break;

        case MODE_CHANGE_TIME_ZONE_NAME:
          mSuppressBlink = true;
          mZoneIndex++;
          if (mZoneIndex >= kZoneRegistrySize) {
            mZoneIndex = 0;
          }
          mChangingClockInfo.timeZone =
              mZoneManager.createForZoneIndex(mZoneIndex);
          mChangingClockInfo.dateTime =
              mChangingClockInfo.dateTime.convertToTimeZone(
                  mChangingClockInfo.timeZone);
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
        case MODE_CHANGE_TIME_ZONE_NAME:
          mSuppressBlink = false;
          break;
      }
    }

    void changeButtonLongPress() {
      switch (mMode) {
        case MODE_VIEW_MED:
          mMedInfo.startTime = mClockInfo.dateTime.toEpochSeconds();
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
      storedInfo.timeZoneData = mClockInfo.timeZone.toTimeZoneData();
      storedInfo.medInfo = mMedInfo;
      mCrcEeprom.writeWithCrc(kStoredInfoEepromAddress, &storedInfo,
          sizeof(StoredInfo));
    }

    void updateDateTime() {
      mClockInfo.dateTime = ZonedDateTime::forEpochSeconds(
          mClock.getNow(), mClockInfo.timeZone);

      // If in CHANGE mode, and the 'second' field has not been cleared,
      // update mChangingClockInfo.dateTime with the current second.
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
      }
    }

    /** Update the internal rendering info. */
    void updateRenderingInfo() {
      mPresenter.setMode(mMode);
      mPresenter.setSuppressBlink(mSuppressBlink);
      mPresenter.setBlinkShowState(mBlinkShowState);

      switch (mMode) {
        case MODE_VIEW_DATE_TIME:
          mPresenter.setDateTime(mClockInfo.dateTime);
          mPresenter.setTimeZone(mClockInfo.timeZone);
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
        case MODE_CHANGE_TIME_ZONE_NAME:
          mPresenter.setDateTime(mChangingClockInfo.dateTime);
          mPresenter.setTimeZone(mChangingClockInfo.timeZone);
          break;

        case MODE_VIEW_MED: {
          if (mClockInfo.dateTime.isError()) {
            mPresenter.setTimePeriod(TimePeriod(0));
          } else {
            int32_t now = mClockInfo.dateTime.toEpochSeconds();
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
    static const uint16_t kCacheSize = 2;
    static const basic::ZoneInfo* const kZoneRegistry[];
    static const uint16_t kZoneRegistrySize;

    Clock& mClock;
    hw::CrcEeprom& mCrcEeprom;
    Presenter& mPresenter;
    BasicZoneManager<kCacheSize> mZoneManager;;

    uint8_t mMode; // current mode
    ClockInfo mClockInfo; // current clock
    ClockInfo mChangingClockInfo; // target clock (in change mode)
    MedInfo mMedInfo; // current med info
    MedInfo mChangingMedInfo; // target med info (in change mode)

    uint16_t mZoneIndex;
    bool mSecondFieldCleared;
    bool mSuppressBlink; // true if blinking should be suppressed

    bool mBlinkShowState = true; // true means actually show
    uint16_t mBlinkCycleStartMillis = 0; // millis since blink cycle start
    bool mIsPreparingToSleep = false;
};

#endif
