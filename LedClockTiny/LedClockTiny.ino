/*
A simplified version of LedClock to reduce flash memory size. Uses:
  * a DS3231 RTC chip
  * a TM1637 7-segment LED display
  * 2 push buttons

Support boards are:
  * SparkFun Pro Micro
  * ATtiny85

Differences compared to LedClock:
  * no timezone support, no DST shifts
  * dayOfWeek is set explicitly, instead of derived from yyyy-mm-dd

Memory size (flash/ram) for `au --cli verify attiny_tm1637`:

  * Initial fork:
      * Pro Micro: 23352/1268
  * Remove ZoneManager and ZonedDateTime:
      * Pro Micro: 13102/641
  * Remove CrcEeprom:
      * Pro Micro: 12160/616
      * ATtiny85: 7734/305
  * Remove COROUTINE(renderLed)
      * Pro Micro: 12040/585
      * ATtiny85: 7612/274
      * Saves about 120 bytes
  * Remove COROUTINE(checkButtons)
      * Pro Micro: 11800/556
      * ATtiny85: 7370/245
      * Saves about 250 bytes
  * Remove COROUTINE(updateClock)
      * Pro Micro: 10868/515
      * ATtiny85: 6456/204
      * Saves about 900 bytes (!)
  * Add back CrcEeprom
      * Pro Micro: 11842/540
      * ATtiny85: 6976/226
      * Increase flash by 1000 bytes on Pro Micro, but only about 500 bytes on
        ATtiny85.
  * Add ability to change dayOfWeek
      * Pro Micro: 11888/540
      * ATtiny85: 7016/226
  * Latest:
      * Pro Micro: 11618/528 (EEPROM)
      * Pro Micro: 10832/518 (no EEPROM)
      * ATtiny85: 7310/260 (EEPROM)
      * ATtiny85: 6366/250 (no EEPROM)
*/

#include "config.h"
#include <Wire.h>
#include <AceSegment.h>
#include <AceButton.h>
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include "Controller.h"

#if defined(ARDUINO_ARCH_AVR) || defined(EPOXY_DUINO)
#include <digitalWriteFast.h>
#include <ace_segment/hw/SoftSpiFastInterface.h>
#include <ace_segment/hw/SoftTmiFastInterface.h>
#endif

using namespace ace_segment;
using namespace ace_button;
using namespace ace_time;
using namespace ace_time::clock;

//------------------------------------------------------------------
// Configure PersistentStore
//------------------------------------------------------------------

const uint32_t kContextId = 0xd12a3e47; // random contextId
const uint16_t kStoredInfoEepromAddress = 0;

PersistentStore persistentStore(kContextId, kStoredInfoEepromAddress);

void setupPersistentStore() {
  persistentStore.setup();
}

//------------------------------------------------------------------
// Configure various Clocks
//------------------------------------------------------------------

DS3231 ds3231;

//------------------------------------------------------------------
// Configure LED display using AceSegment.
//------------------------------------------------------------------

const uint8_t FRAMES_PER_SECOND = 60;

// The chain of resources.
#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using TmiInterface = SoftTmiInterface;
    TmiInterface tmiInterface(DIO_PIN, CLK_PIN, BIT_DELAY);
    Tm1637Module<TmiInterface, NUM_DIGITS> ledModule(tmiInterface);
  #else
    using TmiInterface = SoftTmiFastInterface<DIO_PIN, CLK_PIN, BIT_DELAY>;
    TmiInterface tmiInterface;
    Tm1637Module<TmiInterface, NUM_DIGITS> ledModule(tmiInterface);
  #endif

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
  const uint8_t NUM_DIGITS = 8;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SoftSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SoftSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  Max7219Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface, kDigitRemapArray8Max7219);

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE
    #include <Wire.h>
    using WireInterface = TwoWireInterface<TwoWire>;
    WireInterface wireInterface(Wire);
  #elif INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE
    using WireInterface = SimpleWireInterface;
    WireInterface wireInterface(SDA_PIN, SCL_PIN, BIT_DELAY);
  #elif INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST
    using WireInterface = SimpleWireFastInterface<SDA_PIN, SCL_PIN, BIT_DELAY>;
    WireInterface wireInterface;
  #endif

  Ht16k33Module<WireInterface, NUM_DIGITS> ledModule(
      wireInterface, HT16K33_I2C_ADDRESS, true /* enableColon */);

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595
  // Common Anode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SoftSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SoftSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  Hc595Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      SEGMENT_ON_PATTERN,
      DIGIT_ON_PATTERN,
      FRAMES_PER_SECOND,
      HC595_BYTE_ORDER,
      REMAP_ARRAY
  );

#else
  #error Unknown LED_DISPLAY_TYPE

#endif

