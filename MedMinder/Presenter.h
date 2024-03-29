#ifndef MED_MINDER_PRESENTER_H
#define MED_MINDER_PRESENTER_H

#include "config.h"
#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include "ClockInfo.h"
#include "ClockInfo.h"

using namespace ace_time;
using ace_common::printPad2To;

/** Class responsible for rendering the information to the OLED display. */
class Presenter {
  public:
    Presenter(
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        ManualZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        BasicZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        ExtendedZoneManager& zoneManager,
      #endif
      SSD1306Ascii& oled
    ):
        mZoneManager(zoneManager),
        mOled(oled) {}

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
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

    void prepareToSleep() {
      mOled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
    }

    void wakeup() {
      mOled.ssd1306WriteCmd(SSD1306_DISPLAYON);
    }

  private:
    void clearDisplay() { mOled.clear(); }

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

    void clearToEOL() const {
      mOled.clearToEOL();
      mOled.println();
    }

    void displayData() const {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.print(F("displayData(): "));
        SERIAL_PORT_MONITOR.print(F("mode="));
        SERIAL_PORT_MONITOR.println((uint8_t) mClockInfo.mode);
      }

      mOled.home();
      setFont(1);

      switch (mClockInfo.mode) {
        case Mode::kViewMed:
          displayMed();
          break;

        case Mode::kViewDateTime:
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          displayDateTime();
          break;

        case Mode::kViewAbout:
          displayAbout();
          break;

        case Mode::kViewTimeZone:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
      #endif
          displayTimeZone();
          break;

        case Mode::kChangeMedHour:
        case Mode::kChangeMedMinute:
          displayChangeMed();
          break;

        case Mode::kViewSettings:
        case Mode::kChangeSettingsContrast:
          displaySettings();
          break;

        default:
          break;
      }
    }

    void displayMed() const {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("displayMed()"));
      }

      mOled.println(F("Med due"));
      if (mClockInfo.medInterval.isError()) {
        mOled.print(F("<Overdue>"));
      } else {
        mClockInfo.medInterval.printTo(mOled);
      }
      clearToEOL();
    }

    void displayAbout() const {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("displayAbout()"));
      }
      setFont(0);
      mOled.println(F("MM: " MED_MINDER_VERSION_STRING));
      mOled.print(F("TZDB:"));
      mOled.println(zonedb::kTzDatabaseVersion);
      mOled.println(F("ATim:" ACE_TIME_VERSION_STRING));
      mOled.println(F("ABut:" ACE_BUTTON_VERSION_STRING));
      mOled.println(F("ARou:" ACE_ROUTINE_VERSION_STRING));
      mOled.println(F("ACom:" ACE_COMMON_VERSION_STRING));
    }

    void displayChangeMed() const {
      mOled.println("Med intrvl");

      if (shouldShowFor(Mode::kChangeMedHour)) {
        printPad2To(mOled, mClockInfo.medInterval.hour(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(Mode::kChangeMedMinute)) {
        printPad2To(mOled, mClockInfo.medInterval.minute(), '0');
      } else {
        mOled.print("  ");
      }
      clearToEOL();
    }

    void displayDateTime() const {
      if (ENABLE_SERIAL_DEBUG >= 1) {
        SERIAL_PORT_MONITOR.println(F("displayDateTime()"));
      }

      displayDate();
      displayTime();
    }

    void displayDate() const {
      const ZonedDateTime& dateTime = mClockInfo.dateTime;

      if (dateTime.isError()) {
        mOled.print(F("<INVALID>"));
        return;
      }
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
    }

    void displayTime() const {
      const ZonedDateTime& dateTime = mClockInfo.dateTime;

      if (shouldShowFor(Mode::kChangeHour)) {
        printPad2To(mOled, dateTime.hour(), '0');
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
      clearToEOL();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      clearToEOL();
    }

    void displayTimeZone() const {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayTimeZone()"));
      }

      // Display the timezone using the TimeZoneData, not the dateTime, since
      // dateTime will point to the old timeZone.
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mOled.print("TZ: ");
      const __FlashStringHelper* typeString;
      switch (tz.getType()) {
        case TimeZone::kTypeManual:
          typeString = F("manual");
          break;
        case BasicZoneProcessor::kTypeBasic:
          typeString = F("basic");
          break;
        case ExtendedZoneProcessor::kTypeExtended:
          typeString = F("extd");
          break;
        default:
          typeString = F("unknown");
      }
      mOled.print(typeString);
      clearToEOL();

      switch (tz.getType()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case TimeZone::kTypeManual:
          mOled.println();
          mOled.print("UTC");
          if (shouldShowFor(Mode::kChangeTimeZoneOffset)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mOled);
          }
          clearToEOL();

          mOled.print("DST: ");
          if (shouldShowFor(Mode::kChangeTimeZoneDst)) {
            mOled.print((tz.getDstOffset().isZero()) ? "off" : "on ");
          }
          clearToEOL();
          break;
      #else
        case BasicZoneProcessor::kTypeBasic:
        case ExtendedZoneProcessor::kTypeExtended:
          // Print name of timezone
          if (shouldShowFor(Mode::kChangeTimeZoneName)) {
            tz.printShortTo(mOled);
          }
          clearToEOL();
          break;
      #endif
        default:
          mOled.print(F("<unknown>"));
          clearToEOL();
          break;
      }
    }

    void displaySettings() const {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displaySettings()"));
      }

      mOled.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeSettingsContrast)) {
        mOled.println(mClockInfo.contrastLevel);
      }
      clearToEOL();
    }

    /**
     * True if the display should actually show the data for the given mode.
     * If the clock is in "blinking" mode, then this will return false in
     * accordance with the mBlinkShowState.
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
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.contrastLevel != mClockInfo.contrastLevel) {
        uint8_t value = toOledContrastValue(mClockInfo.contrastLevel);
        mOled.setContrast(value);
      }
    }

    /** Convert [0, 9] contrast level for OLED to a [0, 255] value. */
    static uint8_t toOledContrastValue(uint8_t level) {
      if (level > 9) level = 9;
      return kOledContrastValues[level];
    }

    static const uint8_t kOledContrastValues[];

  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif
    SSD1306Ascii& mOled;
    ClockInfo mClockInfo;
    ClockInfo mPrevClockInfo;
};

#endif
