/*
 * A digital clock with 3 OLED displays using SPI, to show 3 different time
 * zones. The hardware consists of:
 *
 *   * 1 x DS3231 RTC chip (I2C)
 *   * 3 x SSD1306 OLED displays using SPI interface (not I2C)
 *   * 2 x push buttons
 *
 * (It occurs to me that it should be possible support 3 displays using the I2C
 * version of the SSD1306 as well, if we used 3 different SDA lines to the OLED
 * devices, sharing a common SCL line, then use software I2C to communicate with
 * them. This would require using a different SSD1306 library, or modify the
 * SSD1306Ascii library to handle different pins.)
 *
 * Tested on Arduino Pro Micro, but should work for Arduino Nano, ESP8266 and
 * ESP32.
 *
 * Dependencies:
 *
 *  * AceTime (https://github.com/bxparks/AceTime)
 *  * AceRoutine (https://github.com/bxparks/AceRoutine)
 *  * AceButton (https://github.com/bxparks/AceButton)
 *  * AceCommon (https://github.com/bxparks/AceCommon)
 *  * AceUtils (https://github.com/bxparks/AceUtils)
 *  * AceCRC (https://github.com/bxparks/AceCRC)
 *  * SSD1306Ascii (https://github.com/greiman/SSD1306Ascii)
 *
 * Program Size (flash/ram)
 *  * 24798/1531
 *  * 24852/1525
 *    * add support for kInvertDisplayMinutely
 *  * 25242/1502 bytes
 *    * add Invert: on/off indicators
 *    * add AceCommon, AceCRC version numbers to About page
 *    * add setFont() to select Adafruit5x7 font
 *    * simplify clearToEOL()
 *  * 25386/1505 bytes
 *    * AceTime v1.8
 *    * AceTimeClock v1.0
 */

#include "config.h"
#include <Wire.h> // TwoWire, Wire
#include <AceWire.h> // TwoWireInterface
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceCommon.h>
#include <AceUtils.h>
#include <mode_group/mode_group.h> // from AceUtils
#include <SSD1306AsciiSpi.h>
#include "ClockInfo.h"
#include "Controller.h"
#include "PersistentStore.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using ace_utils::mode_group::ModeGroup;

//----------------------------------------------------------------------------
// Configure PersistentStore
//----------------------------------------------------------------------------

const uint32_t kContextId = 0x8f01000b; // random contextId
const uint16_t kStoredInfoEepromAddress = 0;

PersistentStore persistentStore(kContextId, kStoredInfoEepromAddress);

void setupPersistentStore() {
  persistentStore.setup();
}

//----------------------------------------------------------------------------
// Configure various Clocks.
//----------------------------------------------------------------------------

using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
WireInterface wireInterface(Wire);
DS3231Clock<WireInterface> dsClock(wireInterface);
SystemClockCoroutine systemClock(&dsClock, &dsClock);

//----------------------------------------------------------------------------
// Configure OLED display using SSD1306Ascii.
//----------------------------------------------------------------------------

SSD1306AsciiSpi oled0;
SSD1306AsciiSpi oled1;
SSD1306AsciiSpi oled2;

void setupOled() {
  pinMode(OLED_CS0_PIN, OUTPUT);
  pinMode(OLED_CS1_PIN, OUTPUT);
  pinMode(OLED_CS2_PIN, OUTPUT);

  digitalWrite(OLED_CS0_PIN, HIGH);
  digitalWrite(OLED_CS1_PIN, HIGH);
  digitalWrite(OLED_CS2_PIN, HIGH);

  oled0.begin(&Adafruit128x64, OLED_CS0_PIN, OLED_DC_PIN, OLED_RST_PIN);
  oled1.begin(&Adafruit128x64, OLED_CS1_PIN, OLED_DC_PIN);
  oled2.begin(&Adafruit128x64, OLED_CS2_PIN, OLED_DC_PIN);

  oled0.clear();
  oled1.clear();
  oled2.clear();

  oled0.setScrollMode(false);
  oled1.setScrollMode(false);
  oled2.setScrollMode(false);
}

//----------------------------------------------------------------------------
// Create 3 Presenters for 3 OLED displays
//----------------------------------------------------------------------------

Presenter presenter0(oled0);
Presenter presenter1(oled1);
Presenter presenter2(oled2);

