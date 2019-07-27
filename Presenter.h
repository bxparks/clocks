#ifndef MED_MINDER_PRESENTER_H
#define MED_MINDER_PRESENTER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <ace_time/common/DateStrings.h>
#include "RenderingInfo.h"

namespace med_minder {

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
      mOled.setFont(lcd5x7);
      mOled.set2X();

      switch (mRenderingInfo.mode) {
        case MODE_DATE_TIME:
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          displayDateTime();
          break;

        case MODE_TIME_ZONE:
        case MODE_CHANGE_TIME_ZONE_HOUR:
        case MODE_CHANGE_TIME_ZONE_MINUTE:
        case MODE_CHANGE_TIME_ZONE_DST:
          displayTimeZone();
          break;

        case MODE_VIEW_MED:
          displayTimeRemaining();
          break;
        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          displayMedInterval();
          break;
      }
    }

    void displayDateTime() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

      // Date
      if (dateTime.isError()) {
        mOled.print("<INVALID>");
        return;
      }
      if (shouldShowFor(MODE_CHANGE_YEAR)) {
        printPad2(mOled, dateTime.year());
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
      mOled.println();

      // time
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
      const TimeZone& timeZone = mRenderingInfo.dateTime.timeZone();
      int8_t sign;
      uint8_t hour;
      uint8_t minute;
      timeZone.extractStandardHourMinute(sign, hour, minute);

      mOled.print("UTC");
      if (shouldShowFor(MODE_CHANGE_TIME_ZONE_HOUR)) {
        mOled.print((sign < 0) ? '-' : '+');
        printPad2(mOled, hour);
      } else {
        mOled.print("   ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_TIME_ZONE_MINUTE)) {
        printPad2(mOled, minute);
      } else {
        mOled.print("  ");
      }
      mOled.println();
      mOled.print("DST: ");
      if (shouldShowFor(MODE_CHANGE_TIME_ZONE_DST)) {
        mOled.print(timeZone.isDst() ? "on " : "off");
      } else {
        mOled.print("   ");
      }
    }

    void displayTimeRemaining() const {
      mOled.println("Med due");
      mRenderingInfo.timePeriod.printTo(mOled);
      mOled.clearToEOL();
    }

    void displayMedInterval() const {
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

}

#endif
