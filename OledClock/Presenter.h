#ifndef OLED_CLOCK_PRESERNTER_H
#define OLED_CLOCK_PRESERNTER_H

#include <Print.h>
#include <AceCommon.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include "config.h"
#include "StoredInfo.h"
#include "ClockInfo.h"
#include "RenderingInfo.h"
#include "Presenter.h"

using namespace ace_time;
using ace_common::printPad2To;

class Presenter {
  public:
    /**
     * Constructor.
     * @param display either an OLED display or an LCD display
     * @param isOverwriting if true, printing a character to a display
     *        overwrites the existing bits, therefore, displayData() does
     *        NOT need to clear the display
     */
    Presenter(Print& display, bool isOverwriting):
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
      mRenderingInfo.mode = mode;
      mRenderingInfo.suppressBlink = suppressBlink;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.hourMode = clockInfo.hourMode;
      mRenderingInfo.timeZone = clockInfo.timeZone;
      mRenderingInfo.dateTime = clockInfo.dateTime;
    }

  private:
    // Disable copy-constructor and assignment operator
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

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
          || mRenderingInfo.hourMode != mPrevRenderingInfo.hourMode
          || mRenderingInfo.timeZone != mPrevRenderingInfo.timeZone
          || mRenderingInfo.dateTime != mPrevRenderingInfo.dateTime;
    }

    virtual void clearDisplay() = 0;

    virtual void home() = 0;

    virtual void renderDisplay() = 0;

    virtual void setFont() = 0;

    virtual void clearToEOL() = 0;

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
          displayDateTime();
          break;

        case MODE_TIME_ZONE:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          displayTimeZone();
          break;

        case MODE_ABOUT:
          displayAbout();
          break;
      }

      renderDisplay();
    }

    void displayDateTime() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayDateTime()"));
      }
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;
      if (dateTime.isError()) {
        mDisplay.println(F("<Error>"));
        return;
      }

      displayTime(dateTime);
      mDisplay.println();
      displayDate(dateTime);
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
        if (mRenderingInfo.hourMode == StoredInfo::kTwelve) {
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
      if (mRenderingInfo.hourMode == StoredInfo::kTwelve) {
        mDisplay.print((dateTime.hour() < 12) ? "AM" : "PM");
      }
      clearToEOL();
    }

    void displayWeekday(const ZonedDateTime& dateTime) {
      mDisplay.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      clearToEOL();
    }

    void displayTimeZone() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayTimeZone()"));
      }

      // Don't use F() strings for short strings <= 4 characters. Seems to
      // increase flash memory, while saving only a few bytes of RAM.

      // Display the timezone using the TimeZoneData, not the dateTime, since
      // dateTime will contain a TimeZone, which points to the (singular)
      // Controller::mZoneProcessor, which will contain the old timeZone.
      auto& tz = mRenderingInfo.timeZone;
      mDisplay.print("TZ: ");
      const __FlashStringHelper* typeString;
      switch (tz.getType()) {
        case TimeZone::kTypeManual:
          typeString = F("manual");
          break;
        case TimeZone::kTypeBasic:
        case TimeZone::kTypeBasicManaged:
          typeString = F("basic");
          break;
        case TimeZone::kTypeExtended:
        case TimeZone::kTypeExtendedManaged:
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
          mDisplay.println();
          mDisplay.print("UTC");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_OFFSET)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mDisplay);
          }
          clearToEOL();

          mDisplay.println();
          mDisplay.print("DST: ");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_DST)) {
            mDisplay.print((tz.getDstOffset().isZero()) ? "off " : "on");
          }
          clearToEOL();
          break;

      #else
        case TimeZone::kTypeBasic:
        case TimeZone::kTypeExtended:
        case TimeZone::kTypeBasicManaged:
        case TimeZone::kTypeExtendedManaged:
          // Print name of timezone
          mDisplay.println();
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_NAME)) {
            tz.printShortTo(mDisplay);
          }
          clearToEOL();

          // Clear the DST: {on|off} line from a previous screen
          mDisplay.println();
          clearToEOL();
          break;
      #endif

        default:
          mDisplay.println();
          mDisplay.print(F("<unknown>"));
          clearToEOL();
          mDisplay.println();
          clearToEOL();
          break;
      }
    }

    void displayAbout() {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayAbout()"));
      }

      // Use F() macros for these longer strings. Seems to save both
      // flash memory and RAM.
      mDisplay.print(F("TZ: "));
      mDisplay.println(zonedb::kTzDatabaseVersion);
      mDisplay.println(F("AT: " ACE_TIME_VERSION_STRING));
      mDisplay.println(F("AB: " ACE_BUTTON_VERSION_STRING));
      mDisplay.print(F("AR: " ACE_ROUTINE_VERSION_STRING));
    }

    Print& mDisplay;
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
    bool mIsOverwriting;
};

#endif