//----------------------------------------------------------------------------
// Setup time zones.
//----------------------------------------------------------------------------

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
TimeZone tz0 = TimeZone::forHours(-8);
TimeZone tz1 = TimeZone::forHours(-5);
TimeZone tz2 = TimeZone::forHours(0);
#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC
BasicZoneProcessor zoneProcessor0;
BasicZoneProcessor zoneProcessor1;
BasicZoneProcessor zoneProcessor2;
TimeZone tz0 = TimeZone::forZoneInfo(&zonedb::kZoneAmerica_Los_Angeles,
    &zoneProcessor0);
TimeZone tz1 = TimeZone::forZoneInfo(&zonedb::kZoneAmerica_New_York,
    &zoneProcessor1);
TimeZone tz2 = TimeZone::forZoneInfo(&zonedb::kZoneEurope_London,
    &zoneProcessor2);
#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED
ExtendedZoneProcessor zoneProcessor0;
ExtendedZoneProcessor zoneProcessor1;
ExtendedZoneProcessor zoneProcessor2;
TimeZone tz0 = TimeZone::forZoneInfo(&zonedbx::kZoneAmerica_Los_Angeles,
    &zoneProcessor0);
TimeZone tz1 = TimeZone::forZoneInfo(&zonedbx::kZoneAmerica_New_York,
    &zoneProcessor1);
TimeZone tz2 = TimeZone::forZoneInfo(&zonedbx::kZoneEurope_London,
    &zoneProcessor2);
#else
  #error Unknown TIME_ZONE_TYPE
#endif

//-----------------------------------------------------------------------------
// Create mode groups that define the navigation path for the Mode button.
// It forms a recursive tree structure that looks like this:
//
// - View Date Time
//    - Change hour
//    - Change minute
//    - Change second
//    - Change day
//    - Change month
//    - Change year
// - View Settings
//    - Change 12/24 mode
//    - Change Blinking mode
//    - Change Contrast
// - About
//    - (no submodes)

// The Arduino compiler becomes confused without this.
extern const ModeGroup ROOT_MODE_GROUP;

// List of DateTime modes.
const uint8_t DATE_TIME_MODES[] = {
  (uint8_t) Mode::kChangeYear,
  (uint8_t) Mode::kChangeMonth,
  (uint8_t) Mode::kChangeDay,
  (uint8_t) Mode::kChangeHour,
  (uint8_t) Mode::kChangeMinute,
  (uint8_t) Mode::kChangeSecond,
};