// Setup the various resources.
void setupAceSegment() {
  #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595 \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
    spiInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
    tmiInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
    wireInterface.begin();
  #else
    #error Unknown LED_DISPLAY_TYPE
  #endif

  ledModule.begin();
}

void renderLed() {
  static uint16_t lastRunMillis;

  uint16_t nowMillis = millis();
  if (nowMillis - lastRunMillis >= 200) {
    lastRunMillis = nowMillis;

    #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595
      ledModule.renderFieldWhenReady();
    #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
      ledModule.flushIncremental();
    #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
      ledModule.flush();
    #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
      ledModule.flush();
    #endif
  }
}

//------------------------------------------------------------------
// Create an appropriate controller/presenter pair.
//------------------------------------------------------------------

Presenter presenter(ledModule);
Controller controller(ds3231, persistentStore, presenter);

//------------------------------------------------------------------
// Update the Presenter Clock periodically.
//------------------------------------------------------------------

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.display() runs as fast as or faster than 1ms for
// all DISPLAY_TYPEs. So we can set this to 100ms without worrying about too
// much overhead.
void updateClock() {
  static uint16_t lastRunMillis;

  uint16_t nowMillis = millis();
  if (nowMillis - lastRunMillis >= 200) {
    lastRunMillis = nowMillis;
    controller.update();
  }
}

//------------------------------------------------------------------
// Configure AceButton.
//------------------------------------------------------------------

#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL

  ButtonConfig buttonConfig;
  AceButton modeButton(&buttonConfig, MODE_BUTTON_PIN);
  AceButton changeButton(&buttonConfig, CHANGE_BUTTON_PIN);

#elif BUTTON_TYPE == BUTTON_TYPE_ANALOG

  AceButton modeButton((uint8_t) MODE_BUTTON_PIN);
  AceButton changeButton((uint8_t) CHANGE_BUTTON_PIN);
  AceButton* const BUTTONS[] = {&modeButton, &changeButton};
  const uint16_t LEVELS[] = ANALOG_BUTTON_LEVELS;

  LadderButtonConfig buttonConfig(
      ANALOG_BUTTON_PIN,
      sizeof(LEVELS) / sizeof(LEVELS[0]),
      LEVELS,
      sizeof(BUTTONS) / sizeof(BUTTONS[0]),
      BUTTONS
  );

#else

  #error Unknown BUTTON_TYPE

#endif

void handleButtonEvent(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  uint8_t pin = button->getPin();

  if (ENABLE_SERIAL_DEBUG >= 2) {
    SERIAL_PORT_MONITOR.print(F("handleButtonEvent(): eventType="));
    SERIAL_PORT_MONITOR.println(eventType);
  }

  if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventReleased:
        controller.modeButtonPress();
        break;
      case AceButton::kEventLongPressed:
        controller.modeButtonLongPress();
        break;
    }

  } else if (pin == CHANGE_BUTTON_PIN) {
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
  }
}

void setupAceButton() {
#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
  pinModeFast(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinModeFast(CHANGE_BUTTON_PIN, INPUT_PULLUP);
#endif

  buttonConfig.setEventHandler(handleButtonEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig.setRepeatPressInterval(150);
}

// Read the buttons at most every 5 ms delay because if analogRead() is used on
// an ESP8266 to read buttons in a resistor ladder, the WiFi becomes
// disconnected after 5-10 seconds. See
// https://github.com/esp8266/Arduino/issues/1634 and
// https://github.com/esp8266/Arduino/issues/5083.
void checkButtons() {
  static uint16_t lastRunMillis;

  uint16_t nowMillis = millis();
  if (nowMillis - lastRunMillis >= 5) {
    lastRunMillis = nowMillis;
  #if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
    modeButton.check();
    changeButton.check();
  #else
    buttonConfig.checkButtons();
  #endif
  }
}

//------------------------------------------------------------------
// Main setup and loop
//------------------------------------------------------------------

void setup() {
  // Wait for stability on some boards.
  // 1000ms needed for Serial.
  // 1500ms needed for Wire, I2C or SSD1306 (don't know which one).
  delay(2000);

  // Turn off the RX and TX LEDs on Leonardos
#if defined(ARDUINO_AVR_LEONARDO)
  RXLED0; // LED off
  TXLED0; // LED off
#endif

#if ENABLE_SERIAL_DEBUG >= 1
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // Wait until Serial is ready - Leonardo/Micro
  Serial.println(F("setup(): begin"));
#endif

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231 \
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH \
    || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
  Wire.begin();
#endif

  setupPersistentStore();
  setupAceButton();
  setupAceSegment();
  controller.setup();

#if ENABLE_SERIAL_DEBUG >= 1
  Serial.println(F("setup(): end"));
#endif
}

void loop() {
  checkButtons();
  updateClock();
  renderLed();
}
