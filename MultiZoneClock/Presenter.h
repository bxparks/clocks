#ifndef MULTI_ZONE_CLOCK_PRESENTER_H
#define MULTI_ZONE_CLOCK_PRESENTER_H

#include "config.h"
#include <Print.h>
#include <AceCommon.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASICDB \
    || TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDEDDB
  #include <AceTimePro.h> // BasicDbZoneProcessor, ExtendedDbZoneProcessor
#endif
#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
  #include <Adafruit_PCD8544.h>
#else
  #include <SSD1306AsciiWire.h>
#endif
#include "StoredInfo.h"
#include "ClockInfo.h"

using namespace ace_time;
using ace_common::printPad2To;

/**
 * An abstraction layer around an LCD display or OLED display, both of whom
 * implement the `Print` interface.
 *    * Knows what content to render for various modes.
 *    * Handles blinking by printing spaces when appropriate.
 *    * Keeps track of current and previous UI states and updates only when
 *      something has changed.
 *
 * The SSD1306 OLED display uses the SSD1306Ascii driver. This driver
 * overwrites the background pixels when writing a single character, so we
 * don't need to clear the entire display before rendering a new version of the
 * frame. However, we must make sure that we clear the rest of the line with a
 * clearToEOL() to prevent unwanted clutter from the prior frame.
 *
 * The PCD8554 LCD display (aka Nokia 5110) uses Adafruit's driver. Unlike the
 * SSD1306 driver, the Adafruit driver does NOT overwrite the existing
 * background bits of a character (i.e. bit are only turns on, not turned off.)
 * We can either use a double-buffered canvas (did not want to take the time
 * right now to figure that out), or we can erase the entire screen before
 * rendering each frame. Normally, this would cause a flicker in the display.
 * However, it seems like the LCD pixels have so much latency that I don't see
 * any flickering at all. It works, so I'll just keep it like that for now.
 */
class Presenter {
  public:
    /**
     * Constructor.
     * @param zoneManager an instance of ZoneManager
     * @param display either an OLED display or an LCD display, implementing
     *        the Print interface
     * @param isOverwriting if true, printing a character to a display
     *        overwrites the existing bits, therefore, displayData() does
     *        NOT need to clear the display
     */
    Presenter(
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        ManualZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        BasicZoneManager& zoneManager,
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        ExtendedZoneManager& zoneManager,
      #endif
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        Adafruit_PCD8544& display,
      #else
        SSD1306Ascii& display,
      #endif
        bool isOverwriting
    ) :
        mZoneManager(zoneManager),
        mDisplay(display),
        mIsOverwriting(isOverwriting)
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

    /**
     * The Controller uses this method to pass mode and time information to the
     * Presenter.
     */
    void setClockInfo(const ClockInfo& clockInfo) {
      mClockInfo = clockInfo;
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

  // These methods abtracts away the differences between a PCD8544 LCD and
  // and SSD1306 OLED displays
  private:
    void clearDisplay() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.clearDisplay();
    #else
      mDisplay.clear();
    #endif
    }

