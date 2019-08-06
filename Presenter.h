#ifndef MED_MINDER_PRESENTER_H
#define MED_MINDER_PRESENTER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <ace_time/common/DateStrings.h>
#include "RenderingInfo.h"
#include "ClockInfo.h"

using namespace ace_time;
using namespace ace_time::common;

/** Class responsible for rendering the information to the OLED display. */
class Presenter {
  public:
    Presenter(SSD1306Ascii& oled):
        mOled(oled) {}

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void display() {
      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        displayData();
      }

      mPrevRenderingInfo = mRenderingInfo;
    }

    /** Clear the display. */
    void clearDisplay() { mOled.clear(); }

    void setMode(uint8_t mode) { mRenderingInfo.mode = mode; }

    void setDateTime(const ZonedDateTime& dateTime) {
      mRenderingInfo.dateTime = dateTime;
    }

    void setTimeZone(const TimeZone& timeZone) {
      mRenderingInfo.timeZone = timeZone;
    }

    void setTimePeriod(const TimePeriod& timePeriod) {
      mRenderingInfo.timePeriod = timePeriod;
    }

    void setSuppressBlink(bool suppressBlink) {
      mRenderingInfo.suppressBlink = suppressBlink;
    }

    void setBlinkShowState(bool blinkShowState) {
      mRenderingInfo.blinkShowState = blinkShowState;
    }

  private:
    void displayData() const {
      mOled.home();

      switch (mRenderingInfo.mode) {
        case MODE_VIEW_MED:
          displayViewMed();
          break;

        case MODE_VIEW_DATE_TIME:
          displayViewDateTime();
          break;

        case MODE_VIEW_ABOUT:
          displayViewAbout();
          break;

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          displayChangeMed();
          break;

        case MODE_CHANGE_TIME_ZONE_NAME:
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          displayViewDateTime();
          break;
      }
    }

    void displayViewMed() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayViewMed()"));
    #endif
      mOled.setFont(fixed_bold10x15);
      mOled.set1X();

      mOled.println(F("Med due"));
      mRenderingInfo.timePeriod.printTo(mOled);
      mOled.clearToEOL();
    }

    void displayViewAbout() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayViewAbout()"));
    #endif
      mOled.setFont(SystemFont5x7);
      mOled.set1X();

      mOled.print(F("MedMinder: "));
      mOled.print(MED_MINDER_VERSION_STRING);
      mOled.println();
      mOled.print(F("TZ: "));
      mOled.print(zonedb::kTzDatabaseVersion);
      mOled.println();
      mOled.print(F("AceTime: "));
      mOled.print(ACE_TIME_VERSION_STRING);
      mOled.println();
      mOled.print(F("Clock: "));
    #if TIME_PROVIDER == TIME_PROVIDER_DS3231
      mOled.print(F("DS3231"));
    #elif TIME_PROVIDER == TIME_PROVIDER_NTP
      mOled.print(F("NTP"));
    #elif TIME_PROVIDER == TIME_PROVIDER_SYSTEM
      mOled.print(F("millis()"));
    #else
      mOled.print(F("<Unknown>"));
    #endif
    }

    void displayChangeMed() const {
      mOled.println("Med intrvl");

      if (shouldShowFor(MODE_CHANGE_MED_HOUR)) {
        printPad2(mOled, mRenderingInfo.timePeriod.hour());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_MED_MINUTE)) {
        printPad2(mOled, mRenderingInfo.timePeriod.minute());
      } else {
        mOled.print("  ");
      }

      mOled.clearToEOL();
    }

    void displayViewDateTime() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayViewDateTime()"));
    #endif
      mOled.setFont(fixed_bold10x15);
      mOled.set1X();

      displayDate();
      mOled.println();
      displayTime();
      mOled.println();
      displayTimeZone();
    }

    void displayDate() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

      if (dateTime.isError()) {
        mOled.print(F("<INVALID>"));
        return;
      }
      if (shouldShowFor(MODE_CHANGE_YEAR)) {
        mOled.print(dateTime.year());
      } else {
        mOled.print("    ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_MONTH)) {
        printPad2(mOled, dateTime.month());
      } else {
        mOled.print("  ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_DAY)) {
        printPad2(mOled, dateTime.day());
      } else{
        mOled.print("  ");
      }
      mOled.clearToEOL();
    }

    void displayTime() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

      if (shouldShowFor(MODE_CHANGE_HOUR)) {
        printPad2(mOled, dateTime.hour());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_MINUTE)) {
        printPad2(mOled, dateTime.minute());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_SECOND)) {
        printPad2(mOled, dateTime.second());
      } else {
        mOled.print("  ");
      }
      mOled.clearToEOL();
      mOled.println();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      mOled.clearToEOL();
    }

    void displayTimeZone() const {
      const TimeZone& tz = mRenderingInfo.dateTime.timeZone();
      switch (tz.getType()) {
        case TimeZone::kTypeBasic:
        case TimeZone::kTypeExtended:
        case TimeZone::kTypeBasicManaged:
        case TimeZone::kTypeExtendedManaged:
          // Print name of timezone
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_NAME)) {
            tz.printShortTo(mOled);
          }
          mOled.clearToEOL();
          break;
        default:
          mOled.print(F("<unknown>"));
          mOled.clearToEOL();
          break;
      }
    }

    /**
     * True if the display should actually show the data for the given mode.
     * If the clock is in "blinking" mode, then this will return false in
     * accordance with the mBlinkShowState.
     */
    bool shouldShowFor(uint8_t mode) const {
      return mode != mRenderingInfo.mode
          || mRenderingInfo.suppressBlink
          || mRenderingInfo.blinkShowState;
    }

    /** The display needs to be cleared before rendering. */
    bool needsClear() const {
      return mRenderingInfo.mode != mPrevRenderingInfo.mode;
    }

    /** The display needs to be updated because something changed. */
    bool needsUpdate() const {
      return mRenderingInfo.mode != mPrevRenderingInfo.mode
          || mRenderingInfo.suppressBlink != mPrevRenderingInfo.suppressBlink
          || (!mRenderingInfo.suppressBlink
              && (mRenderingInfo.blinkShowState
                  != mPrevRenderingInfo.blinkShowState))
          || mRenderingInfo.dateTime != mPrevRenderingInfo.dateTime
          || mRenderingInfo.timePeriod != mPrevRenderingInfo.timePeriod;
    }

    SSD1306Ascii& mOled;
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
};

#endif
