#ifndef WORLD_CLOCK_PRESENTER_H
#define WORLD_CLOCK_PRESENTER_H

#include <SSD1306Ascii.h>
#include <AceTime.h>
#include "RenderingInfo.h"
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
      if (mRenderingInfo.mode == MODE_UNKNOWN) {
        clearDisplay();
        return;
      }

      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        updateDisplaySettings();
        displayData();
        mIsRendered = true;
      }
    }

    /**
     * Updating the rendering info and doing the actual rendering is now
     * decoupled in WorldClock because it takes too long to render the 3 OLED
     * displays, while blocking everything else. So use the 'mIsRendered' flag
     * to keep track of whether the mPrevRenderingInfo has actually been
     * rendered or not, before clobbering it with the new RenderingInfo.
     */
    void setRenderingInfo(uint8_t mode, acetime_t now, bool blinkShowState,
        const ClockInfo& clockInfo) {
      if (mIsRendered) {
        mPrevRenderingInfo = mRenderingInfo;
        mIsRendered = false;
      }

      mRenderingInfo.mode = mode;
      mRenderingInfo.now = now;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.clockInfo = clockInfo;
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    /**
     * Update the display settings, e.g. brightness, backlight, etc. Normally,
     * this is called from display(). But the WorldClock is special because it
     * has 3 displays so updating all of them takes too much time. But if a
     * display settings is changed (backlight, inversion, etc), we have to
     * update those settings right away. So this method is exposed so that
     * Controller::update() can call this lighter weight method, instead of
     * display().
     */
    void updateDisplaySettings() {
      ClockInfo& prevClockInfo = mPrevRenderingInfo.clockInfo;
      ClockInfo& clockInfo = mRenderingInfo.clockInfo;

      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.contrastLevel != clockInfo.contrastLevel) {
        uint8_t value = toContrastValue(clockInfo.contrastLevel);
        mOled.setContrast(value);
      }

      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.invertDisplay != clockInfo.invertDisplay) {
        mOled.invertDisplay(clockInfo.invertDisplay);
      }
    }

    void clearDisplay() { mOled.clear(); }

    void displayData() {
      mOled.home();

      switch (mRenderingInfo.mode) {
        case MODE_DATE_TIME:
          displayDateTime();
          break;

        case MODE_ABOUT:
          displayAbout();
          break;

        case MODE_SETTINGS:
        case MODE_CHANGE_HOUR_MODE:
        case MODE_CHANGE_BLINKING_COLON:
        case MODE_CHANGE_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_DST0:
        case MODE_CHANGE_TIME_ZONE_DST1:
        case MODE_CHANGE_TIME_ZONE_DST2:
#endif
          displayClockInfo();
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          displayChangeableDateTime();
          break;
      }
    }

    void displayDateTime() {
      mOled.setFont(fixed_bold10x15);

      ClockInfo& clockInfo = mRenderingInfo.clockInfo;
      const ZonedDateTime dateTime = ZonedDateTime::forEpochSeconds(
          mRenderingInfo.now, clockInfo.timeZone);
      if (dateTime.isError()) {
        clearDisplay();
        mOled.println(F("<Error>"));
        return;
      }

      // time
      mOled.set2X();
      uint8_t hour = dateTime.hour();
      if (clockInfo.hourMode == ClockInfo::kTwelve) {
        if (hour == 0) {
          hour = 12;
        } else if (hour > 12) {
          hour -= 12;
        }
        printPad2To(mOled, hour, ' ');
      } else {
        printPad2To(mOled, hour, '0');
      }
      mOled.print(
          (! clockInfo.blinkingColon || shouldShowFor(MODE_DATE_TIME))
          ? ':' : ' ');
      printPad2To(mOled, dateTime.minute(), '0');

      // AM/PM indicator
      mOled.set1X();
      if (clockInfo.hourMode == ClockInfo::kTwelve) {
        mOled.print((dateTime.hour() < 12) ? 'A' : 'P');
      }

      // dayOfWeek, month/day, AM/PM
      // "Thu 10/18 P"
      mOled.println();
      mOled.println();

      mOled.print(DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()));
      mOled.print(' ');
      printPad2To(mOled, dateTime.month(), ' ');
      mOled.print('/');
      printPad2To(mOled, dateTime.day(), '0');
      mOled.print(' ');
      mOled.clearToEOL();
      mOled.println();

      // place name
      acetime_t epochSeconds = dateTime.toEpochSeconds();
      mOled.print(dateTime.timeZone().getAbbrev(epochSeconds));
      mOled.print(' ');
      mOled.print('(');
      mOled.print(clockInfo.name);
      mOled.print(')');
      mOled.clearToEOL();
      mOled.println();
    }

    void displayChangeableDateTime() const {
      const ClockInfo& clockInfo = mRenderingInfo.clockInfo;
      const ZonedDateTime dateTime = ZonedDateTime::forEpochSeconds(
          mRenderingInfo.now, clockInfo.timeZone);

      mOled.setFont(fixed_bold10x15);
      mOled.set1X();

      // date
      if (shouldShowFor(MODE_CHANGE_YEAR)) {
        mOled.print(dateTime.year());
      } else {
        mOled.print("    ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_MONTH)) {
        printPad2To(mOled, dateTime.month(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_DAY)) {
        printPad2To(mOled, dateTime.day(), '0');
      } else{
        mOled.print("  ");
      }
      mOled.clearToEOL();
      mOled.println();

      // time
      if (shouldShowFor(MODE_CHANGE_HOUR)) {
        uint8_t hour = dateTime.hour();
        if (clockInfo.hourMode == ClockInfo::kTwelve) {
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
      if (shouldShowFor(MODE_CHANGE_MINUTE)) {
        printPad2To(mOled, dateTime.minute(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_SECOND)) {
        printPad2To(mOled, dateTime.second(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(' ');
      if (clockInfo.hourMode == ClockInfo::kTwelve) {
        mOled.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
      mOled.clearToEOL();
      mOled.println();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      mOled.clearToEOL();

      // abbreviation and place name
      mOled.println();
      mOled.print(dateTime.timeZone().getAbbrev(mRenderingInfo.now));
      mOled.print(' ');
      mOled.print('(');
      mOled.print(clockInfo.name);
      mOled.print(')');
      mOled.clearToEOL();
      mOled.println();
    }

    void displayClockInfo() const {
      const ClockInfo& clockInfo = mRenderingInfo.clockInfo;

      mOled.print(F("12/24:"));
      if (shouldShowFor(MODE_CHANGE_HOUR_MODE)) {
        mOled.print(clockInfo.hourMode == ClockInfo::kTwelve
            ? "12" : "24");
      } else {
        mOled.print("  ");
      }
      mOled.println();

      mOled.print(F("Blink:"));
      if (shouldShowFor(MODE_CHANGE_BLINKING_COLON)) {
        mOled.print(clockInfo.blinkingColon ? "on " : "off");
      } else {
        mOled.print("   ");
      }
      mOled.println();

      mOled.print(F("Contrast:"));
      if (shouldShowFor(MODE_CHANGE_CONTRAST)) {
        mOled.print(clockInfo.contrastLevel);
      } else {
        mOled.print(' ');
      }
      mOled.println();

      mOled.print(F("Invert:"));
      if (shouldShowFor(MODE_CHANGE_INVERT_DISPLAY)) {
        mOled.println(clockInfo.invertDisplay);
      } else {
        mOled.println(' ');
      }

      // Extract time zone info.
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
      const TimeZone& timeZone = clockInfo.timeZone;
      TimeOffset timeOffset = timeZone.getUtcOffset(0);
      int8_t hour;
      uint8_t minute;
      timeOffset.toHourMinute(hour, minute);

      mOled.print("UTC");
      mOled.print((hour < 0) ? '-' : '+');
      if (hour < 0) hour = -hour;
      printPad2To(mOled, hour, '0');
      mOled.print(':');
      printPad2To(mOled, minute, '0');
      mOled.println();

      mOled.print("DST:");
      if (shouldShowFor(MODE_CHANGE_TIME_ZONE_DST0)
          && shouldShowFor(MODE_CHANGE_TIME_ZONE_DST1)
          && shouldShowFor(MODE_CHANGE_TIME_ZONE_DST2)) {
        mOled.print(timeZone.isDst() ? "on " : "off");
      } else {
        mOled.print("   ");
      }
      mOled.println();
#endif
    }

    void displayAbout() const;

    /**
     * True if the display should actually show the data. If the clock is in
     * "blinking" mode, then this will return false in accordance with the
     * mBlinkShowState.
     */
    bool shouldShowFor(uint8_t mode) const {
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

    static uint8_t toContrastValue(uint8_t level) {
      if (level > kNumContrastValues - 1) level = kNumContrastValues - 1;
      return kContrastValues[level];
    }

  private:
    static const uint8_t kNumContrastValues = 10;
    static const uint8_t kContrastValues[];

    SSD1306Ascii& mOled;

    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
    bool mIsRendered = true; // true after a successful display()
};

#endif
