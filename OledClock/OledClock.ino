/*
 * A simple digital clock using:
 *   * a DS3231 RTC chip and/or NTP server
 *   * an SSD1306 OLED display
 *   * 2 push buttons
 *
 * Tested using:
 *   * Arduino Nano
 *   * Arduino Pro Mini
 *   * Arduino Pro Micro
 *   * ESP8266
 *
 * Dependencies:
 *   * [AceTime](https://github.com/bxparks/AceTime)
 *   * [AceRoutine](https://github.com/bxparks/AceRoutine)
 *   * [AceButton](https://github.com/bxparks/AceButton)
 *   * [FastCRC](https://github.com/FrankBoesing/FastCRC)
 *   * [SSD1306Ascii](https://github.com/greiman/SSD1306Ascii)
 *
 * If ENABLE_SERIAL is set to 1, it prints diagnostics like this:
 *  * sizeof(ClockInfo): 32
 *  * sizeof(StoredInfo): 4
 *  * sizeof(RenderingInfo): 28
 */

#include <Wire.h>
#include <SSD1306AsciiWire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include "config.h"
#include "PersistentStore.h"
#include "Presenter.h"
#include "Controller.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;

//------------------------------------------------------------------
// Configure various Clocks and Clocks.
//------------------------------------------------------------------

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  DS3231Clock dsClock;
  SystemClockCoroutine systemClock(&dsClock, &dsClock);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  SystemClockCoroutine systemClock(&ntpClock, nullptr);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  DS3231Clock dsClock;
  NtpClock ntpClock;
  SystemClockCoroutine systemClock(&ntpClock, &dsClock);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  SystemClockCoroutine systemClock(nullptr /*sync*/, nullptr /*backup*/);
#else
  #error Unknown clock option
#endif

//------------------------------------------------------------------
// Configure OLED display using SSD1306Ascii.
//------------------------------------------------------------------

// OLED address: 0X3C+SA0 - 0x3C or 0x3D
#define OLED_I2C_ADDRESS 0x3C

SSD1306AsciiWire oled;

void setupOled() {
  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
  oled.displayRemap(OLED_REMAP);
  oled.clear();
}

//------------------------------------------------------------------
// Create controller/presenter pair.
//------------------------------------------------------------------

PersistentStore persistentStore;
Presenter presenter(oled);
Controller controller(persistentStore, systemClock, presenter);

//------------------------------------------------------------------
// Render the Clock periodically.
//------------------------------------------------------------------

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.display() runs as fast as or faster than 1ms, so
// we can set this to 100ms without worrying about too much overhead.
COROUTINE(displayClock) {
  COROUTINE_LOOP() {
    controller.update();
    COROUTINE_DELAY(100);
  }
}

//------------------------------------------------------------------
// Configure AceButton.
//------------------------------------------------------------------

#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL

ButtonConfig buttonConfig;
AceButton modeButton(&buttonConfig, MODE_BUTTON_PIN);
AceButton changeButton(&buttonConfig, CHANGE_BUTTON_PIN);

#else

  AceButton modeButton((uint8_t) MODE_BUTTON_PIN);
  AceButton changeButton((uint8_t) CHANGE_BUTTON_PIN);
  AceButton* const BUTTONS[] = {&modeButton, &changeButton};
  #if ANALOG_BITS == 8
    uint16_t LEVELS[] = {0, 128, 255};
  #elif ANALOG_BITS == 10
    uint16_t LEVELS[] = {0, 512, 1023};
  #else
    #error Unknown number of ADC bits
  #endif
  LadderButtonConfig buttonConfig(ANALOG_BUTTON_PIN, 3, LEVELS, 2, BUTTONS);

#endif

void handleButton(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  uint8_t pin = button->getPin();

  if (pin == CHANGE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:
        controller.changeButtonPress();
        break;

      case AceButton::kEventReleased:
      case AceButton::kEventLongReleased:
        controller.changeButtonRelease();
        break;

      case AceButton::kEventRepeatPressed:
        controller.changeButtonRepeatPress();
        break;
    }
  } else if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventReleased:
        controller.modeButtonPress();
        break;

      case AceButton::kEventLongPressed:
        controller.modeButtonLongPress();
        break;
    }
  }
}

void setupAceButton() {
#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);
#endif

  buttonConfig.setEventHandler(handleButton);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig.setRepeatPressInterval(150);
}

//------------------------------------------------------------------
// Main setup and loop
//------------------------------------------------------------------

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

#if ENABLE_SERIAL == 1
  SERIAL_PORT_MONITOR.begin(115200);
  while (!SERIAL_PORT_MONITOR); // Leonardo/Micro
  SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  SERIAL_PORT_MONITOR.print(F("sizeof(ClockInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(ClockInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(StoredInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(StoredInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(RenderingInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(RenderingInfo));
#endif

  Wire.begin();
  Wire.setClock(400000L);

  setupAceButton();
  setupOled();

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  dsClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  ntpClock.setup(AUNITER_SSID, AUNITER_PASSWORD);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  dsClock.setup();
  ntpClock.setup(AUNITER_SSID, AUNITER_PASSWORD);
#endif
  systemClock.setup();
  controller.setup();
  persistentStore.setup();

  systemClock.setupCoroutine(F("clock"));
  CoroutineScheduler::setup();

#if ENABLE_SERIAL == 1
  SERIAL_PORT_MONITOR.println(F("setup(): end"));
#endif
}

void loop() {
  CoroutineScheduler::loop();
#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
  modeButton.check();
  changeButton.check();
#else
  buttonConfig.checkButtons();
#endif
}
