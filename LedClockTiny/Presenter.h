#ifndef LED_CLOCK_TINY_PRESENTER_H
#define LED_CLOCK_TINY_PRESENTER_H

#include <AceTime.h>
#include <AceSegment.h>
#include "config.h"
#include "ClockInfo.h"

using ace_time::DateStrings;
using ace_time::OffsetDateTime;
using ace_segment::kDigitSpace;
using ace_segment::kPatternSpace;
using ace_segment::LedModule;
using ace_segment::PatternWriter;
using ace_segment::NumberWriter;
using ace_segment::ClockWriter;
using ace_segment::CharWriter;
using ace_segment::StringWriter;

class Presenter {
  public:
    Presenter(LedModule& ledModule):
        mDisplay(ledModule),
        mPatternWriter(ledModule),
        mNumberWriter(mPatternWriter),
        mClockWriter(mNumberWriter),
        mCharWriter(mPatternWriter),
        mStringWriter(mCharWriter)
    {}

    void updateDisplay() {
      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        updateDisplaySettings();
        displayData();
      }

      mPrevClockInfo = mClockInfo;
    }

    void setClockInfo(const ClockInfo& clockInfo) {
      mClockInfo = clockInfo;
    }

  private:
    /**
     * True if the display should actually show the data. If the clock is in
     * "blinking" mode, then this will return false in accordance with the
     * mBlinkShowState.
     */
    bool shouldShowFor(Mode mode) const {
      return mode != mClockInfo.mode
          || mClockInfo.blinkShowState
          || mClockInfo.suppressBlink;
    }

    /** The display needs to be cleared before rendering. */
    bool needsClear() const {
      return mClockInfo.mode != mPrevClockInfo.mode;
    }

    /** The display needs to be updated because something changed. */
    bool needsUpdate() const {
      return mClockInfo != mPrevClockInfo;
    }

    /** Update the display settings, e.g. brightness, backlight, etc. */
    void updateDisplaySettings() {
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.brightness != mClockInfo.brightness) {
        mDisplay.setBrightness(mClockInfo.brightness);
      }
    }

    void clearDisplay() { mPatternWriter.clear(); }

    void displayData() {
      const OffsetDateTime& dateTime = mClockInfo.dateTime;
      #if ENABLE_SERIAL_DEBUG >= 2
        SERIAL_PORT_MONITOR.print(F("displayData():"));
        dateTime.printTo(SERIAL_PORT_MONITOR);
        SERIAL_PORT_MONITOR.println();
      #endif

      mPatternWriter.home();

      switch ((Mode) mClockInfo.mode) {
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

        case Mode::kViewWeekday:
        case Mode::kChangeWeekday:
          displayWeekday(dateTime);
          break;

        case Mode::kViewBrightness:
        case Mode::kChangeBrightness:
          displayBrightness();
          break;

        default:
          break;
      }
    }

    void displayHourMinute(const OffsetDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        mNumberWriter.writeDec2(dateTime.hour());
      } else {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }

      if (shouldShowFor(Mode::kChangeMinute)) {
        mNumberWriter.writeDec2(dateTime.minute());
      } else {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }
      mClockWriter.writeColon(true);
    }

    void displaySecond(const OffsetDateTime& dateTime) {
      mNumberWriter.writeDigit(kDigitSpace);
      mNumberWriter.writeDigit(kDigitSpace);

      if (shouldShowFor(Mode::kChangeSecond)) {
        mNumberWriter.writeDec2(dateTime.second());
        mClockWriter.writeColon(true);
      } else {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }
    }

    void displayYear(const OffsetDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeYear)) {
        mNumberWriter.writeDec4(dateTime.year());
      } else {
        clearDisplay();
      }
      mClockWriter.writeColon(false);
    }

    void displayMonth(const OffsetDateTime& dateTime) {
      mNumberWriter.writeDigit(kDigitSpace);
      mNumberWriter.writeDigit(kDigitSpace);
      if (shouldShowFor(Mode::kChangeMonth)) {
        mNumberWriter.writeDec2(dateTime.month());
      } else {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }
      mClockWriter.writeColon(false);
    }

    void displayDay(const OffsetDateTime& dateTime) {
      mNumberWriter.writeDigit(kDigitSpace);
      mNumberWriter.writeDigit(kDigitSpace);
      if (shouldShowFor(Mode::kChangeDay)) {
        mNumberWriter.writeDec2(dateTime.day());
      } else  {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }
      mClockWriter.writeColon(false);
    }

    void displayWeekday(const OffsetDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeWeekday)) {
        mStringWriter.writeString(
            DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()));
        mStringWriter.clearToEnd();
      } else {
        clearDisplay();
      }
    }

    void displayBrightness() {
      mCharWriter.writeChar('B');
      mCharWriter.writeChar('r');
      mClockWriter.writeColon(true);
      if (shouldShowFor(Mode::kChangeBrightness)) {
        // Save 110 bytes of flash using NumberWriter::writeDec2() instead of
        // the more general writeUnsignedDecimal(). We could avoid
        // NumberWriter completely by manually doing the conversion here. But
        // that turns out to save only 18 bytes (12 of which are character
        // patterns in NumberWriter::kDigitPatterns[]) so not really worth
        // duplicating the code here.
        mNumberWriter.writeDec2(mClockInfo.brightness, kPatternSpace);
      } else {
        mNumberWriter.writeDigit(kDigitSpace);
        mNumberWriter.writeDigit(kDigitSpace);
      }
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    LedModule& mDisplay;
    PatternWriter<LedModule> mPatternWriter;
    NumberWriter<LedModule> mNumberWriter;
    ClockWriter<LedModule> mClockWriter;
    CharWriter<LedModule> mCharWriter;
    StringWriter<LedModule> mStringWriter;

    ClockInfo mClockInfo;
    ClockInfo mPrevClockInfo;

};

#endif
