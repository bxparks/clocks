/*
 * A digital clock with 3 OLED displays to show 3 different time zones. The
 * hardware consists of:
 *
 *   * 1 x DS3231 RTC chip (I2C)
 *   * 3 x SSD1306 OLED displays using SPI interface (not I2C)
 *   * 2 x push buttons
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
 */

#include "config.h"
#include <Wire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include <AceUtilsModeGroup.h>
#include <SSD1306AsciiSpi.h>
#include "ClockInfo.h"
#include "Controller.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using ace_utils::mode_group::ModeGroup;
using ace_utils::crc_eeprom::AvrEepromAdapter;
using ace_utils::crc_eeprom::EspEepromAdapter;
using ace_utils::crc_eeprom::CrcEeprom;

//----------------------------------------------------------------------------
// Configure CrcEeprom.
//----------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266) || defined(EPOXY_DUINO)
  #include <EEPROM.h>
  EspEepromAdapter<EEPROMClass> eepromAdapter(EEPROM);
#elif defined(ARDUINO_ARCH_AVR)
  #include <EEPROM.h>
  AvrEepromAdapter<EEPROMClass> eepromAdapter(EEPROM);
#elif defined(ARDUINO_ARCH_STM32)
  #include <AceUtilsStm32BufferedEeprom.h>
  EspEepromAdapter<BufferedEEPROMClass> eepromAdapter(BufferedEEPROM);
#else
  #error EEPROM unsupported
#endif

// Needed by ESP8266 and ESP32 chips. Has no effect on other chips.
const int EEPROM_SIZE = CrcEeprom::toSavedSize(sizeof(StoredInfo));

CrcEeprom crcEeprom(eepromAdapter, CrcEeprom::toContextId('w', 'c', 'l', 'k'));

//----------------------------------------------------------------------------
// Configure various Clocks.
//----------------------------------------------------------------------------

DS3231Clock dsClock;
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
  MODE_CHANGE_YEAR,
  MODE_CHANGE_MONTH,
  MODE_CHANGE_DAY,
  MODE_CHANGE_HOUR,
  MODE_CHANGE_MINUTE,
  MODE_CHANGE_SECOND,
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
//  MODE_CHANGE_TIME_ZONE_OFFSET,
//  MODE_CHANGE_TIME_ZONE_DST,
//#else
//  MODE_CHANGE_TIME_ZONE_NAME,
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
  MODE_CHANGE_HOUR_MODE,
  MODE_CHANGE_BLINKING_COLON,
  MODE_CHANGE_CONTRAST,
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
  MODE_DATE_TIME,
  //MODE_TIME_ZONE,
  MODE_SETTINGS,
  MODE_ABOUT,
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
    systemClock, crcEeprom, &ROOT_MODE_GROUP,
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
      case AceButton::kEventPressed:
        controller.handleChangeButtonPress();
        break;
      case AceButton::kEventReleased:
      case AceButton::kEventLongReleased:
        controller.handleChangeButtonRelease();
        break;
      case AceButton::kEventRepeatPressed:
        controller.handleChangeButtonRepeatPress();
        break;
    }
  } else if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventReleased:
        controller.handleModeButtonPress();
        break;
      case AceButton::kEventLongPressed:
        controller.handleModeButtonLongPress();
        break;
    }
  }
}

void setupAceButton() {
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleButton);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig->setRepeatPressInterval(150);
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

  if (ENABLE_SERIAL_DEBUG == 1) {
    SERIAL_PORT_MONITOR.begin(115200);
    while (!SERIAL_PORT_MONITOR); // Leonardo/Micro
    SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  }

  Wire.begin();
  Wire.setClock(400000L);
  crcEeprom.begin(EEPROM_SIZE);

  setupAceButton();
  setupOled();

  dsClock.setup();
  systemClock.setup();

  // Hold down the Mode button to perform factory reset.
  bool isModePressedDuringBoot = modeButton.isPressedRaw();
  controller.setup(isModePressedDuringBoot);

  if (ENABLE_SERIAL_DEBUG == 1) {
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