// ModeGroup for the DateTime modes.
const ModeGroup DATE_TIME_MODE_GROUP = {
  &ROOT_MODE_GROUP /* parentGroup */,
  sizeof(DATE_TIME_MODES) / sizeof(uint8_t),
  DATE_TIME_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of TimeZone modes.
//const uint8_t TIME_ZONE_MODES[] = {
//#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
//  Mode::kchangeTimeZoneOffset,
//  Mode::kchangeTimeZoneDst,
//#else
//  Mode::kchangeTimeZoneName,
//#endif
//};

// MOdeGroup for the TimeZone modes.
//const ModeGroup TIME_ZONE_MODE_GROUP = {
//  &ROOT_MODE_GROUP /* parentGroup */,
//  sizeof(TIME_ZONE_MODES) / sizeof(uint8_t),
//  TIME_ZONE_MODES /* modes */,
//  nullptr /* childGroups */,
//};

// List of Settings modes.
const uint8_t SETTINGS_MODES[] = {
  (uint8_t) Mode::kChangeHourMode,
  (uint8_t) Mode::kChangeBlinkingColon,
  (uint8_t) Mode::kChangeContrast,
  (uint8_t) Mode::kChangeInvertDisplay,
};

// ModeGroup for the Settings modes.
const ModeGroup SETTINGS_MODE_GROUP = {
  &ROOT_MODE_GROUP /* parentGroup */,
  sizeof(SETTINGS_MODES) / sizeof(uint8_t),
  SETTINGS_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of top level modes.
const uint8_t TOP_LEVEL_MODES[] = {
  (uint8_t) Mode::kViewDateTime,
  (uint8_t) Mode::kViewSettings,
  (uint8_t) Mode::kViewAbout,
};

// List of children ModeGroups for each element in TOP_LEVEL_MODES, in the same
// order.
const ModeGroup* const TOP_LEVEL_CHILD_GROUPS[] = {
  &DATE_TIME_MODE_GROUP,
  //&TIME_ZONE_MODE_GROUP,
  &SETTINGS_MODE_GROUP,
  nullptr, // About mode has no submodes
};

// Root mode group
const ModeGroup ROOT_MODE_GROUP = {
  nullptr /* parentGroup */,
  sizeof(TOP_LEVEL_MODES) / sizeof(uint8_t),
  TOP_LEVEL_MODES /* modes */,
  TOP_LEVEL_CHILD_GROUPS /* childGroups */,
};

//----------------------------------------------------------------------------
// Create controller with 3 presenters for the 3 OLED displays.
//----------------------------------------------------------------------------

Controller controller(
    systemClock, persistentStore, &ROOT_MODE_GROUP,
    presenter0, presenter1, presenter2,
    tz0, tz1, tz2,
    "SFO", "PHL", "LHR"
);

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.update() runs faster than 1ms so we can set this
// to 100ms without worrying about too much overhead.
COROUTINE(updateController) {
  COROUTINE_LOOP() {
    controller.update();

    COROUTINE_YIELD();
    controller.updatePresenter0();

    COROUTINE_YIELD();
    controller.updatePresenter1();

    COROUTINE_YIELD();
    controller.updatePresenter2();

    COROUTINE_DELAY(100);
  }
}

//----------------------------------------------------------------------------
// Configure AceButton.
//----------------------------------------------------------------------------

AceButton modeButton((uint8_t) MODE_BUTTON_PIN);
AceButton changeButton((uint8_t) CHANGE_BUTTON_PIN);

void handleButton(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  uint8_t pin = button->getPin();

  if (pin == CHANGE_BUTTON_PIN) {
    switch (eventType) {
      // Advance editable field by one step.
      case AceButton::kEventPressed:
        controller.handleChangeButtonPress();
        break;

      // Resume blinking, because both handleChangeButtonPress() and
      // handleChangeButtonRepeatPress() suppress blinking until the button is
      // lifted.
      //
      // We have only a single ButtonConfig, so both buttons will trigger a
      // DoubleClicked. On the Change button, we have to treat it like just a
      // normal click release. Otherwise, the clock never gets the Released
      // event since it is suppressed by kFeatureSuppressAfterDoubleClick.
      case AceButton::kEventDoubleClicked:
      case AceButton::kEventClicked:
      case AceButton::kEventReleased:
      case AceButton::kEventLongReleased:
        controller.handleChangeButtonRelease();
        break;

      // Repeatedly advance the editable field.
      case AceButton::kEventRepeatPressed:
        controller.handleChangeButtonRepeatPress();
        break;
    }
  } else if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      // Advance to the next mode.
      case AceButton::kEventReleased:
      case AceButton::kEventClicked:
        controller.handleModeButtonPress();
        break;

      // Toggle Edit mode.
      case AceButton::kEventLongPressed:
        controller.handleModeButtonLongPress();
        break;

      // Cancel Edit mode.
      case AceButton::kEventDoubleClicked:
        controller.handleModeButtonDoubleClick();
        break;
    }
  }
}

void setupAceButton() {
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleButton);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig->setRepeatPressInterval(150);

  // Shorten the DoubleClick delay to prevent accidental interpretation of
  // rapid Mode button presses as a DoubleClick.
  buttonConfig->setDoubleClickDelay(250);
}

//----------------------------------------------------------------------------
// Main setup and loop
//----------------------------------------------------------------------------

void setup() {
  // Wait for stability on some boards.
  // 1000ms needed for SERIAL_PORT_MONITOR.
  // 1500ms needed for Wire, I2C or SSD1306 (don't know which one).
  delay(2000);

  // Turn off the RX and TX LEDs on Leonardos
#if defined(ARDUINO_AVR_LEONARDO)
  RXLED0; // LED off
  TXLED0; // LED off
#endif

  if (ENABLE_SERIAL_DEBUG >= 1) {
    SERIAL_PORT_MONITOR.begin(115200);
    while (!SERIAL_PORT_MONITOR); // Leonardo/Micro
    SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  }

  Wire.begin();
  Wire.setClock(400000L);

  setupPersistentStore();
  setupAceButton();
  setupOled();

  dsClock.setup();
  systemClock.setup();

  // Hold down the Mode button to perform factory reset.
  bool isModePressedDuringBoot = modeButton.isPressedRaw();
  controller.setup(isModePressedDuringBoot);

  if (ENABLE_SERIAL_DEBUG >= 1) {
    SERIAL_PORT_MONITOR.println(F("setup(): end"));
  }
}

void loop() {
  // Using the CoroutineScheduler is conceptually cleaner, but consumes 159
  // bytes of extra flash memory. So run the coroutines manually instead of
  // call CoroutineScheduler::loop();
  updateController.runCoroutine();
  systemClock.runCoroutine();

  // Call AceButton::check directly instead of using COROUTINE() to save 174
  // bytes in flash memory, and 31 bytes in static memory.
  modeButton.check();
  changeButton.check();
}
