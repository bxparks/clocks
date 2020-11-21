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
 *    * FastCRC (https://github.com/FrankBoesing/FastCRC)
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

#include <Wire.h>
#include <SSD1306AsciiWire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <CrcEeprom.h>

#include "config.h"
#if ENABLE_LOW_POWER == 1
  #include <LowPower.h>
  #include <EnableInterrupt.h>
#endif
#include "Presenter.h"
#include "Controller.h"
#include <ace_time/hw/HardwareDateTime.h>

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;

//------------------------------------------------------------------
// Configure CrcEeprom.
//------------------------------------------------------------------

// Needed by ESP32 chips. Has no effect on other chips.
// Should be bigger than (sizeof(crc32) + sizeof(StoredInfo)).
#define EEPROM_SIZE 32

CrcEeprom crcEeprom;

//------------------------------------------------------------------
// Configure the SystemClock.
//------------------------------------------------------------------

#if TIME_PROVIDER == TIME_PROVIDER_DS3231
  DS3231Clock dsClock;
  SystemClockCoroutine systemClock(&dsClock /*reference*/, &dsClock /*backup*/);
#elif TIME_PROVIDER == TIME_PROVIDER_NTP
  NtpClock ntpClock;
  SystemClockCoroutine systemClock(&ntpClock /*reference*/, nullptr /*backup*/);
#else
  SystemClockCoroutine systemClock(nullptr, nullptr);
#endif

//------------------------------------------------------------------
// Configure the OLED display.
//------------------------------------------------------------------

SSD1306AsciiWire oled;

void setupOled() {
  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
  oled.displayRemap(OLED_REMAP);
  oled.clear();
}

//------------------------------------------------------------------
// Configure the controller.
//------------------------------------------------------------------

Presenter presenter(oled);
Controller controller(systemClock, crcEeprom, presenter);

//------------------------------------------------------------------
// Run the controller.
//------------------------------------------------------------------

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

//------------------------------------------------------------------
// Sleep Manager
//------------------------------------------------------------------

#define RUN_MODE_AWAKE 0
#define RUN_MODE_SLEEPING 1
#define RUN_MODE_DREAMING 2

static uint16_t lastUserActionMillis;

#if ENABLE_LOW_POWER == 1
volatile uint8_t runMode = RUN_MODE_AWAKE;

void buttonInterrupt() {
  runMode = RUN_MODE_AWAKE;
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

    runMode = RUN_MODE_SLEEPING;
    // What happens if a button is pressed right here?
    while (true) {
      isWakingUp = false;
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

      // Check if button caused wakeup.
      if (runMode == RUN_MODE_AWAKE) break;

      isWakingUp = true;
      if (ENABLE_SERIAL_DEBUG) {
        SERIAL_PORT_MONITOR.println(
            F("Dreaming for 1000ms... then going back to sleep"));
        COROUTINE_DELAY(500);
      }
      runMode = RUN_MODE_DREAMING;
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

ButtonConfig modeButtonConfig;
AceButton modeButton(&modeButtonConfig);

ButtonConfig changeButtonConfig;
AceButton changeButton(&changeButtonConfig);

void handleModeButton(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  lastUserActionMillis = millis();

  switch (eventType) {
    case AceButton::kEventReleased:
#if ENABLE_LOW_POWER
      // Eat the first button event if just woken up.
      if (isWakingUp) {
        isWakingUp = false;
        return;
      }
#endif
      controller.modeButtonPress();
      break;
    case AceButton::kEventLongPressed:
      controller.modeButtonLongPress();
      break;
  }
}

void handleChangeButton(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  lastUserActionMillis = millis();

  switch (eventType) {
    case AceButton::kEventPressed:
      controller.changeButtonPress();
      break;
    case AceButton::kEventReleased:
#if ENABLE_LOW_POWER
      // Eat the first button release event if just woken up.
      if (isWakingUp) {
        isWakingUp = false;
        return;
      }
#endif
      controller.changeButtonRelease();
      break;
    case AceButton::kEventRepeatPressed:
      controller.changeButtonRepeatPress();
      break;
    case AceButton::kEventLongPressed:
      controller.changeButtonLongPress();
      break;
  }
}

void setupAceButton() {
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  modeButton.init(MODE_BUTTON_PIN);

  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);
  changeButton.init(CHANGE_BUTTON_PIN);

  modeButtonConfig.setEventHandler(handleModeButton);
  modeButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  modeButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  changeButtonConfig.setEventHandler(handleChangeButton);
  changeButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  changeButtonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  changeButtonConfig.setRepeatPressInterval(150);
}

COROUTINE(checkButton) {
  COROUTINE_LOOP() {
    modeButton.check();
    changeButton.check();
    COROUTINE_DELAY(5); // check button every 5 ms
  }
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

  crcEeprom.begin(EEPROM_SIZE);
#if TIME_PROVIDER == TIME_PROVIDER_DS3231
  dsClock.setup();
#elif TIME_PROVIDER == TIME_PROVIDER_NTP
  ntpClock.setup(AUNITER_SSID, AUNITER_PASSWORD);
#endif
  systemClock.setup();

  setupAceButton();
  setupOled();
  controller.setup();

#if ENABLE_LOW_POWER == 1
  enableInterrupt(MODE_BUTTON_PIN, buttonInterrupt, CHANGE);
  enableInterrupt(CHANGE_BUTTON_PIN, buttonInterrupt, CHANGE);
#endif

  lastUserActionMillis = millis();

  systemClock.setupCoroutine(F("systemClock"));
  CoroutineScheduler::setup();

  if (ENABLE_SERIAL_DEBUG) SERIAL_PORT_MONITOR.println(F("setup(): end"));
}

void loop() {
  CoroutineScheduler::loop();
}
