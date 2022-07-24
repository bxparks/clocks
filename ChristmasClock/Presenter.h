#ifndef CHRISTMAS_CLOCK_PRESENTER_H
#define CHRISTMAS_CLOCK_PRESENTER_H

#include <AceTime.h>
#include <AceSegment.h>
#include <AceSegmentWriter.h>
#include "config.h"
#include "RenderingInfo.h"

using ace_time::DateStrings;
using ace_time::ZonedDateTime;
using ace_segment::kHexCharSpace;
using ace_segment::kPatternSpace;
using ace_segment::LedModule;
using ace_segment::PatternWriter;
using ace_segment::ClockWriter;
using ace_segment::NumberWriter;
using ace_segment::CharWriter;
using ace_segment::StringWriter;

class Presenter {
  public:
    Presenter(LedModule& ledModule):
        mLedModule(ledModule),
        mClockWriter(ledModule),
        mNumberWriter(ledModule),
        mCharWriter(ledModule),
        mStringWriter(mCharWriter)
    {}

    void display() {
      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        updateDisplaySettings();
        displayData();
      }

      mPrevRenderingInfo = mRenderingInfo;
    }

    void setRenderingInfo(Mode mode, bool blinkShowState,
        const ClockInfo& clockInfo) {
      mRenderingInfo.mode = mode;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.hourMode = clockInfo.hourMode;
      mRenderingInfo.brightness = clockInfo.brightness;
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

    /** Update the display settings, e.g. brightness, backlight, etc. */
    void updateDisplaySettings() {
      if (mPrevRenderingInfo.mode == Mode::kUnknown ||
          mPrevRenderingInfo.brightness != mRenderingInfo.brightness) {
        mLedModule.setBrightness(mRenderingInfo.brightness);
      }
    }

    void clearDisplay() { mClockWriter.clear(); }

    void displayData() {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;
      #if ENABLE_SERIAL_DEBUG >= 2
        SERIAL_PORT_MONITOR.print(F("displayData():"));
        dateTime.printTo(SERIAL_PORT_MONITOR);
        SERIAL_PORT_MONITOR.println();
      #endif

      switch ((Mode) mRenderingInfo.mode) {
        case Mode::kViewHourMinute:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
          displayHourMinute(dateTime);
          break;

        case Mode::kViewSecond:
        case Mode::kChangeSecond:
          displaySecond(dateTime);
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

        case Mode::kViewWeekday: {
          uint8_t written = mStringWriter.writeStringAt(
              0, DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()));
          mStringWriter.clearToEnd(written);
          break;
        }

        case Mode::kViewBrightness:
        case Mode::kChangeBrightness:
          displayBrightness();
          break;

        default:
          break;
      }
    }

    void displayHourMinute(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        mNumberWriter.writeDec2At(0, dateTime.hour());
      } else {
        mNumberWriter.writeHexChars2At(0, kHexCharSpace, kHexCharSpace);
      }

      if (shouldShowFor(Mode::kChangeMinute)) {
        mNumberWriter.writeDec2At(2, dateTime.minute());
      } else {
        mNumberWriter.writeHexChars2At(2, kHexCharSpace, kHexCharSpace);
      }
      mClockWriter.writeColon(true);
    }

    void displaySecond(const ZonedDateTime& dateTime) {
      mNumberWriter.writeHexChars2At(0, kHexCharSpace, kHexCharSpace);

      if (shouldShowFor(Mode::kChangeSecond)) {
        mNumberWriter.writeDec2At(2, dateTime.second());
        mClockWriter.writeColon(true);
      } else {
        mNumberWriter.writeHexChars2At(2, kHexCharSpace, kHexCharSpace);
      }
    }

    void displayYear(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeYear)) {
        mNumberWriter.writeDec4At(0, dateTime.year());
      } else {
        clearDisplay();
      }
      mClockWriter.writeColon(false);
    }

    void displayMonth(const ZonedDateTime& dateTime) {
      mNumberWriter.writeHexChars2At(0, kHexCharSpace, kHexCharSpace);
      if (shouldShowFor(Mode::kChangeMonth)) {
        mNumberWriter.writeDec2At(2, dateTime.month());
      } else {
        mNumberWriter.writeHexChars2At(2, kHexCharSpace, kHexCharSpace);
      }
      mClockWriter.writeColon(false);
    }

    void displayDay(const ZonedDateTime& dateTime) {
      mNumberWriter.writeHexChars2At(0, kHexCharSpace, kHexCharSpace);
      if (shouldShowFor(Mode::kChangeDay)) {
        mNumberWriter.writeDec2At(2, dateTime.day());
      } else  {
        mNumberWriter.writeHexChars2At(2, kHexCharSpace, kHexCharSpace);
      }
      mClockWriter.writeColon(false);
    }

    void displayBrightness() {
      mCharWriter.writeCharAt(0, 'b');
      mCharWriter.writeCharAt(1, 'r');
      mClockWriter.writeColon(true);
      if (shouldShowFor(Mode::kChangeBrightness)) {
        // Save 110 bytes of flash using NumberWriter::writeDec2At() instead of
        // the more general writeUnsignedDecimalAt(). We could avoid
        // NumberWriter completely by manually doing the conversion here. But
        // that turns out to save only 18 bytes (12 of which are character
        // patterns in NumberWriter::kHexCharPatterns[]) so not really worth
        // duplicating the code here.
        mNumberWriter.writeDec2At(2, mRenderingInfo.brightness, kPatternSpace);
      } else {
        mNumberWriter.writeHexChars2At(2, kHexCharSpace, kHexCharSpace);
      }
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    LedModule& mLedModule;
    ClockWriter<LedModule> mClockWriter;
    NumberWriter<LedModule> mNumberWriter;
    CharWriter<LedModule> mCharWriter;
    StringWriter<LedModule> mStringWriter;

    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;

};

#endif
