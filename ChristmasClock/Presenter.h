#ifndef CHRISTMAS_CLOCK_PRESENTER_H
#define CHRISTMAS_CLOCK_PRESENTER_H

#include <AceTime.h>
#include <AceSegment.h>
#include <AceSegmentWriter.h>
#include "config.h"
#include "ClockInfo.h"

using ace_time::acetime_t;
using ace_time::daysUntil;
using ace_time::DateStrings;
using ace_time::ZonedDateTime;
using ace_time::TimeZone;
using ace_time::TimeZoneData;
using ace_time::ManualZoneManager;
using ace_time::BasicZoneManager;
using ace_time::ExtendedZoneManager;
using ace_time::BasicZoneProcessor;
using ace_time::ExtendedZoneProcessor;
using ace_time::ZonedExtra;
using ace_segment::kDigitSpace;
using ace_segment::kPatternSpace;
using ace_segment::LedModule;
using ace_segment::PatternWriter;
using ace_segment::ClockWriter;
using ace_segment::NumberWriter;
using ace_segment::CharWriter;
using ace_segment::StringWriter;

class Presenter {
  public:
    Presenter(
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        BasicZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        ExtendedZoneManager& zoneManager,
      #endif
        LedModule& ledModule
    ):
        mZoneManager(zoneManager),
        mLedModule(ledModule),
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
        mLedModule.setBrightness(mClockInfo.brightness);
      }
    }

    void clearDisplay() { mPatternWriter.clear(); }

    void displayData() {
      const ZonedDateTime& dateTime = mClockInfo.dateTime;
      #if ENABLE_SERIAL_DEBUG >= 2
        SERIAL_PORT_MONITOR.print(F("displayData():"));
        dateTime.printTo(SERIAL_PORT_MONITOR);
        SERIAL_PORT_MONITOR.println();
      #endif

      mPatternWriter.home();

      switch (mClockInfo.mode) {
        case Mode::kViewCountdown:
          displayCountdown(dateTime);
          break;

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
          mStringWriter.clear();
          mStringWriter.writeString(
              DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()));
          break;
        }

        case Mode::kViewTimeZone:
        case Mode::kChangeTimeZone:
          displayTimeZone();
          break;

        case Mode::kViewBrightness:
        case Mode::kChangeBrightness:
          displayBrightness();
          break;

        case Mode::kViewVersion:
          displayVersion();
          break;

        default:
          break;
      }
    }

    /** Display number of days until Christmas. */
    void displayCountdown(const ZonedDateTime& dateTime) {
      int32_t days = daysUntil(dateTime.localDateTime().localDate(), 12, 25);
      mNumberWriter.writeDec4((uint16_t) days, kPatternSpace);
      mClockWriter.writeColon(false);
    }

    void displayHourMinute(const ZonedDateTime& dateTime) {
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

    void displaySecond(const ZonedDateTime& dateTime) {
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

    void displayYear(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeYear)) {
        mNumberWriter.writeDec4(dateTime.year());
      } else {
        clearDisplay();
      }
      mClockWriter.writeColon(false);
    }

    void displayMonth(const ZonedDateTime& dateTime) {
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

    void displayDay(const ZonedDateTime& dateTime) {
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

    void displayTimeZone() {
      TimeZone tz = mZoneManager.createForTimeZoneData(
          mClockInfo.timeZoneData);
      acetime_t epochSeconds = mClockInfo.dateTime.toEpochSeconds();
      if (shouldShowFor(Mode::kChangeTimeZone)) {
        const char* name;
        ZonedExtra ze;
        switch (tz.getType()) {
          case BasicZoneProcessor::kTypeBasic:
          case ExtendedZoneProcessor::kTypeExtended:
            ze = ZonedExtra::forEpochSeconds(epochSeconds, tz);
            name = ze.abbrev();
            break;

          case TimeZone::kTypeManual:
          default:
            name = "----";
            break;
        }
        mStringWriter.clear();
        mStringWriter.writeString(name);
      } else  {
        clearDisplay();
      }
      mClockWriter.writeColon(false);
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

    void displayVersion() {
      mCharWriter.writeChar('V');
      mCharWriter.writeChar('e');
      mClockWriter.writeColon(true);
      mNumberWriter.writeDec2(CHRISTMAS_CLOCK_VERSION, kPatternSpace);
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif

    LedModule& mLedModule;
    PatternWriter<LedModule> mPatternWriter;
    NumberWriter<LedModule> mNumberWriter;
    ClockWriter<LedModule> mClockWriter;
    CharWriter<LedModule> mCharWriter;
    StringWriter<LedModule> mStringWriter;

    ClockInfo mClockInfo;
    ClockInfo mPrevClockInfo;

};

#endif
