#ifndef WORLD_CLOCK_PRESENTER_H
#define WORLD_CLOCK_PRESENTER_H

#include <SSD1306Ascii.h>
#include <AceTime.h>
#include "ClockInfo.h"
#include "config.h"

using namespace ace_time;
using ace_common::printPad2To;

/**
 * Class that knows how to render a specific Mode on the OLED display.
 *
 * Note: Don't use F() macro for the short strings in this class. It causes the
 * flash/ram to increase from (27748/1535) to (27820/1519). In other words, we
 * increase program size by 72 bytes, to save 16 bytes of RAM. For the
 * WorldClock app, flash memory is more precious than RAM.
 */
class Presenter {
  public:
    /** Constructor. */
    Presenter(SSD1306Ascii& oled):
        mOled(oled) {}

    void display() {
      if (mClockInfo.mode == Mode::kUnknown) {
        clearDisplay();
        return;
      }

      if (needsClear()) {
        clearDisplay();
      }

      if (needsUpdate()) {
      #if ENABLE_SERIAL_DEBUG >= 1
        SERIAL_PORT_MONITOR.println(F("display(): needsUpdate"));
      #endif
        writeDisplayData();
        writeDisplaySettings();
        mPrevClockInfo = mClockInfo;
      }
    }

    /**
     * Set the ClockInfo of the Presenter. This is called about 10 times a
     * second.
     */
    void setClockInfo(const ClockInfo& clockInfo) {
      mClockInfo = clockInfo;
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    /** Write the display settings (brightness, contrast, inversion). */
    void writeDisplaySettings() {
      // Update contrastLevel if changed.
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.contrastLevel != mClockInfo.contrastLevel) {
        uint8_t value = toContrastValue(mClockInfo.contrastLevel);
        mOled.setContrast(value);
      }

      // Update invertDisplay if changed.
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.invertState != mClockInfo.invertState) {
        mOled.invertDisplay(mClockInfo.invertState);
      }
    }

    void clearDisplay() const { mOled.clear(); }

    void clearToEOL() const {
      mOled.clearToEOL();
      mOled.println();
    }

    /**
     * Set font and size.
     *
     *  0 - extra small size
     *  1 - normal 1X
     *  2 - double 2X
     */
    void setFont(uint8_t size) const {
      if (size == 0) {
        mOled.setFont(Adafruit5x7);
        mOled.set1X();
      } else if (size == 1) {
        mOled.setFont(fixed_bold10x15);
        mOled.set1X();
      } else if (size == 2) {
        mOled.setFont(fixed_bold10x15);
        mOled.set2X();
      }
    }

    void writeDisplayData() {
      mOled.home();

      switch ((Mode) mClockInfo.mode) {
        case Mode::kViewDateTime:
          displayDateTime();
          break;

        case Mode::kViewAbout:
          displayAbout();
          break;

        case Mode::kViewSettings:
        case Mode::kChangeHourMode:
        case Mode::kChangeBlinkingColon:
        case Mode::kChangeContrast:
        case Mode::kChangeInvertDisplay:
          displayClockInfo();
          break;

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          displayChangeableDateTime();
          break;

        default:
          break;
      }
    }

    void displayDateTime() const {
      setFont(1);

      if (mClockInfo.dateTime.isError()) {
        clearDisplay();
        mOled.println(F("<Error>"));
        return;
      }

      // time
      setFont(2);
      uint8_t hour = mClockInfo.dateTime.hour();
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        hour = toTwelveHour(hour);
        printPad2To(mOled, hour, ' ');
      } else {
        printPad2To(mOled, hour, '0');
      }
      mOled.print(
          (! mClockInfo.blinkingColon || shouldShowFor(Mode::kViewDateTime))
          ? ':' : ' ');
      printPad2To(mOled, mClockInfo.dateTime.minute(), '0');

      // AM/PM indicator
      mOled.set1X();
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        mOled.print((mClockInfo.dateTime.hour() < 12) ? 'A' : 'P');
      }

      // dayOfWeek, month/day, AM/PM
      // "Thu 10/18 P"
      mOled.println();
      mOled.println();

      mOled.print(DateStrings().dayOfWeekShortString(
          mClockInfo.dateTime.dayOfWeek()));
      mOled.print(' ');
      printPad2To(mOled, mClockInfo.dateTime.month(), ' ');
      mOled.print('/');
      printPad2To(mOled, mClockInfo.dateTime.day(), '0');
      mOled.print(' ');
      clearToEOL();

      // place name
      ZonedExtra extra = ZonedExtra::forLocalDateTime(
          mClockInfo.dateTime.localDateTime(),
          mClockInfo.dateTime.timeZone());
      mOled.print(extra.abbrev());
      mOled.print(' ');
      mOled.print('(');
      mOled.print(mClockInfo.name);
      mOled.print(')');
      clearToEOL();
    }

    /** Return 12 hour version of 24 hour. */
    static uint8_t toTwelveHour(uint8_t hour) {
      if (hour == 0) {
        return 12;
      } else if (hour > 12) {
        return hour - 12;
      } else {
        return hour;
      }
    }

    void displayChangeableDateTime() const {
      setFont(1);

      // date
      if (shouldShowFor(Mode::kChangeYear)) {
        mOled.print(mClockInfo.dateTime.year());
      } else {
        mOled.print("    ");
      }
      mOled.print('-');
      if (shouldShowFor(Mode::kChangeMonth)) {
        printPad2To(mOled, mClockInfo.dateTime.month(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print('-');
      if (shouldShowFor(Mode::kChangeDay)) {
        printPad2To(mOled, mClockInfo.dateTime.day(), '0');
      } else{
        mOled.print("  ");
      }
      clearToEOL();

      // time
      if (shouldShowFor(Mode::kChangeHour)) {
        uint8_t hour = mClockInfo.dateTime.hour();
        if (mClockInfo.hourMode == ClockInfo::kTwelve) {
          if (hour == 0) {
            hour = 12;
          } else if (hour > 12) {
            hour -= 12;
          }
          printPad2To(mOled, hour, ' ');
        } else {
          printPad2To(mOled, hour, '0');
        }
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(Mode::kChangeMinute)) {
        printPad2To(mOled, mClockInfo.dateTime.minute(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(Mode::kChangeSecond)) {
        printPad2To(mOled, mClockInfo.dateTime.second(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(' ');
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        mOled.print((mClockInfo.dateTime.hour() < 12) ? "AM" : "PM");
      }
      clearToEOL();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(
          mClockInfo.dateTime.dayOfWeek()));
      clearToEOL();

      // abbreviation and place name
      ZonedExtra extra = ZonedExtra::forLocalDateTime(
          mClockInfo.dateTime.localDateTime(),
          mClockInfo.dateTime.timeZone());
      mOled.print(extra.abbrev());
      mOled.print(' ');
      mOled.print('(');
      mOled.print(mClockInfo.name);
      mOled.print(')');
      clearToEOL();
    }

    void displayClockInfo() const {
      mOled.print(F("12/24:"));
      if (shouldShowFor(Mode::kChangeHourMode)) {
        mOled.print(mClockInfo.hourMode == ClockInfo::kTwelve
            ? "12" : "24");
      }
      clearToEOL();

      mOled.print(F("Blink:"));
      if (shouldShowFor(Mode::kChangeBlinkingColon)) {
        mOled.print(mClockInfo.blinkingColon ? "on " : "off");
      }
      clearToEOL();

      mOled.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeContrast)) {
        mOled.print(mClockInfo.contrastLevel);
      }
      clearToEOL();

      mOled.print(F("Invert:"));
      if (shouldShowFor(Mode::kChangeInvertDisplay)) {
        const __FlashStringHelper* statusString = F("<error>");
        switch (mClockInfo.invertDisplay) {
          case ClockInfo::kInvertDisplayOff:
            statusString = F("off");
            break;
          case ClockInfo::kInvertDisplayOn:
            statusString = F("on");
            break;
          case ClockInfo::kInvertDisplayMinutely:
            statusString = F("min");
            break;
          case ClockInfo::kInvertDisplayHourly:
            statusString = F("hour");
            break;
          case ClockInfo::kInvertDisplayDaily:
            statusString = F("day");
            break;
        }
        mOled.print(statusString);
      }
      clearToEOL();
    }

    void displayAbout() const;

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

    static uint8_t toContrastValue(uint8_t level) {
      if (level > kNumContrastValues - 1) level = kNumContrastValues - 1;
      return kContrastValues[level];
    }

  private:
    static const uint8_t kNumContrastValues = 10;
    static const uint8_t kContrastValues[];

    SSD1306Ascii& mOled;

    mutable ClockInfo mClockInfo;
    mutable ClockInfo mPrevClockInfo;
};

#endif
