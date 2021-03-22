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
#include "StoredInfo.h"
#include "ClockInfo.h"
#include "RenderingInfo.h"

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
     *        overwrites the existing bits, therefore, displayData() does
     *        NOT need to clear the display
     */
    Presenter(
        ZoneManager& zoneManager,
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

    void setRenderingInfo(uint8_t mode, bool blinkShowState,
        const ClockInfo& clockInfo) {
      mRenderingInfo.mode = mode;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.clockInfo = clockInfo;
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
      mDisplay.setFont(fixed_bold10x15);
      //mDisplay.setFont(Adafruit5x7);
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
      return mRenderingInfo != mPrevRenderingInfo;
    }

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

    /** Update the display settings, e.g. brightness, backlight, etc. */
    void updateDisplaySettings() {
      ClockInfo &prevClockInfo = mPrevRenderingInfo.clockInfo;
      ClockInfo &clockInfo = mRenderingInfo.clockInfo;

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.backlightLevel != mRenderingInfo.backlightLevel) {
        uint16_t value = toLcdBacklightValue(clockInfo.backlightLevel);
        analogWrite(LCD_BACKLIGHT_PIN, value);
      }
      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.contrast != clockInfo.contrast) {
        mDisplay.setContrast(clockInfo.contrast);
      }
      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.bias != clockInfo.bias) {
        mDisplay.setBias(clockInfo.bias);
      }
    #else
      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.contrastLevel != clockInfo.contrastLevel) {
        uint8_t value = toOledContrastValue(clockInfo.contrastLevel);
        mDisplay.setContrast(value);
      }
      if (mPrevRenderingInfo.mode == MODE_UNKNOWN ||
          prevClockInfo.invertDisplay != clockInfo.invertDisplay) {
        mDisplay.invertDisplay(clockInfo.invertDisplay);
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

      switch (mRenderingInfo.mode) {
        case MODE_DATE_TIME:
        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          displayDateTimeMode();
          break;

        case MODE_TIME_ZONE:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          displayTimeZoneMode();
          break;

        case MODE_SETTINGS:
      #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
        case MODE_CHANGE_SETTINGS_BACKLIGHT:
        case MODE_CHANGE_SETTINGS_CONTRAST:
        case MODE_CHANGE_SETTINGS_BIAS:
      #else
        case MODE_CHANGE_SETTINGS_CONTRAST:
        case MODE_CHANGE_INVERT_DISPLAY:
      #endif
          displaySettingsMode();
          break;

        case MODE_ABOUT:
          displayAboutMode();
          break;
      }

      renderDisplay();
    }

    void displayDateTimeMode() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayDateTimeMode()"));
      }
      const ZonedDateTime& dateTime = mRenderingInfo.clockInfo.dateTime;
      if (dateTime.isError()) {
        mDisplay.println(F("<Error>"));
        return;
      }

      displayDate(dateTime);
      mDisplay.println();
      displayTime(dateTime);
      mDisplay.println();
      displayWeekday(dateTime);
    }

    void displayDate(const ZonedDateTime& dateTime) {
      if (shouldShowFor(MODE_CHANGE_YEAR)) {
        mDisplay.print(dateTime.year());
      } else {
        mDisplay.print("    ");
      }
      mDisplay.print('-');
      if (shouldShowFor(MODE_CHANGE_MONTH)) {
        printPad2To(mDisplay, dateTime.month(), '0');
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print('-');
      if (shouldShowFor(MODE_CHANGE_DAY)) {
        printPad2To(mDisplay, dateTime.day(), '0');
      } else{
        mDisplay.print("  ");
      }
      clearToEOL();
    }

    void displayTime(const ZonedDateTime& dateTime) {
      if (shouldShowFor(MODE_CHANGE_HOUR)) {
        uint8_t hour = dateTime.hour();
        if (mRenderingInfo.clockInfo.hourMode == ClockInfo::kTwelve) {
          if (hour == 0) {
            hour = 12;
          } else if (hour > 12) {
            hour -= 12;
          }
          printPad2To(mDisplay, hour, ' ');
        } else {
          printPad2To(mDisplay, hour, '0');
        }
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(MODE_CHANGE_MINUTE)) {
        printPad2To(mDisplay, dateTime.minute(), '0');
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(':');
      if (shouldShowFor(MODE_CHANGE_SECOND)) {
        printPad2To(mDisplay, dateTime.second(), '0');
      } else {
        mDisplay.print("  ");
      }
      mDisplay.print(' ');
      if (mRenderingInfo.clockInfo.hourMode == ClockInfo::kTwelve) {
        mDisplay.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
      clearToEOL();
    }

    void displayWeekday(const ZonedDateTime& dateTime) {
      mDisplay.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      clearToEOL();
    }

    void displayTimeZoneMode() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayTimeZoneMode()"));
      }

      // Don't use F() strings for short strings <= 4 characters. Seems to
      // increase flash memory, while saving only a few bytes of RAM.

      // Display the timezone using the TimeZoneData, not the dateTime, since
      // dateTime will contain a TimeZone, which points to the (singular)
      // Controller::mZoneProcessor, which will contain the old timeZone.
      TimeZone tz = mZoneManager.createForTimeZoneData(
          mRenderingInfo.clockInfo.timeZoneData);
      mDisplay.print("TZ: ");
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
      mDisplay.println();

      switch (tz.getType()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case TimeZone::kTypeManual:
          mDisplay.print("UTC");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_OFFSET)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mDisplay);
          }
          clearToEOL();
          mDisplay.println();

          mDisplay.print("DST: ");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_DST)) {
            mDisplay.print((tz.getDstOffset().isZero()) ? "off" : "on ");
          }
          clearToEOL();
          mDisplay.println();
          break;

      #else
        case BasicZoneProcessor::kTypeBasic:
        case ExtendedZoneProcessor::kTypeExtended:
          // Print name of timezone
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_NAME)) {
            tz.printShortTo(mDisplay);
          }
          clearToEOL();
          mDisplay.println();

          // Clear the DST: {on|off} line from a previous screen
          clearToEOL();
          mDisplay.println();
          break;
      #endif

        default:
          mDisplay.print(F("<unknown>"));
          clearToEOL();
          mDisplay.println();
          clearToEOL();
          mDisplay.println();
          break;
      }
    }

    void displaySettingsMode() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displaySettingsMode()"));
      }

      ClockInfo &clockInfo = mRenderingInfo.clockInfo;

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.print(F("Backlight:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_BACKLIGHT)) {
        mDisplay.println(clockInfo.backlightLevel);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_CONTRAST)) {
        mDisplay.println(clockInfo.contrast);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Bias:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_BIAS)) {
        mDisplay.println(clockInfo.bias);
      } else {
        mDisplay.println(' ');
      }

    #else
      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_CONTRAST)) {
        mDisplay.println(clockInfo.contrastLevel);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Invert:"));
      if (shouldShowFor(MODE_CHANGE_INVERT_DISPLAY)) {
        mDisplay.println(clockInfo.invertDisplay);
      } else {
        mDisplay.println(' ');
      }

    #endif
    }

    void displayAboutMode() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayAboutMode()"));
      }

      // Use F() macros for these longer strings. Seems to save both
      // flash memory and RAM.
      mDisplay.print(F("TZ: "));
      mDisplay.println(zonedb::kTzDatabaseVersion);
      mDisplay.println(F("AT: " ACE_TIME_VERSION_STRING));
      mDisplay.println(F("AB: " ACE_BUTTON_VERSION_STRING));
      mDisplay.println(F("AR: " ACE_ROUTINE_VERSION_STRING));
    }

  private:
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    static const uint16_t kLcdBacklightValues[];
  #else
    static const uint8_t kOledContrastValues[];
  #endif

    ZoneManager& mZoneManager;
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    Adafruit_PCD8544& mDisplay;
  #else
    SSD1306Ascii& mDisplay;
  #endif
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
    bool const mIsOverwriting;
};

#endif
