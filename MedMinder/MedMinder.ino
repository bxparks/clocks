/*
 * A medication reminder app. There are 2 buttons, the Mode button and the
 * Change button. Press and hold the Mode button to set the time interval
 * between doses (e.g. 24h00m). A countdown clock is shown. Press the "Change"
 * button to reset the countdown clock. Press and hold the Mode button to change
 * the time and date. Press and hold the Mode button to change the TimeZone. The
 * TimeZone is either a UTC offset plus a DST flag, or a TimeZone identifier
 * (e.g. "Los_Angeles" or "Denver").
 *
 * The hardware dependencies are:
 *
 *    * Arduino Pro Mini
 *    * a DS3231 RTC chip
 *    * an SSD1306 OLED display
 *    * 2 push buttons
 *
 * The library dependencies are:
 *    * AceButton (https://github.com/bxparks/AceButton)
 *    * AceRoutine (https://github.com/bxparks/AceRoutine)
 *    * AceTime (https://github.com/bxparks/AceTime)
 *    * AceCRC (https://github.com/bxparks/AceCRC)
 *    * SSD1306Ascii (https://github.com/greiman/SSD1306Ascii)
 *    * Low-Power (https://github.com/rocketscream/Low-Power)
 *    * EnableInterrupt (https://github.com/GreyGnome/EnableInterrupt)
 *    * EEPROM (Arduino builtin)
 *    * Wire (Arduino builtin)
 *
 * Other boards that may work are:
 *    * Arduino Nano
 *    * Arduino Pro Mini
 *    * SparkFun Pro Micro
 *    * ESP8266
 *
 * Power usage:
 *    * Pro Mini 5V 16MHz (w/ power LED)
 *      * Normal: 12 mA
 *      * Sleep: 420 uA
 *      * Powered by 3 x 800mAh NiMh AAA batteries
 *    * Pro Mini 3.3V 8Mhz (w/o power LED)
 *      * Normal: 7-9 mA (depending on OLED display)
 *      * Sleep: 162 uA
 *      * Powered by 3 x 800mAh NiMh AAA batteries
 *        * should last about 200 days.
 *    * Pro Micro 5V 16MHz (w/o power LED)
 *      * Normal: 19-24 mA (depending on OLED display)
 *      * Sleep: 288 uA
 *      * Powered by 3 AAA (3.9V)
 */

#include <Wire.h> // TwoWire, Wire
#include <AceWire.h> // TwoWireInterface
#include <SSD1306AsciiWire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>

#include "config.h"
#if ENABLE_LOW_POWER == 1
  #include <LowPower.h>
  #include <EnableInterrupt.h>
#endif
#include "Presenter.h"
#include "PersistentStore.h"
#include "Controller.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;

//------------------------------------------------------------------
// Configure PersistentStore
//------------------------------------------------------------------

const uint32_t kContextId = 0x9b2ddeca; // random contextId
const uint16_t kStoredInfoEepromAddress = 0;

PersistentStore persistentStore(kContextId, kStoredInfoEepromAddress);

void setupPersistentStore() {
  persistentStore.setup();
}

//-----------------------------------------------------------------------------
// Configure time zones and ZoneManager.
//-----------------------------------------------------------------------------

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC

static const basic::ZoneInfo* const ZONE_REGISTRY[] ACE_TIME_PROGMEM = {
  &zonedb::kZoneAmerica_Los_Angeles,
  &zonedb::kZoneAmerica_Denver,
  &zonedb::kZoneAmerica_Chicago,
  &zonedb::kZoneAmerica_New_York,
  &zonedb::kZoneEurope_London,
  &zonedb::kZoneAsia_Bangkok,
};

static const uint16_t ZONE_REGISTRY_SIZE =
    sizeof(ZONE_REGISTRY) / sizeof(basic::ZoneInfo*);

// Only 1 displayed at any given time, need 2 for conversions.
static const uint16_t CACHE_SIZE = 1 + 1;
static BasicZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static BasicZoneManager zoneManager(
    ZONE_REGISTRY_SIZE, ZONE_REGISTRY, zoneProcessorCache);

#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED

static const extended::ZoneInfo* const ZONE_REGISTRY[] ACE_TIME_PROGMEM = {
  &zonedbx::kZoneAmerica_Los_Angeles,
  &zonedbx::kZoneAmerica_Denver,
  &zonedbx::kZoneAmerica_Chicago,
  &zonedbx::kZoneAmerica_New_York,
  &zonedbx::kZoneEurope_London,
  &zonedbx::kZoneAsia_Bangkok,
};

static const uint16_t ZONE_REGISTRY_SIZE =
    sizeof(ZONE_REGISTRY) / sizeof(extended::ZoneInfo*);

// Only 1 displayed at any given time, need 2 for conversions.
static const uint16_t CACHE_SIZE = 1 + 1;
static ExtendedZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static ExtendedZoneManager zoneManager(
    ZONE_REGISTRY_SIZE, ZONE_REGISTRY, zoneProcessorCache);

#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL

static ManualZoneManager zoneManager;

#endif

//-----------------------------------------------------------------------------
// Create initial display timezone. Normally, these will be replaced by
// the information retrieved from the EEPROM by the Controller.
//-----------------------------------------------------------------------------

#if TIME_ZONE_TYPE == TIME_ZONE_MANUAL

  const TimeZoneData DISPLAY_ZONE = {-8*60, 0} /*-08:00*/;

#else

  // zoneIds are identical in zonedb:: and zonedbx::
  const TimeZoneData DISPLAY_ZONE = {zonedb::kZoneIdAmerica_Los_Angeles};

#endif

//------------------------------------------------------------------
// Configure the SystemClock.
//------------------------------------------------------------------

#if TIME_PROVIDER == TIME_PROVIDER_DS3231
  using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
  WireInterface wireInterface(Wire);
  DS3231Clock<WireInterface> dsClock(wireInterface);
  SystemClockCoroutine systemClock(&dsClock /*reference*/, &dsClock /*backup*/);
#elif TIME_PROVIDER == TIME_PROVIDER_NTP
  NtpClock ntpClock;
  SystemClockCoroutine systemClock(&ntpClock /*reference*/, nullptr /*backup*/);
#else
  SystemClockCoroutine systemClock(nullptr, nullptr);
#endif

void setupClocks() {
#if TIME_PROVIDER == TIME_PROVIDER_DS3231
  dsClock.setup();
#elif TIME_PROVIDER == TIME_PROVIDER_NTP
  ntpClock.setup(WIFI_SSID, WIFI_PASSWORD);
#endif
  systemClock.setup();
}

//------------------------------------------------------------------
// Configure the OLED display.
//------------------------------------------------------------------

SSD1306AsciiWire oled;

void setupOled() {
  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
  oled.displayRemap(OLED_REMAP);
  oled.setFont(fixed_bold10x15);
  oled.clear();
  oled.setScrollMode(false);
}

//------------------------------------------------------------------
// Configure the controller.
//------------------------------------------------------------------

Presenter presenter(zoneManager, oled);
Controller controller(systemClock, persistentStore, presenter, zoneManager,
    DISPLAY_ZONE);

void setupController() {
  controller.setup();
}

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. On a slow 16MHz
// AVR, Controller::run() returns in less than 1ms according to manual
// benchmarking. Therefore, we can set this to 100ms without worrying about too
// much overhead.
COROUTINE(runController) {
  COROUTINE_LOOP() {
    controller.update();
    COROUTINE_DELAY(100);
  }
}

COROUTINE(blinker) {
  COROUTINE_LOOP() {
    controller.updateBlinkState();
    COROUTINE_DELAY(500);
  }
}

//------------------------------------------------------------------
// Sleep Manager
//------------------------------------------------------------------

enum class RunMode : uint8_t {
  kAwake,
  kSleeping,
  kDreaming,
};

static uint16_t lastUserActionMillis;

#if ENABLE_LOW_POWER == 1
volatile RunMode runMode = RunMode::kAwake;

void buttonInterrupt() {
  runMode = RunMode::kAwake;
}

const uint16_t SLEEP_DELAY_MILLIS = 5000;

static bool isWakingUp;