    void home() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.setCursor(0, 0);
    #else
      mDisplay.home();
    #endif
    }

    void renderDisplay() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.display();
    #else
      // OLED display updates immediately upon println(), no need to call
      // anything else.
    #endif
    }

    void setFont() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      // Use default font
    #else
      //mDisplay.setFont(fixed_bold10x15);
      mDisplay.setFont(Adafruit5x7);
    #endif
    }

    void setSize(uint8_t size) {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.setTextSize(size);
    #else
      if (size == 1) {
        mDisplay.set1X();
      } else if (size == 2) {
        mDisplay.set2X();
      }
    #endif
    }

    void clearToEOL() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      // Not available, and not needed since the entire screen is clear
      // on re-render.
    #else
      mDisplay.clearToEOL();
      mDisplay.println();
    #endif
    }

    /* Set the cursor just under the AM/PM indicator */
    void setCursorUnderAmPm() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.setCursor(60 /*x*/, 8 /*y*/);
    #else
      mDisplay.setCursor(60 /*x pixels*/, 1 /*y row number*/);
    #endif
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
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.backlightLevel != mClockInfo.backlightLevel) {
        uint16_t value = toLcdBacklightValue(mClockInfo.backlightLevel);
        analogWrite(LCD_BACKLIGHT_PIN, value);
      }
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.contrast != mClockInfo.contrast) {
        mDisplay.setContrast(mClockInfo.contrast);
      }
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.bias != mClockInfo.bias) {
        mDisplay.setBias(mClockInfo.bias);
      }
    #else
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.contrastLevel != mClockInfo.contrastLevel) {
        uint8_t value = toOledContrastValue(mClockInfo.contrastLevel);
        mDisplay.setContrast(value);
      }
      if (mPrevClockInfo.mode == Mode::kUnknown ||
          mPrevClockInfo.invertDisplay != mClockInfo.invertDisplay) {
        mDisplay.invertDisplay(mClockInfo.invertDisplay);
      }
    #endif
    }

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    /**
     * Return the PWM value for the given LCD backlight brightness level. Since
     * the backlight is driven LOW (low values of PWM means brighter light),
     * subtract from 1024.
     *
     * TODO: This works for chips using 10-bit PWM, e.g. ESP8266. Needs to
     * support 8-bit and 12-bit PWM chips.
     *
     * @param level brightness level in the range of [0, 9]
     */
    static uint16_t toLcdBacklightValue(uint8_t level) {
      if (level > 9) level = 9;
      return 1023 - kLcdBacklightValues[level];
    }

  #else

    /** Convert [0, 9] contrast level for OLED to a [0, 255] value. */
    static uint8_t toOledContrastValue(uint8_t level) {
      if (level > 9) level = 9;
      return kOledContrastValues[level];
    }

  #endif

    void displayData() {
      home();
      if (!mIsOverwriting) {
        clearDisplay();
      }
      setFont();

      switch (mClockInfo.mode) {
        case Mode::kViewDateTime:
        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          displayDateTimeMode();
          break;

        case Mode::kViewTimeZone:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case Mode::kChangeTimeZone0Offset:
        case Mode::kChangeTimeZone0Dst:
        case Mode::kChangeTimeZone1Offset:
        case Mode::kChangeTimeZone1Dst:
        case Mode::kChangeTimeZone2Offset:
        case Mode::kChangeTimeZone2Dst:
        case Mode::kChangeTimeZone3Offset:
        case Mode::kChangeTimeZone3Dst:
      #else
        case Mode::kChangeTimeZone0Name:
        case Mode::kChangeTimeZone1Name:
        case Mode::kChangeTimeZone2Name:
        case Mode::kChangeTimeZone3Name:
      #endif
          displayTimeZoneMode();
          break;

        case Mode::kViewSettings:
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case Mode::kChangeSettingsBacklight:
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeSettingsBias:
      #else
        case Mode::kChangeSettingsContrast:
        case Mode::kChangeInvertDisplay:
      #endif
          displaySettingsMode();
          break;

        case Mode::kViewSysclock:
          displaySystemClockMode();
          break;

        case Mode::kViewAbout:
          displayAboutMode();
          break;

        default:
          break;
      }

      renderDisplay();
    }

    void displayDateTimeMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayDateTimeMode()"));
      }

      const ZonedDateTime& dateTime = mClockInfo.dateTime;
      if (dateTime.isError()) {
        mDisplay.println(F("<Error>"));
        return;
      }

      // Display primary time in large font.
      displayLargeTime(dateTime);

      // Display alternates in normal font.
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.zones[1]);
      ZonedDateTime altDateTime = dateTime.convertToTimeZone(tz);
      displayDateChangeIndicator(dateTime, altDateTime);
      displayTimeWithAbbrev(altDateTime);

      tz = mZoneManager.createForTimeZoneData(mClockInfo.zones[2]);
      altDateTime = dateTime.convertToTimeZone(tz);
      displayDateChangeIndicator(dateTime, altDateTime);
      displayTimeWithAbbrev(altDateTime);

      tz = mZoneManager.createForTimeZoneData(mClockInfo.zones[3]);
      altDateTime = dateTime.convertToTimeZone(tz);
      displayDateChangeIndicator(dateTime, altDateTime);
      displayTimeWithAbbrev(altDateTime);

      displayHumanDate(dateTime);
    }

    // Print a '>' or '<' if the date of the target time is different.
    void displayDateChangeIndicator(
        const ZonedDateTime& current,
        const ZonedDateTime& target) {
      const LocalDate& currentDate = current.localDateTime().localDate();
      const LocalDate& targetDate = target.localDateTime().localDate();
      int8_t compare = targetDate.compareTo(currentDate);
      char indicator;
      if (compare < 0) {
        indicator = '<';
      } else if (compare > 0) {
        indicator = '>';
      } else {
        indicator = ' ';
      }
      mDisplay.print(indicator);
    }

    static uint8_t convert24To12(uint8_t hour) {
      if (hour == 0) {
        return 12;
      } else if (hour > 12) {
        hour -= 12;
      }
      return hour;
    }

    void displayTime(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        uint8_t hour = dateTime.hour();
        if (mClockInfo.hourMode == ClockInfo::kTwelve) {
          hour = convert24To12(hour);
          printPad2To(mDisplay, hour, ' ');
        } else {
          printPad2To(mDisplay, hour, '0');
        }
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(Mode::kChangeMinute)) {
        printPad2To(mDisplay, dateTime.minute(), '0');
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(Mode::kChangeSecond)) {
        printPad2To(mDisplay, dateTime.second(), '0');
      } else {
        mDisplay.print("  ");
      }

      // With monospaced fonts, this extra space does not seem necessary.
      //mDisplay.print(' ');

      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        mDisplay.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
      clearToEOL();
    }

    void displayLargeTime(const ZonedDateTime& dateTime) {
      setSize(2);
      if (shouldShowFor(Mode::kChangeHour)) {
        uint8_t hour = dateTime.hour();
        if (mClockInfo.hourMode == ClockInfo::kTwelve) {
          hour = convert24To12(hour);
          printPad2To(mDisplay, hour, ' ');
        } else {
          printPad2To(mDisplay, hour, '0');
        }
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(Mode::kChangeMinute)) {
        printPad2To(mDisplay, dateTime.minute(), '0');
      } else {
        mDisplay.print("  ");
      }

      // With large font, this space looks too wide. We can use the extra space
      // to display both the PM and TZ Abbreviation.
      //mDisplay.print(' ');

      // AM/PM
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        setSize(1);
        mDisplay.print((dateTime.hour() < 12) ? "AM" : "PM");
        mDisplay.clearToEOL();
      }

      // TimeZone
      setSize(1);
      setCursorUnderAmPm();
      displayTimeZoneAbbrev(dateTime);
      clearToEOL();
    }

    void displayTimeWithAbbrev(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        uint8_t hour = dateTime.hour();
        if (mClockInfo.hourMode == ClockInfo::kTwelve) {
          hour = convert24To12(hour);
          printPad2To(mDisplay, hour, ' ');
        } else {
          printPad2To(mDisplay, hour, '0');
        }
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(Mode::kChangeMinute)) {
        printPad2To(mDisplay, dateTime.minute(), '0');
      } else {
        mDisplay.print("  ");
      }

      // AM/PM
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        // With monospaced fonts, this extra space does not seem necessary.
        //mDisplay.print(' ');
        mDisplay.print((dateTime.hour() < 12) ? "A" : "P");
      }

      mDisplay.print(' ');
      displayTimeZoneAbbrev(dateTime);
      clearToEOL();
    }

    // Timezone abbreviation. For Manual timezone, the abbreviation is just
    // 'STD' or 'DST' which is not useful when multiple time zones are
    // displayed. Instead, print out the short name, which will be "+hh:mm"
    // for a manual timezone.
    void displayTimeZoneAbbrev(const ZonedDateTime& dateTime) {
      const TimeZone& tz = dateTime.timeZone();
      if (tz.getType() == TimeZone::kTypeManual) {
        tz.printShortTo(mDisplay);
      } else {
        ZonedExtra ze = ZonedExtra::forEpochSeconds(
            dateTime.toEpochSeconds(), tz);
        mDisplay.print(ze.abbrev());
      }
    }

    void displayDate(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeYear)) {
        mDisplay.print(dateTime.year());
      } else {
        mDisplay.print("    ");
      }
      mDisplay.print('-');
      if (shouldShowFor(Mode::kChangeMonth)) {
        printPad2To(mDisplay, dateTime.month(), '0');
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print('-');
      if (shouldShowFor(Mode::kChangeDay)) {
        printPad2To(mDisplay, dateTime.day(), '0');
      } else{
        mDisplay.print("  ");
      }
      clearToEOL();
    }

    void displayHumanDate(const ZonedDateTime& dateTime) {
      mDisplay.print(DateStrings().dayOfWeekShortString(dateTime.dayOfWeek()));
      mDisplay.print(' ');

      if (shouldShowFor(Mode::kChangeDay)) {
        printPad2To(mDisplay, dateTime.day(), '0');
      } else{
        mDisplay.print("  ");
      }

      if (shouldShowFor(Mode::kChangeMonth)) {
        mDisplay.print(DateStrings().monthShortString(dateTime.month()));
      } else {
        mDisplay.print("   ");
      }

      if (shouldShowFor(Mode::kChangeYear)) {
        mDisplay.print(dateTime.year());
      } else {
        mDisplay.print("    ");
      }

      clearToEOL();
    }

    void displayWeekday(const ZonedDateTime& dateTime) {
      mDisplay.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      clearToEOL();
    }

    void displayTimeZoneMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayTimeZoneMode()"));
      }

      displayTimeZoneType();

      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        displayManualTimeZone(0, mClockInfo.zones[0],
            Mode::kChangeTimeZone0Offset, Mode::kChangeTimeZone0Dst);
        displayManualTimeZone(1, mClockInfo.zones[1],
            Mode::kChangeTimeZone1Offset, Mode::kChangeTimeZone1Dst);
        displayManualTimeZone(2, mClockInfo.zones[2],
            Mode::kChangeTimeZone2Offset, Mode::kChangeTimeZone2Dst);
        displayManualTimeZone(3, mClockInfo.zones[3],
            Mode::kChangeTimeZone3Offset, Mode::kChangeTimeZone3Dst);
      #else
        displayAutoTimeZone(0, mClockInfo.zones[0], Mode::kChangeTimeZone0Name);
        displayAutoTimeZone(1, mClockInfo.zones[1], Mode::kChangeTimeZone1Name);
        displayAutoTimeZone(2, mClockInfo.zones[2], Mode::kChangeTimeZone2Name);
        displayAutoTimeZone(3, mClockInfo.zones[3], Mode::kChangeTimeZone3Name);
      #endif
    }

    // Don't use F() strings for short strings <= 4 characters. Seems to
    // increase flash memory, while saving only a few bytes of RAM.
    void displayTimeZoneType() {
      mDisplay.print("TZ:");
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        mDisplay.print(F("manual"));
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
        mDisplay.print(F("basic"));
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
        mDisplay.print(F("extended"));
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASICDB
        mDisplay.print(F("basicdb"));
      #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDEDDB
        mDisplay.print(F("extendeddb"));
      #else
        mDisplay.print(F("unknown"));
      #endif
      clearToEOL();
    }

    void displayManualTimeZone(uint8_t pos, const TimeZoneData& zone,
        Mode changeOffsetMode, Mode changeDstMode) {
      mDisplay.print(pos);
      mDisplay.print(':');

      TimeZone tz = mZoneManager.createForTimeZoneData(zone);
      switch (tz.getType()) {
        case TimeZone::kTypeManual:
          mDisplay.print("UTC");
          if (shouldShowFor(changeOffsetMode)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mDisplay);
          } else {
            mDisplay.print("      ");
          }

          mDisplay.print("; DST: ");
          if (shouldShowFor(changeDstMode)) {
            mDisplay.print((tz.getDstOffset().isZero()) ? "off" : "on ");
          }
          clearToEOL();
          break;

        default:
          mDisplay.print(F("<unknown>"));
          clearToEOL();
          break;
      }
      mDisplay.println();
    }

    void displayAutoTimeZone(uint8_t pos, const TimeZoneData& zone,
        Mode changeTimeZoneNameMode) {
      mDisplay.print(pos);
      mDisplay.print(':');

      TimeZone tz = mZoneManager.createForTimeZoneData(zone);
      switch (tz.getType()) {
        case BasicZoneProcessor::kTypeBasic:
        case ExtendedZoneProcessor::kTypeExtended:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASICDB \
          || TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDEDDB
        case BasicDbZoneProcessor::kTypeBasicDb:
        case ExtendedDbZoneProcessor::kTypeExtendedDb:
      #endif
          if (shouldShowFor(changeTimeZoneNameMode)) {
            tz.printShortTo(mDisplay);
          }
          clearToEOL();
          break;

        default:
          mDisplay.print(F("<unknown>"));
          clearToEOL();
          break;
      }
    }

    void displaySettingsMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displaySettingsMode()"));
      }

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.print(F("Backlight:"));
      if (shouldShowFor(Mode::kChangeSettingsBacklight)) {
        mDisplay.println(mClockInfo.backlightLevel);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeSettingsContrast)) {
        mDisplay.println(mClockInfo.contrast);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Bias:"));
      if (shouldShowFor(Mode::kChangeSettingsBias)) {
        mDisplay.println(mClockInfo.bias);
      } else {
        mDisplay.println(' ');
      }

    #else
      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeSettingsContrast)) {
        mDisplay.println(mClockInfo.contrastLevel);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Invert:"));
      if (shouldShowFor(Mode::kChangeInvertDisplay)) {
        mDisplay.println(mClockInfo.invertDisplay);
      } else {
        mDisplay.println(' ');
      }
    #endif
    }

    void displaySystemClockMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displaySystemClockMode()"));
      }

    #if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
      mDisplay.print(F("SClkLoop:"));
    #else
      mDisplay.print(F("SClkCortn:"));
    #endif
      mDisplay.print(mClockInfo.syncStatusCode);
      clearToEOL();

      // Print the prev sync as a negative
      mDisplay.print(F("<:"));
      TimePeriod prevSync = mClockInfo.prevSync;
      prevSync.sign(-prevSync.sign());
      displayTimePeriodHMS(prevSync);
      clearToEOL();

      mDisplay.print(F(">:"));
      displayTimePeriodHMS(mClockInfo.nextSync);
      clearToEOL();

      mDisplay.print(F("S:"));
      displayTimePeriodHMS(mClockInfo.clockSkew);
      clearToEOL();
    }

    void displayTimePeriodHMS(const TimePeriod& tp) {
      tp.printTo(mDisplay);
    }

    void displayAboutMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayAboutMode()"));
      }

      // Use F() macros for these longer strings. Seems to save both
      // flash memory and RAM.
      mDisplay.println(F("MZC: " MULTI_ZONE_CLOCK_VERSION_STRING));
      mDisplay.print(F("TZDB:"));
      mDisplay.println(zonedb::kTzDatabaseVersion);
      mDisplay.println(F("ATim:" ACE_TIME_VERSION_STRING));
      mDisplay.println(F("ABut:" ACE_BUTTON_VERSION_STRING));
      mDisplay.println(F("ARou:" ACE_ROUTINE_VERSION_STRING));
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.println(F("ACom:" ACE_COMMON_VERSION_STRING));
    #endif
    }

  private:
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    static const uint16_t kLcdBacklightValues[];
  #else
    static const uint8_t kOledContrastValues[];
  #endif

  #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
    ManualZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
    BasicZoneManager& mZoneManager;
  #elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
    ExtendedZoneManager& mZoneManager;
  #endif
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    Adafruit_PCD8544& mDisplay;
  #else
    SSD1306Ascii& mDisplay;
  #endif

    ClockInfo mClockInfo;
    ClockInfo mPrevClockInfo;
    bool const mIsOverwriting;
};

#endif
