#ifndef LED_CLOCK_PRESENTER_H
#define LED_CLOCK_PRESENTER_H

#include <AceTime.h>
#include <AceSegment.h>
#include "config.h"
#include "RenderingInfo.h"

using ace_time::DateStrings;
using ace_time::ZonedDateTime;
using ace_segment::LedDisplay;
using ace_segment::ClockWriter;
using ace_segment::CharWriter;
using ace_segment::StringWriter;

class Presenter {
  public:
    Presenter(LedDisplay& display):
        mDisplay(display),
        mClockWriter(display),
        mCharWriter(display),
        mStringWriter(mCharWriter)
    {}

    void display() {
      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        displayData();
      }

      mPrevRenderingInfo = mRenderingInfo;
    }

    void setRenderingInfo(Mode mode, bool blinkShowState,
        const ClockInfo& clockInfo) {
      mRenderingInfo.mode = mode;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.hourMode = clockInfo.hourMode;
      mRenderingInfo.timeZoneData = clockInfo.timeZoneData;
      mRenderingInfo.dateTime = clockInfo.dateTime;
    }

  private:
    /**
     * True if the display should actually show the data. If the clock is in
     * "blinking" mode, then this will return false in accordance with the
     * mBlinkShowState.
     */
    bool shouldShowFor(Mode mode) const {
      return mode != mRenderingInfo.mode || mRenderingInfo.blinkShowState;
    }

    /** The display needs to be cleared before rendering. */
    bool needsClear() const {
      return mRenderingInfo.mode != mPrevRenderingInfo.mode;
    }

    /** The display needs to be updated because something changed. */
    bool needsUpdate() const {
      return mRenderingInfo != mPrevRenderingInfo;
    }

    void clearDisplay() { mDisplay.clear(); }

    void displayData() {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;
      #if ENABLE_SERIAL_DEBUG > 0
        dateTime.printTo(SERIAL_PORT_MONITOR);
        SERIAL_PORT_MONITOR.println();
      #endif

      switch ((Mode) mRenderingInfo.mode) {
        case Mode::kViewHourMinute:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
          displayHourMinute(dateTime);
          break;

        case Mode::kViewMinuteSecond:
          displayMinuteSecond(dateTime);
          break;

        case Mode::kViewYear:
        case Mode::kChangeYear:
          displayYear(dateTime);
          break;

        case Mode::kViewMonth:
        case Mode::kChangeMonth:
          displayMonth(dateTime);
          break;

        case Mode::kViewDay:
        case Mode::kChangeDay:
          displayDay(dateTime);
          break;

        case Mode::kViewWeekday:
          mStringWriter.writeStringAt(
              0, DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()),
              true /* padRight */);
          break;

        default:
          break;
      }
    }

    void displayHourMinute(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        mClockWriter.writeDec2At(0, dateTime.hour());
      } else {
        mClockWriter.writeCharAt(0, ClockWriter::kCharSpace);
        mClockWriter.writeCharAt(1, ClockWriter::kCharSpace);
      }

      if (shouldShowFor(Mode::kChangeMinute)) {
        mClockWriter.writeDec2At(2, dateTime.minute());
      } else {
        mClockWriter.writeCharAt(2, ClockWriter::kCharSpace);
        mClockWriter.writeCharAt(3, ClockWriter::kCharSpace);
      }
      mClockWriter.writeColon(true);
    }

    void displayMinuteSecond(const ZonedDateTime& dateTime) {
      mClockWriter.writeCharAt(0, ClockWriter::kCharSpace);
      mClockWriter.writeCharAt(1, ClockWriter::kCharSpace);
      mClockWriter.writeDec2At(2, dateTime.second());
      mClockWriter.writeColon(true);
    }

    void displayYear(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeYear)) {
        mClockWriter.writeDec4At(0, dateTime.year());
      } else {
        clearDisplay();
      }
      mClockWriter.writeColon(false);
    }

    void displayMonth(const ZonedDateTime& dateTime) {
      mClockWriter.writeCharAt(0, ClockWriter::kCharSpace);
      mClockWriter.writeCharAt(1, ClockWriter::kCharSpace);
      if (shouldShowFor(Mode::kChangeMonth)) {
        mClockWriter.writeDec2At(2, dateTime.month());
      } else {
        mClockWriter.writeCharAt(2, ClockWriter::kCharSpace);
        mClockWriter.writeCharAt(3, ClockWriter::kCharSpace);
      }
      mClockWriter.writeColon(false);
    }

    void displayDay(const ZonedDateTime& dateTime) {
      mClockWriter.writeCharAt(0, ClockWriter::kCharSpace);
      mClockWriter.writeCharAt(1, ClockWriter::kCharSpace);
      if (shouldShowFor(Mode::kChangeDay)) {
        mClockWriter.writeDec2At(2, dateTime.day());
      } else  {
        mClockWriter.writeCharAt(2, ClockWriter::kCharSpace);
        mClockWriter.writeCharAt(3, ClockWriter::kCharSpace);
      }
      mClockWriter.writeColon(false);
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    LedDisplay& mDisplay;
    ClockWriter mClockWriter;
    CharWriter mCharWriter;
    StringWriter mStringWriter;

    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;

};

#endif
