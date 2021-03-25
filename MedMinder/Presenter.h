#ifndef MED_MINDER_PRESENTER_H
#define MED_MINDER_PRESENTER_H

#include "config.h"
#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include "RenderingInfo.h"
#include "ClockInfo.h"

using namespace ace_time;
using ace_common::printPad2To;

/** Class responsible for rendering the information to the OLED display. */
class Presenter {
  public:
    Presenter(SSD1306Ascii& oled):
        mOled(oled) {}

    /**
     * This should be called every 0.1s to support blinking mode and to avoid
     * noticeable drift against the RTC which has a 1 second resolution.
     */
    void display() {
      if (needsClear()) {
        clearDisplay();
      }
      if (needsUpdate()) {
        displayData();
      }

      mPrevRenderingInfo = mRenderingInfo;
    }

    void setRenderingInfo(
        Mode mode,
        bool blinkShowState,
        const ClockInfo& clockInfo
    ) {
      mRenderingInfo.mode = mode;
      mRenderingInfo.blinkShowState = blinkShowState;
      mRenderingInfo.timeZone = clockInfo.timeZone;
      mRenderingInfo.dateTime = clockInfo.dateTime;
      mRenderingInfo.timePeriod = clockInfo.medInterval;
    }

    void prepareToSleep() {
      mOled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
    }

    void wakeup() {
      mOled.ssd1306WriteCmd(SSD1306_DISPLAYON);
    }

  private:
    void clearDisplay() { mOled.clear(); }

    void displayData() const {
      mOled.home();

      switch (mRenderingInfo.mode) {
        case Mode::kViewMed:
          displayMed();
          break;

        case Mode::kViewDateTime:
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

        case Mode::kChangeYear:
        case Mode::kChangeMonth:
        case Mode::kChangeDay:
        case Mode::kChangeHour:
        case Mode::kChangeMinute:
        case Mode::kChangeSecond:
          displayDateTime();
          break;

        default:
          break;
      }
    }

    void displayMed() const {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayMed()"));
      }

      mOled.println(F("Med due"));
      if (mRenderingInfo.timePeriod.isError()) {
        mOled.print(F("<Overdue>"));
      } else {
        mRenderingInfo.timePeriod.printTo(mOled);
      }
      mOled.clearToEOL();
    }

    void displayAbout() const {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayAbout()"));
      }
      mOled.print(F("TZ:"));
      mOled.println(zonedb::kTzDatabaseVersion);
      mOled.println(F("AT:" ACE_TIME_VERSION_STRING));
      mOled.println(F("AB:" ACE_BUTTON_VERSION_STRING));
      mOled.println(F("AR:" ACE_ROUTINE_VERSION_STRING));
    }

    void displayChangeMed() const {
      mOled.println("Med intrvl");

      if (shouldShowFor(Mode::kChangeMedHour)) {
        printPad2To(mOled, mRenderingInfo.timePeriod.hour(), '0');
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(Mode::kChangeMedMinute)) {
        printPad2To(mOled, mRenderingInfo.timePeriod.minute(), '0');
      } else {
        mOled.print("  ");
      }

      mOled.clearToEOL();
    }

    void displayDateTime() const {
      if (ENABLE_SERIAL_DEBUG == 1) {
        SERIAL_PORT_MONITOR.println(F("displayDateTime()"));
      }

      displayDate();
      mOled.println();
      displayTime();
    }

    void displayDate() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

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
      mOled.clearToEOL();
    }

    void displayTime() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

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
      mOled.clearToEOL();
      mOled.println();

      // week day
      mOled.print(DateStrings().dayOfWeekLongString(dateTime.dayOfWeek()));
      mOled.clearToEOL();
    }

    void displayTimeZone() const {
      const auto& tz = mRenderingInfo.timeZone;

      // Display the timezone using the TimeZoneData, not the dateTime, since
      // dateTime will contain a TimeZone, which points to the (singular)
      // Controller::mZoneProcessor, which will contain the old timeZone.
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
      mOled.clearToEOL();
      mOled.println();

      switch (tz.getType()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case TimeZone::kTypeManual:
          mOled.println();
          mOled.print("UTC");
          if (shouldShowFor(Mode::kChangeTimeZoneOffset)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mOled);
          }
          mOled.clearToEOL();

          mOled.println();
          mOled.print("DST: ");
          if (shouldShowFor(Mode::kChangeTimeZoneDst)) {
            mOled.print((tz.getDstOffset().isZero()) ? "off" : "on ");
          }
          mOled.clearToEOL();
          break;
      #else
        case BasicZoneProcessor::kTypeBasic:
        case ExtendedZoneProcessor::kTypeExtended:
          // Print name of timezone
          if (shouldShowFor(Mode::kChangeTimeZoneName)) {
            tz.printShortTo(mOled);
          }
          mOled.clearToEOL();
          break;
      #endif
        default:
          mOled.print(F("<unknown>"));
          mOled.clearToEOL();
          break;
      }
    }

    /**
     * True if the display should actually show the data for the given mode.
     * If the clock is in "blinking" mode, then this will return false in
     * accordance with the mBlinkShowState.
     */
    bool shouldShowFor(Mode mode) const {
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

    SSD1306Ascii& mOled;
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
};

#endif
