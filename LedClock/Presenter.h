#ifndef LED_CLOCK_PRESENTER_H
#define LED_CLOCK_PRESENTER_H

#include <AceTime.h>
#include <AceSegment.h>
#include "config.h"
#include "LedDisplay.h"
#include "RenderingInfo.h"

using namespace ace_segment;
using namespace ace_time;
using ace_time::DateStrings;

class Presenter {
  public:
    Presenter(const LedDisplay& display):
        mDisplay(display) {}

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

    void clearDisplay() { mDisplay.renderer->clear(); }

    void displayData() {
      setBlinkStyle();

      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;
      switch ((Mode) mRenderingInfo.mode) {
        case Mode::kViewHourMinute:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
          mDisplay.clockWriter->writeClock(dateTime.hour(), dateTime.minute());
          break;

        case Mode::kViewMinuteSecond:
          mDisplay.clockWriter->writeCharAt(0, ClockWriter::kSpace);
          mDisplay.clockWriter->writeCharAt(1, ClockWriter::kSpace);
          mDisplay.clockWriter->writeDecimalAt(2, dateTime.second());
          mDisplay.clockWriter->writeColon(true);
          break;

        case Mode::kViewYear:
        case Mode::kChangeYear:
          mDisplay.clockWriter->writeClock(20, dateTime.yearTiny());
          mDisplay.clockWriter->writeColon(false);
          break;

        case Mode::kViewMonth:
        case Mode::kChangeMonth:
          mDisplay.clockWriter->writeDecimalAt(0, dateTime.month());
          mDisplay.clockWriter->writeColon(false);
          mDisplay.clockWriter->writeCharAt(2, ClockWriter::kSpace);
          mDisplay.clockWriter->writeCharAt(3, ClockWriter::kSpace);
          break;

        case Mode::kViewDay:
        case Mode::kChangeDay:
          mDisplay.clockWriter->writeDecimalAt(0, dateTime.day());
          mDisplay.clockWriter->writeColon(false);
          mDisplay.clockWriter->writeCharAt(2, ClockWriter::kSpace);
          mDisplay.clockWriter->writeCharAt(3, ClockWriter::kSpace);
          break;

        case Mode::kViewWeekday:
          mDisplay.stringWriter->writeStringAt(
              0, DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()),
              true /* padRight */);
          break;

        default:
          break;
      }
    }

    void setBlinkStyle() {
      switch ((Mode) mRenderingInfo.mode) {
        case Mode::kChangeHour:
          mDisplay.clockWriter->writeStyleAt(0, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(1, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(2, 0);
          mDisplay.clockWriter->writeStyleAt(3, 0);
          break;

        case Mode::kChangeMinute:
          mDisplay.clockWriter->writeStyleAt(0, 0);
          mDisplay.clockWriter->writeStyleAt(1, 0);
          mDisplay.clockWriter->writeStyleAt(2, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(3, LedDisplay::BLINK_STYLE);
          break;

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
          mDisplay.clockWriter->writeStyleAt(0, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(1, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(2, LedDisplay::BLINK_STYLE);
          mDisplay.clockWriter->writeStyleAt(3, LedDisplay::BLINK_STYLE);
          break;

        default:
          mDisplay.clockWriter->writeStyleAt(0, 0);
          mDisplay.clockWriter->writeStyleAt(1, 0);
          mDisplay.clockWriter->writeStyleAt(2, 0);
          mDisplay.clockWriter->writeStyleAt(3, 0);
      }
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    const LedDisplay& mDisplay;
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;

};

#endif
