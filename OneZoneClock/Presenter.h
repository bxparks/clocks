#ifndef ONE_ZONE_CLOCK_PRESERNTER_H
#define ONE_ZONE_CLOCK_PRESERNTER_H

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
#include "Presenter.h"

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
        displayData();
      }

      mPrevRenderingInfo = mRenderingInfo;
    }

    void setRenderingInfo(uint8_t mode, bool suppressBlink, bool blinkShowState,
        const ClockInfo& clockInfo) {
  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mRenderingInfo.backlightLevel = clockInfo.backlightLevel;
      mRenderingInfo.contrast = clockInfo.contrast;
      mRenderingInfo.bias = clockInfo.bias;
  #else
      mRenderingInfo.contrastLevel = clockInfo.contrastLevel;
  #endif
      mRenderingInfo.invertDisplay = clockInfo.invertDisplay;

      mRenderingInfo.mode = mode;
      mRenderingInfo.suppressBlink = suppressBlink;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.hourMode = clockInfo.hourMode;
      mRenderingInfo.timeZoneData = clockInfo.timeZoneData;
      mRenderingInfo.dateTime = clockInfo.dateTime;
    }

  #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
    /** Set the LCD brightness value. */
    void setBrightness(uint16_t value) {
      analogWrite(LCD_BACKLIGHT_PIN, value);
    }

    /** Set the LCD contrast value. */
    void setContrast(uint8_t value) {
      mDisplay.setContrast(value);
    }

    /** Set the LCD bias. */
    void setBias(uint8_t value) {
      mDisplay.setBias(value);
    }

  #else
    /** Set the OLED contrast value. */
    void setContrast(uint8_t value) {
      mDisplay.setContrast(value);
    }
  #endif

    /** Set the inversion mode. */
    void invertDisplay(uint8_t value) {
      mDisplay.invertDisplay(value);
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
    /**
     * True if the display should actually show the data. If the clock is in
     * "blinking" mode, then this will return false in accordance with the
     * mBlinkShowState.
     */
    bool shouldShowFor(uint8_t mode) const {
      return mode != mRenderingInfo.mode
          || mRenderingInfo.suppressBlink
          || mRenderingInfo.blinkShowState;
    }

    /** The display needs to be cleared before rendering. */
    bool needsClear() const {
      return mRenderingInfo.mode != mPrevRenderingInfo.mode;
    }

    /** The display needs to be updated because something changed. */
    bool needsUpdate() const {
      return mRenderingInfo.mode != mPrevRenderingInfo.mode
          || mRenderingInfo.suppressBlink != mPrevRenderingInfo.suppressBlink
          || (!mRenderingInfo.suppressBlink
              && (mRenderingInfo.blinkShowState
                  != mPrevRenderingInfo.blinkShowState))
        #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
          || mRenderingInfo.backlightLevel != mPrevRenderingInfo.backlightLevel
          || mRenderingInfo.contrast != mPrevRenderingInfo.contrast
          || mRenderingInfo.bias != mPrevRenderingInfo.bias
        #else
          || mRenderingInfo.contrastLevel != mPrevRenderingInfo.contrastLevel
        #endif
          || mRenderingInfo.invertDisplay != mPrevRenderingInfo.invertDisplay
          || mRenderingInfo.hourMode != mPrevRenderingInfo.hourMode
          || mRenderingInfo.timeZoneData != mPrevRenderingInfo.timeZoneData
          || mRenderingInfo.dateTime != mPrevRenderingInfo.dateTime;
    }

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
      #endif
        case MODE_CHANGE_INVERT_DISPLAY:
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
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;
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
        if (mRenderingInfo.hourMode == ClockInfo::kTwelve) {
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
      if (mRenderingInfo.hourMode == ClockInfo::kTwelve) {
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
          mRenderingInfo.timeZoneData);
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

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD
      mDisplay.print(F("Backlight:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_BACKLIGHT)) {
        mDisplay.println(mRenderingInfo.backlightLevel);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_CONTRAST)) {
        mDisplay.println(mRenderingInfo.contrast);
      } else {
        mDisplay.println(' ');
      }

      mDisplay.print(F("Bias:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_BIAS)) {
        mDisplay.println(mRenderingInfo.bias);
      } else {
        mDisplay.println(' ');
      }
    #else
      mDisplay.print(F("Contrast:"));
      if (shouldShowFor(MODE_CHANGE_SETTINGS_CONTRAST)) {
        mDisplay.println(mRenderingInfo.contrastLevel);
      } else {
        mDisplay.println(' ');
      }
    #endif

      mDisplay.print(F("Invert:"));
      if (shouldShowFor(MODE_CHANGE_INVERT_DISPLAY)) {
        mDisplay.println(mRenderingInfo.invertDisplay);
      } else {
        mDisplay.println(' ');
      }
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
