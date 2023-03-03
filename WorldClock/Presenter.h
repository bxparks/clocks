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
      if (mRenderingInfo.mode == Mode::kUnknown) {
        clearDisplay();
        return;
      }

      if (needsClear()) {
        clearDisplay();
      }

      updateDisplaySettings();

      if (needsUpdate()) {
      #if ENABLE_SERIAL_DEBUG >= 1
        SERIAL_PORT_MONITOR.println(F("display(): needsUpdate"));
      #endif
        writeDisplayData();
        writeDisplaySettings();
        mPrevRenderingInfo = mRenderingInfo;
      }
    }

    /**
     * Set the RenderingInfo of the Presenter. This is called about 10 times a
     * second.
     */
    void setRenderingInfo(
        Mode mode, acetime_t now,
        const ClockInfo& clockInfo,
        const TimeZone& primaryTimeZone) {

      mRenderingInfo.mode = mode;
      mRenderingInfo.now = now;
      mRenderingInfo.clockInfo = clockInfo;
      mRenderingInfo.primaryTimeZone = primaryTimeZone;
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    /**
     * Update the display settings, e.g. brightness, backlight, etc, but don't
     * write the data to the display yet.
     */
    void updateDisplaySettings() {
      ClockInfo& clockInfo = mRenderingInfo.clockInfo;

      // Calculate the next actual invertDisplay setting. Automatically
      // alternating inversion is an attempt to extend the life-time of these
      // OLED devices which seem to suffer from burn-in after about 6-12 months.
      uint8_t invertDisplay;
      if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayMinutely
          || clockInfo.invertDisplay == ClockInfo::kInvertDisplayDaily
          || clockInfo.invertDisplay == ClockInfo::kInvertDisplayHourly) {
        const ZonedDateTime dateTime = ZonedDateTime::forEpochSeconds(
            mRenderingInfo.now, mRenderingInfo.primaryTimeZone);
        const LocalDateTime& ldt = dateTime.localDateTime();
        // The XOR alternates the pattern of on/off to smooth out the wear level
        // on specific digits. For example, if kInvertDisplayMinutely is
        // selected, and if last bit of only the minute is used, then the "1" on
        // the minute segment will always be inverted, which will cause uneven
        // wearning. By XOR'ing with the hour(), we invert the on/off cycle
        // every hour.
        if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayMinutely) {
          invertDisplay = ((ldt.minute() & 0x1) ^ (ldt.hour() & 0x1))
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        } else if (clockInfo.invertDisplay == ClockInfo::kInvertDisplayHourly) {
          invertDisplay = ((ldt.hour() & 0x1) ^ (ldt.day() & 0x1))
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        } else {
          invertDisplay = (7 <= ldt.hour() && ldt.hour() < 19)
              ? ClockInfo::kInvertDisplayOn
              : ClockInfo::kInvertDisplayOff;
        }
      } else {
        invertDisplay = clockInfo.invertDisplay;
      }
      mRenderingInfo.invertDisplay = invertDisplay;
    }

    /** Write the display settings (brightness, contrast, inversion). */
    void writeDisplaySettings() {
      ClockInfo& prevClockInfo = mPrevRenderingInfo.clockInfo;
      ClockInfo& clockInfo = mRenderingInfo.clockInfo;

      // Update contrastLevel if changed.
      if (mPrevRenderingInfo.mode == Mode::kUnknown
          || prevClockInfo.contrastLevel != clockInfo.contrastLevel) {
        uint8_t value = toContrastValue(mRenderingInfo.clockInfo.contrastLevel);
        mOled.setContrast(value);
      }

      // Update invertDisplay if changed.
      if (mPrevRenderingInfo.mode == Mode::kUnknown
          || mPrevRenderingInfo.invertDisplay != mRenderingInfo.invertDisplay) {
        mOled.invertDisplay(mRenderingInfo.invertDisplay);
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

      switch ((Mode) mRenderingInfo.mode) {
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
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneDst0:
        case Mode::kChangeTimeZoneDst1:
        case Mode::kChangeTimeZoneDst2:
#endif
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

      ClockInfo& clockInfo = mRenderingInfo.clockInfo;
      const ZonedDateTime dateTime = ZonedDateTime::forEpochSeconds(
          mRenderingInfo.now, clockInfo.timeZone);
      if (dateTime.isError()) {
        clearDisplay();
        mOled.println(F("<Error>"));
        return;
      }

      // time
      setFont(2);
      uint8_t hour = dateTime.hour();
      if (clockInfo.hourMode == ClockInfo::kTwelve) {
        hour = toTwelveHour(hour);
        printPad2To(mOled, hour, ' ');
      } else {
        printPad2To(mOled, hour, '0');
      }
      mOled.print(
          (! clockInfo.blinkingColon || shouldShowFor(Mode::kViewDateTime))
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
      clearToEOL();

      // place name
      ZonedExtra ze = ZonedExtra::forEpochSeconds(
          mRenderingInfo.now, dateTime.timeZone());
      mOled.print(ze.abbrev());
      mOled.print(' ');
      mOled.print('(');
      mOled.print(clockInfo.name);
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
      const ClockInfo& clockInfo = mRenderingInfo.clockInfo;
      const ZonedDateTime dateTime = ZonedDateTime::forEpochSeconds(
          mRenderingInfo.now, clockInfo.timeZone);

      setFont(1);

      // date
      if (shouldShowFor(Mode::kChangeYear)) {
        mOled.print(dateTime.year());
      } else {
        mOled.print("    ");
      }
      mOled.print('-');
      if (shouldShowFor(Mode::kChangeMonth)) {
        printPad2To(mOled, dateTime.month(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print('-');
      if (shouldShowFor(Mode::kChangeDay)) {
        printPad2To(mOled, dateTime.day(), '0');
      } else{
        mOled.print("  ");
      }
      clearToEOL();

      // time
      if (shouldShowFor(Mode::kChangeHour)) {
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
      if (shouldShowFor(Mode::kChangeMinute)) {
        printPad2To(mOled, dateTime.minute(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(Mode::kChangeSecond)) {
        printPad2To(mOled, dateTime.second(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(' ');
      if (clockInfo.hourMode == ClockInfo::kTwelve) {
        mOled.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
      clearToEOL();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      clearToEOL();

      // abbreviation and place name
      ZonedExtra ze = ZonedExtra::forEpochSeconds(
          mRenderingInfo.now, dateTime.timeZone());
      mOled.print(ze.abbrev());
      mOled.print(' ');
      mOled.print('(');
      mOled.print(clockInfo.name);
      mOled.print(')');
      clearToEOL();
    }

    void displayClockInfo() const {
      const ClockInfo& clockInfo = mRenderingInfo.clockInfo;

      mOled.print(F("12/24:"));
      if (shouldShowFor(Mode::kChangeHourMode)) {
        mOled.print(clockInfo.hourMode == ClockInfo::kTwelve
            ? "12" : "24");
      }
      clearToEOL();

      mOled.print(F("Blink:"));
      if (shouldShowFor(Mode::kChangeBlinkingColon)) {
        mOled.print(clockInfo.blinkingColon ? "on " : "off");
      }
      clearToEOL();

      mOled.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeContrast)) {
        mOled.print(clockInfo.contrastLevel);
      }
      clearToEOL();

      mOled.print(F("Invert:"));
      if (shouldShowFor(Mode::kChangeInvertDisplay)) {
        const __FlashStringHelper* statusString = F("<error>");
        switch (clockInfo.invertDisplay) {
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
      clearToEOL();

      mOled.print("DST:");
      if (shouldShowFor(Mode::kChangeTimeZoneDst0)
          && shouldShowFor(Mode::kChangeTimeZoneDst1)
          && shouldShowFor(Mode::kChangeTimeZoneDst2)) {
        mOled.print(timeZone.isDst() ? "on " : "off");
      }
      clearToEOL();
#endif
    }

    void displayAbout() const;

    /**
     * True if the display should actually show the data. If the clock is in
     * "blinking" mode, then this will return false in accordance with the
     * mBlinkShowState.
     */
    bool shouldShowFor(Mode mode) const {
      return mode != mRenderingInfo.mode
          || mRenderingInfo.clockInfo.blinkShowState
          || mRenderingInfo.clockInfo.suppressBlink;
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

    mutable RenderingInfo mRenderingInfo;
    mutable RenderingInfo mPrevRenderingInfo;
};

#endif
