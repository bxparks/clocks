#ifndef ONE_ZONE_CLOCK_PRESENTER_H
#define ONE_ZONE_CLOCK_PRESENTER_H

#include "config.h"
#include <Print.h>
#include <AceCommon.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
  #include <Adafruit_PCD8544.h>
#else
  #include <SSD1306AsciiWire.h>
#endif
#if ENABLE_LED_DISPLAY
  #include <AceTMI.h>
  #include <AceSegment.h>
  #include <AceSegmentWriter.h>
  using ace_segment::Tm1637Module;
  using ace_segment::ClockWriter;
  using ace_segment::LedModule;
  const uint8_t NUM_DIGITS = 4;
  using TmiInterface = ace_tmi::SimpleTmi1637Interface;
  using LedDisplay = Tm1637Module<TmiInterface, NUM_DIGITS>;
#endif
#include "StoredInfo.h"
#include "ClockInfo.h"
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
     * @param display either an OLED display or an LCD display
     * @param isOverwriting if true, printing a character to a display
     *        overwrites the existing bits, therefore, displayPrimary() does
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
      #if ENABLE_LED_DISPLAY
        LedDisplay& ledModule,
      #endif
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        Adafruit_PCD8544& display,
      #else
        SSD1306Ascii& display,
      #endif
        bool isOverwriting
    ) :
        mZoneManager(zoneManager),
      #if ENABLE_LED_DISPLAY
        mLedModule(ledModule),
      #endif
        mDisplay(display),
        mIsOverwriting(isOverwriting)
      {}

    void updateDisplay() {
      if (needsClear()) {
        clearDisplay();
      }

      if (needsUpdate()) {
        updateDisplaySettings();
        displayPrimary();
      #if ENABLE_LED_DISPLAY
        displayLedModule();
      #endif
      }

      mPrevClockInfo = mClockInfo;
    }

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

    /**
     * Set font and size.
     *
     *  0 - extra small size
     *  1 - normal 1X
     *  2 - double 2X
     */
    void setFont(uint8_t size) {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      // Use default font
      // if (size == 0) {
      //   mDisplay.setTextSize(8);
      // }
    #else
      if (size == 0) {
        mDisplay.setFont(Adafruit5x7);
        mDisplay.set1X();
      } else if (size == 1) {
        mDisplay.setFont(fixed_bold10x15);
        mDisplay.set1X();
      } else if (size == 2) {
        mDisplay.setFont(fixed_bold10x15);
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
    /** The display needs to be updated because something changed. */
    bool needsUpdate() const {
      return mClockInfo != mPrevClockInfo;
    }

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

    /** Update the display settings, e.g. brightness, backlight, etc. */
    void updateDisplaySettings() {
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.backlightLevel != clockInfo.backlightLevel) {
        uint16_t value = toLcdBacklightValue(clockInfo.backlightLevel);
        analogWrite(LCD_BACKLIGHT_PIN, value);
      }
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.contrast != clockInfo.contrast) {
        mDisplay.setContrast(clockInfo.contrast);
      }
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.bias != clockInfo.bias) {
        mDisplay.setBias(clockInfo.bias);
      }
    #else
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.contrastLevel != mClockInfo.contrastLevel) {
        uint8_t value = toOledContrastValue(mClockInfo.contrastLevel);
        mDisplay.setContrast(value);
      }

      // Invert the display if needed
      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.invertState != mClockInfo.invertState) {
        mDisplay.invertDisplay(mClockInfo.invertState);
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

    /** Render the primary OLED or LCD display. */
    void displayPrimary() {
      home();
      if (!mIsOverwriting) {
        clearDisplay();
      }
      setFont(1);

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
        case Mode::kChangeTimeZoneOffset:
        case Mode::kChangeTimeZoneDst:
      #else
        case Mode::kChangeTimeZoneName:
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
      #if ENABLE_LED_DISPLAY
        case Mode::kChangeSettingsLedOnOff:
        case Mode::kChangeSettingsLedBrightness:
      #endif
          displaySettingsMode();
          break;

      #if ENABLE_DHT22
        case Mode::kViewTemperature:
          displayTemperature();
          break;
      #endif

        case Mode::kViewSysclock:
          displaySystemClockMode();
          break;

        case Mode::kViewAbout:
          displayAboutMode();
          break;

        default:
          // do nothing
          break;
      }

      renderDisplay();
    }

  #if ENABLE_LED_DISPLAY
    /** Render the secondary LED module. */
    void displayLedModule() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayLedModule()"));
      }

      const ZonedDateTime& prevDateTime = mPrevClockInfo.dateTime;
      const ZonedDateTime& dateTime = clockInfo.dateTime;
      if (mPrevClockInfo.mode == Mode::kUnknown
          || prevDateTime != dateTime
          || mPrevClockInfo.hourMode != clockInfo.hourMode) {
        uint8_t hour = (clockInfo.hourMode == ClockInfo::kTwelve)
            ? toTwelveHour(dateTime.hour())
            : dateTime.hour();
        uint8_t minute = dateTime.minute();
        ClockWriter<LedModule> clockWriter(mLedModule);
        if (clockInfo.hourMode == ClockInfo::kTwelve) {
          clockWriter.writeHourMinute12(hour, minute);
        } else {
          clockWriter.writeHourMinute24(hour, minute);
        }
      }

      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.ledOnOff != clockInfo.ledOnOff) {
        mLedModule.setDisplayOn(clockInfo.ledOnOff);
      }

      if (mPrevClockInfo.mode == Mode::kUnknown
          || mPrevClockInfo.ledBrightness != clockInfo.ledBrightness) {
        mLedModule.setBrightness(clockInfo.ledBrightness);
      }

      if (mPrevClockInfo.mode == Mode::kUnknown
          || mLedModule.isFlushRequired()) {
        if (ENABLE_SERIAL_DEBUG >= 2) {
          SERIAL_PORT_MONITOR.println(F("displayLedModule(): flush()"));
        }
        mLedModule.flush();
      }
    }
  #endif

    void displayDateTimeMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayDateTimeMode()"));
      }
      const ZonedDateTime& dateTime = mClockInfo.dateTime;
      if (dateTime.isError()) {
        mDisplay.println(F("<Error>"));
        return;
      }

      displayDate(dateTime);
      clearToEOL();
      displayTime(dateTime);
      clearToEOL();
      displayWeekday(dateTime);
      clearToEOL();
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
    }

    void displayTime(const ZonedDateTime& dateTime) {
      if (shouldShowFor(Mode::kChangeHour)) {
        uint8_t hour = dateTime.hour();
        if (mClockInfo.hourMode == ClockInfo::kTwelve) {
          hour = toTwelveHour(hour);
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
      mDisplay.print(' ');
      if (mClockInfo.hourMode == ClockInfo::kTwelve) {
        mDisplay.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
    }

    void displayWeekday(const ZonedDateTime& dateTime) {
      mDisplay.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
    }

    void displayTimeZoneMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayTimeZoneMode()"));
      }

      // Display the timezone using the TimeZoneData, not the dateTime, since
      // dateTime will point to the old timeZone.
      TimeZone tz = mZoneManager.createForTimeZoneData(mClockInfo.timeZoneData);
      mDisplay.print("TZ:");
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
      mDisplay.print(typeString);
      clearToEOL();

      switch (tz.getType()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case TimeZone::kTypeManual:
          mDisplay.print("UTC");
          if (shouldShowFor(Mode::kChangeTimeZoneOffset)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mDisplay);
          }
          clearToEOL();

          mDisplay.print("DST: ");
          if (shouldShowFor(Mode::kChangeTimeZoneDst)) {
            mDisplay.print((tz.getDstOffset().isZero()) ? "off" : "on ");
          }
          clearToEOL();
          break;

      #else
        case BasicZoneProcessor::kTypeBasic:
        case ExtendedZoneProcessor::kTypeExtended:
          // Print name of timezone
          if (shouldShowFor(Mode::kChangeTimeZoneName)) {
            tz.printShortTo(mDisplay);
          }
          clearToEOL();

          // Clear the DST: {on|off} line from a previous screen
          clearToEOL();
          break;
      #endif

        default:
          mDisplay.print(F("<unknown>"));
          clearToEOL();
          clearToEOL();
          break;
      }
    }

    void displaySettingsMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displaySettingsMode()"));
      }

      ClockInfo &clockInfo = mClockInfo;

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.print(F("Backlight:"));
      if (shouldShowFor(Mode::kChangeSettingsBacklight)) {
        mDisplay.println(clockInfo.backlightLevel);
      } else {
        clearToEOL();
      }

      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeSettingsContrast)) {
        mDisplay.println(clockInfo.contrast);
      } else {
        clearToEOL();
      }

      mDisplay.print(F("Bias:"));
      if (shouldShowFor(Mode::kChangeSettingsBias)) {
        mDisplay.println(clockInfo.bias);
      } else {
        clearToEOL();
      }

    #else
      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(Mode::kChangeSettingsContrast)) {
        mDisplay.println(clockInfo.contrastLevel);
      } else {
        clearToEOL();
      }

      mDisplay.print(F("Invert:"));
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
        mDisplay.print(statusString);
      }
      clearToEOL();
    #endif

    #if ENABLE_LED_DISPLAY
      mDisplay.print(F("LED:"));
      if (shouldShowFor(Mode::kChangeSettingsLedOnOff)) {
        mDisplay.print(clockInfo.ledOnOff ? "on" : "off");
      }
      clearToEOL();

      mDisplay.print(F("LED Lvl:"));
      if (shouldShowFor(Mode::kChangeSettingsLedBrightness)) {
        mDisplay.print(clockInfo.ledBrightness);
      }
      clearToEOL();
    #endif

    }

  #if ENABLE_DHT22
    void displayTemperature() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displayTemperature()"));
      }

      ClockInfo &clockInfo = mClockInfo;

      mDisplay.print(F("Temp:"));
      mDisplay.print(clockInfo.temperatureC, 1);
      mDisplay.print('C');
      clearToEOL();

      mDisplay.print(F("Temp:"));
      mDisplay.print(clockInfo.temperatureC * 9 / 5 + 32, 1);
      mDisplay.print('F');
      clearToEOL();

      mDisplay.print(F("Humi:"));
      mDisplay.print(clockInfo.humidity, 1);
      mDisplay.print('%');
      clearToEOL();
    }
  #endif

    void displaySystemClockMode() {
      if (ENABLE_SERIAL_DEBUG >= 2) {
        SERIAL_PORT_MONITOR.println(F("displaySystemClockMode()"));
      }

      ClockInfo &clockInfo = mClockInfo;

    #if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
      mDisplay.print(F("SClkLoop:"));
    #else
      mDisplay.print(F("SClkCortn:"));
    #endif
      mDisplay.print(clockInfo.syncStatusCode);
      clearToEOL();

      // Print the prev sync as a negative
      mDisplay.print(F("<:"));
      TimePeriod prevSync = clockInfo.prevSync;
      prevSync.sign(-prevSync.sign());
      displayTimePeriodHMS(prevSync);
      clearToEOL();

      mDisplay.print(F(">:"));
      displayTimePeriodHMS(clockInfo.nextSync);
      clearToEOL();

      mDisplay.print(F("S:"));
      displayTimePeriodHMS(clockInfo.clockSkew);
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
      setFont(0);
      mDisplay.println(F("OZC: " ONE_ZONE_CLOCK_VERSION_STRING));
      mDisplay.print(F("TZDB:"));
      mDisplay.println(zonedb::kTzDatabaseVersion);
      mDisplay.println(F("ATim:" ACE_TIME_VERSION_STRING));
      mDisplay.println(F("ABut:" ACE_BUTTON_VERSION_STRING));
      mDisplay.println(F("ARou:" ACE_ROUTINE_VERSION_STRING));
      mDisplay.println(F("ACom:" ACE_COMMON_VERSION_STRING));
    #if ENABLE_LED_DISPLAY
      mDisplay.println(F("ASeg:" ACE_SEGMENT_VERSION_STRING));
      mDisplay.println(F("ASgW:" ACE_SEGMENT_WRITER_VERSION_STRING));
      mDisplay.println(F("ATMI:" ACE_TMI_VERSION_STRING));
    #endif
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
  #if ENABLE_LED_DISPLAY
    LedDisplay& mLedModule;
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
