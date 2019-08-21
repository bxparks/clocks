#ifndef MED_MINDER_PRESENTER_H
#define MED_MINDER_PRESENTER_H

#include <SSD1306AsciiWire.h>
#include <AceTime.h>
#include <ace_time/common/DateStrings.h>
#include "RenderingInfo.h"
#include "ClockInfo.h"

using namespace ace_time;
using namespace ace_time::common;

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

    /** Clear the display. */
    void clearDisplay() { mOled.clear(); }

    void setRenderingInfo(uint8_t mode, bool suppressBlink, bool blinkShowState,
        const ClockInfo& clockInfo) {
      mRenderingInfo.mode = mode;
      mRenderingInfo.suppressBlink = suppressBlink;
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
    void displayData() const {
      mOled.home();

      switch (mRenderingInfo.mode) {
        case MODE_VIEW_MED:
          displayMed();
          break;

        case MODE_VIEW_DATE_TIME:
          displayDateTime();
          break;

        case MODE_VIEW_ABOUT:
          displayAbout();
          break;

        case MODE_VIEW_TIME_ZONE:
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case MODE_CHANGE_TIME_ZONE_OFFSET:
        case MODE_CHANGE_TIME_ZONE_DST:
      #else
        case MODE_CHANGE_TIME_ZONE_NAME:
      #endif
          displayTimeZone();
          break;

        case MODE_CHANGE_MED_HOUR:
        case MODE_CHANGE_MED_MINUTE:
          displayChangeMed();
          break;

        case MODE_CHANGE_YEAR:
        case MODE_CHANGE_MONTH:
        case MODE_CHANGE_DAY:
        case MODE_CHANGE_HOUR:
        case MODE_CHANGE_MINUTE:
        case MODE_CHANGE_SECOND:
          displayDateTime();
          break;
      }
    }

    void displayMed() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayMed()"));
    #endif
      mOled.setFont(fixed_bold10x15);
      mOled.set1X();

      mOled.println(F("Med due"));
      mRenderingInfo.timePeriod.printTo(mOled);
      mOled.clearToEOL();
    }

    void displayAbout() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayAbout()"));
    #endif
      mOled.setFont(SystemFont5x7);
      mOled.set1X();

      mOled.print(F("MedMinder: "));
      mOled.print(MED_MINDER_VERSION_STRING);
      mOled.println();
      mOled.print(F("TZ: "));
      mOled.print(zonedb::kTzDatabaseVersion);
      mOled.println();
      mOled.print(F("AceTime: "));
      mOled.print(ACE_TIME_VERSION_STRING);
      mOled.println();
      mOled.print(F("Clock: "));
    #if TIME_PROVIDER == TIME_PROVIDER_DS3231
      mOled.print(F("DS3231"));
    #elif TIME_PROVIDER == TIME_PROVIDER_NTP
      mOled.print(F("NTP"));
    #elif TIME_PROVIDER == TIME_PROVIDER_SYSTEM
      mOled.print(F("millis()"));
    #else
      mOled.print(F("<Unknown>"));
    #endif
    }

    void displayChangeMed() const {
      mOled.println("Med intrvl");

      if (shouldShowFor(MODE_CHANGE_MED_HOUR)) {
        printPad2(mOled, mRenderingInfo.timePeriod.hour());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_MED_MINUTE)) {
        printPad2(mOled, mRenderingInfo.timePeriod.minute());
      } else {
        mOled.print("  ");
      }

      mOled.clearToEOL();
    }

    void displayDateTime() const {
    #if ENABLE_SERIAL == 1
      SERIAL_PORT_MONITOR.println(F("displayDateTime()"));
    #endif
      mOled.setFont(fixed_bold10x15);
      mOled.set1X();

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
      if (shouldShowFor(MODE_CHANGE_YEAR)) {
        mOled.print(dateTime.year());
      } else {
        mOled.print("    ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_MONTH)) {
        printPad2(mOled, dateTime.month());
      } else {
        mOled.print("  ");
      }
      mOled.print('-');
      if (shouldShowFor(MODE_CHANGE_DAY)) {
        printPad2(mOled, dateTime.day());
      } else{
        mOled.print("  ");
      }
      mOled.clearToEOL();
    }

    void displayTime() const {
      const ZonedDateTime& dateTime = mRenderingInfo.dateTime;

      if (shouldShowFor(MODE_CHANGE_HOUR)) {
        printPad2(mOled, dateTime.hour());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_MINUTE)) {
        printPad2(mOled, dateTime.minute());
      } else {
        mOled.print("  ");
      }
      mOled.print(':');
      if (shouldShowFor(MODE_CHANGE_SECOND)) {
        printPad2(mOled, dateTime.second());
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
      mOled.print(typeString);
      mOled.clearToEOL();
      mOled.println();

      switch (tz.getType()) {
      #if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
        case TimeZone::kTypeManual:
          mOled.println();
          mOled.print("UTC");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_OFFSET)) {
            TimeOffset offset = tz.getStdOffset();
            offset.printTo(mOled);
          }
          mOled.clearToEOL();

          mOled.println();
          mOled.print("DST: ");
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_DST)) {
            mOled.print((tz.getDstOffset().isZero()) ? "off " : "on");
          }
          mOled.clearToEOL();
          break;
      #else
        case TimeZone::kTypeBasic:
        case TimeZone::kTypeExtended:
        case TimeZone::kTypeBasicManaged:
        case TimeZone::kTypeExtendedManaged:
          // Print name of timezone
          if (shouldShowFor(MODE_CHANGE_TIME_ZONE_NAME)) {
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
          || mRenderingInfo.dateTime != mPrevRenderingInfo.dateTime
          || mRenderingInfo.timePeriod != mPrevRenderingInfo.timePeriod;
    }

    SSD1306Ascii& mOled;
    RenderingInfo mRenderingInfo;
    RenderingInfo mPrevRenderingInfo;
};

#endif