COROUTINE(manageSleep) {
  COROUTINE_LOOP() {
    // Go to sleep if more than 5 seconds passes after the last user action.
    COROUTINE_AWAIT((uint16_t) ((uint16_t) millis() - lastUserActionMillis)
        >= SLEEP_DELAY_MILLIS);
    controller.prepareToSleep();
    if (ENABLE_SERIAL_DEBUG) {
      SERIAL_PORT_MONITOR.println("Powering down");
      COROUTINE_DELAY(500);
    }

    runMode = RunMode::kSleeping;
    // What happens if a button is pressed right here?
    while (true) {
      isWakingUp = false;
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

      // Check if button caused wakeup.
      if (runMode == RunMode::kAwake) break;

      isWakingUp = true;
      if (ENABLE_SERIAL_DEBUG) {
        SERIAL_PORT_MONITOR.println(
            F("Dreaming for 1000ms... then going back to sleep"));
        COROUTINE_DELAY(500);
      }
      runMode = RunMode::kDreaming;
      COROUTINE_DELAY(250);
    }

    if (ENABLE_SERIAL_DEBUG) SERIAL_PORT_MONITOR.println("Powering up");
    controller.wakeup();
    isWakingUp = true;
    lastUserActionMillis = millis();
  }
}
#endif

//------------------------------------------------------------------
// Configurations for AceButton
//------------------------------------------------------------------

ButtonConfig buttonConfig;
AceButton modeButton(&buttonConfig, MODE_BUTTON_PIN);
AceButton changeButton(&buttonConfig, CHANGE_BUTTON_PIN);

void handleButton(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  lastUserActionMillis = millis();
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
      case AceButton::kEventReleased:
      case AceButton::kEventLongReleased:
      #if ENABLE_LOW_POWER
        // Eat the first button release event if just woken up.
        if (isWakingUp) {
          isWakingUp = false;
          return;
        }
      #endif
        controller.handleChangeButtonRelease();
        break;

      // Repeatedly advance the editable field.
      case AceButton::kEventRepeatPressed:
        controller.handleChangeButtonRepeatPress();
        break;

      // Reset countdown timer.
      case AceButton::kEventLongPressed:
        controller.handleChangeButtonLongPress();
        break;
    }
  } else if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      // Advance to the next mode.
      case AceButton::kEventReleased:
      #if ENABLE_LOW_POWER
        // Eat the first button event if just woken up.
        if (isWakingUp) {
          isWakingUp = false;
          return;
        }
      #endif
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

  buttonConfig.setEventHandler(handleButton);
  buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig.setRepeatPressInterval(150);
}

//------------------------------------------------------------------
// MedMinder main loop
//------------------------------------------------------------------

void setup() {
  // Wait for stability on some boards.
  // 1000ms needed for Serial.
  // 1500ms needed for Wire, I2C or SSD1306 (don't know which one).
  delay(2000);

#if defined(ARDUINO_AVR_LEONARDO)
  RXLED0; // LED off
  TXLED0; // LED off
#endif

  if (ENABLE_SERIAL_DEBUG) {
    SERIAL_PORT_MONITOR.begin(115200);
    while (!SERIAL_PORT_MONITOR); // Wait until Serial is ready - Leonardo/Micro
    SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  }

  Wire.begin();
  Wire.setClock(400000L);

  setupPersistentStore();
  setupClocks();
  setupAceButton();
  setupOled();
  setupController();

#if ENABLE_LOW_POWER == 1
  enableInterrupt(MODE_BUTTON_PIN, buttonInterrupt, CHANGE);
  enableInterrupt(CHANGE_BUTTON_PIN, buttonInterrupt, CHANGE);
#endif

  lastUserActionMillis = millis();

  CoroutineScheduler::setup();

  if (ENABLE_SERIAL_DEBUG) SERIAL_PORT_MONITOR.println(F("setup(): end"));
}

void loop() {
  CoroutineScheduler::loop();

  // Calling AceButton loops directly instead of in a COROUTINE() saves 146
  // bytes of flash, and 31 bytes of SRAM.
  modeButton.check();
  changeButton.check();
}
