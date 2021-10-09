/*
A simple digital clock using:
  * a DS3231 RTC chip
  * a 7-segment LED display
  * 2 push buttons

Supported boards are:
  * SparkFun Pro Micro
  * STM32 Blue Pill
  * ESP8266 (e.g. NodeMCU or D1Mini)
  * ESP32 (e.g. devkit v1)
*/

#include "config.h"
#include <AceSPI.h>
#include <AceTMI.h>
#include <AceWire.h>
#include <AceSegment.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceTimeClock.h>
#include "PersistentStore.h"
#include "Controller.h"

#if defined(ARDUINO_ARCH_AVR) || defined(EPOXY_DUINO)
#include <digitalWriteFast.h>
#include <ace_spi/SimpleSpiFastInterface.h>
#include <ace_tmi/SimpleTmiFastInterface.h>
#include <ace_wire/SimpleWireFastInterface.h>
#include <ace_segment/direct/DirectFast4Module.h>
#endif

using namespace ace_segment;
using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;
using namespace ace_spi;
using namespace ace_tmi;
using namespace ace_wire;

//------------------------------------------------------------------
// Configure PersistentStore
//------------------------------------------------------------------

const uint32_t kContextId = 0xbe5af950; // random contextId
const uint16_t kStoredInfoEepromAddress = 0;

PersistentStore persistentStore(kContextId, kStoredInfoEepromAddress);

void setupPersistentStore() {
  persistentStore.setup();
}

//-----------------------------------------------------------------------------
// Configure time zones and ZoneManager.
//-----------------------------------------------------------------------------

#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL

static ManualZoneManager zoneManager;

#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_BASIC

static const basic::ZoneInfo* const ZONE_REGISTRY[] ACE_TIME_PROGMEM = {
  &zonedb::kZoneAmerica_Los_Angeles,
  &zonedb::kZoneAmerica_Denver,
  &zonedb::kZoneAmerica_Chicago,
  &zonedb::kZoneAmerica_New_York,
};

static const uint16_t ZONE_REGISTRY_SIZE =
    sizeof(ZONE_REGISTRY) / sizeof(basic::ZoneInfo*);

// Only 1 displayed at any given time, need 2 for conversions.
static const uint16_t CACHE_SIZE = 1 + 1;
static BasicZoneManager<CACHE_SIZE> zoneManager(
    ZONE_REGISTRY_SIZE, ZONE_REGISTRY);

#elif TIME_ZONE_TYPE == TIME_ZONE_TYPE_EXTENDED

static const extended::ZoneInfo* const ZONE_REGISTRY[] ACE_TIME_PROGMEM = {
  &zonedbx::kZoneAmerica_Los_Angeles,
  &zonedbx::kZoneAmerica_Denver,
  &zonedbx::kZoneAmerica_Chicago,
  &zonedbx::kZoneAmerica_New_York,
};

static const uint16_t ZONE_REGISTRY_SIZE =
    sizeof(ZONE_REGISTRY) / sizeof(extended::ZoneInfo*);

// Only 1 displayed at any given time, need 2 for conversions.
static const uint16_t CACHE_SIZE = 1 + 1;
static ExtendedZoneManager<CACHE_SIZE> zoneManager(
    ZONE_REGISTRY_SIZE, ZONE_REGISTRY);

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
// Configure various Clocks
//------------------------------------------------------------------

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  #include <Wire.h> // TwoWire, Wire
  using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
  WireInterface wireInterface(Wire);
  DS3231Clock<WireInterface> dsClock(wireInterface);
  SYSTEM_CLOCK systemClock(&dsClock /*ref*/, &dsClock /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  SYSTEM_CLOCK systemClock(&ntpClock /*ref*/, nullptr /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  #include <Wire.h> // TwoWire, Wire
  using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
  WireInterface wireInterface(Wire);
  DS3231Clock<WireInterface> dsClock(wireInterface);
  NtpClock ntpClock;
  SYSTEM_CLOCK systemClock(&ntpClock /*ref*/, &dsClock /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  SYSTEM_CLOCK systemClock(nullptr /*ref*/, nullptr /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  Stm32F1Clock stm32F1Clock;
  SYSTEM_CLOCK systemClock(
      &stm32F1Clock /*ref*/, &stm32F1Clock /*backup*/, 60 /*syncPeriod*/);
#else
  #error Unknown time keeper option
#endif

void setupClocks() {
#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  dsClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  ntpClock.setup(WIFI_SSID, WIFI_PASSWORD);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STMRTC
  stmClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  stm32F1Clock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  dsClock.setup();
  ntpClock.setup(WIFI_SSID, WIFI_PASSWORD);
#endif

  systemClock.setup();
  if (systemClock.getNow() == ace_time::LocalDate::kInvalidEpochSeconds) {
    systemClock.setNow(0);
  }
}

//------------------------------------------------------------------
// Configure LED display using AceSegment.
//------------------------------------------------------------------

const uint8_t FRAMES_PER_SECOND = 60;

// The chain of resources.
#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using TmiInterface = SimpleTmiInterface;
    TmiInterface tmiInterface(DIO_PIN, CLK_PIN, BIT_DELAY);
  #else
    using TmiInterface = SimpleTmiFastInterface<DIO_PIN, CLK_PIN, BIT_DELAY>;
    TmiInterface tmiInterface;
  #endif

  Tm1637Module<TmiInterface, NUM_DIGITS> ledModule(tmiInterface);
  const uint8_t BRIGHTNESS_LEVELS = 7;
  const uint8_t BRIGHTNESS_MIN = 1;
  const uint8_t BRIGHTNESS_MAX = 7;

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
  const uint8_t NUM_DIGITS = 8;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SimpleSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SimpleSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif

  Max7219Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface, kDigitRemapArray8Max7219);

  const uint8_t BRIGHTNESS_LEVELS = 16;
  const uint8_t BRIGHTNESS_MIN = 0;
  const uint8_t BRIGHTNESS_MAX = 15;

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

  const uint8_t BRIGHTNESS_LEVELS = 16;
  const uint8_t BRIGHTNESS_MIN = 0;
  const uint8_t BRIGHTNESS_MAX = 15;

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595
  // Common Anode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 8;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SimpleSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SimpleSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif

  Hc595Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      kActiveLowPattern,
      kActiveHighPattern,
      FRAMES_PER_SECOND,
      ace_segment::kByteOrderSegmentHighDigitLow,
      ace_segment::kDigitRemapArray8Hc595
  );

  const uint8_t BRIGHTNESS_LEVELS = 1;
  const uint8_t BRIGHTNESS_MIN = 1;
  const uint8_t BRIGHTNESS_MAX = 1;

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT
  // Common Anode, with transitions on Group pins
  const uint8_t NUM_DIGITS = 4;
  const uint8_t NUM_SEGMENTS = 8;
  const uint8_t SEGMENT_PINS[NUM_SEGMENTS] = {8, 9, 10, 16, 14, 18, 19, 15};
  const uint8_t DIGIT_PINS[NUM_DIGITS] = {4, 5, 6, 7};
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    DirectModule<NUM_DIGITS> ledModule(
        kActiveLowPattern /*segmentOnPattern*/,
        kActiveLowPattern /*digitOnPattern*/,
        FRAMES_PER_SECOND,
        SEGMENT_PINS,
        DIGIT_PINS);
  #else
    DirectFast4Module<
        8, 9, 10, 16, 14, 18, 19, 15, // segment pins
        4, 5, 6, 7, // digit pins
        NUM_DIGITS
    > ledModule(
        kActiveLowPattern /*segmentOnPattern*/,
        kActiveLowPattern /*digitOnPattern*/,
        FRAMES_PER_SECOND);

  #endif

  const uint8_t BRIGHTNESS_LEVELS = 1;
  const uint8_t BRIGHTNESS_MIN = 1;
  const uint8_t BRIGHTNESS_MAX = 1;

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HYBRID
  // Common Cathode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 4;
  const uint8_t DIGIT_PINS[NUM_DIGITS] = {4, 5, 6, 7};
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SimpleSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SimpleSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  HybridModule<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      kActiveHighPattern /*segmentOnPattern*/,
      kActiveHighPattern /*digitOnPattern*/,
      FRAMES_PER_SECOND,
      DIGIT_PINS
  );

  const uint8_t BRIGHTNESS_LEVELS = 1;
  const uint8_t BRIGHTNESS_MIN = 1;
  const uint8_t BRIGHTNESS_MAX = 1;

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_FULL
  // Common Anode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SimpleSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SimpleSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  Hc595Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      kActiveLowPattern,
      kActiveLowPattern,
      FRAMES_PER_SECOND,
      ace_segment::kByteOrderDigitHighSegmentLow,
      nullptr /* remapArray */
  );

  const uint8_t BRIGHTNESS_LEVELS = 1;
  const uint8_t BRIGHTNESS_MIN = 1;
  const uint8_t BRIGHTNESS_MAX = 1;

#else
  #error Unknown LED_DISPLAY_TYPE

#endif

// Setup the various resources.
void setupAceSegment() {
  #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HYBRID \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595 \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
    spiInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
    tmiInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
    wireInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT
  #else
    #error Unknown LED_DISPLAY_TYPE
  #endif

  ledModule.begin();
}

COROUTINE(renderLed) {
  COROUTINE_LOOP() {
  #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HYBRID \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595
    ledModule.renderFieldWhenReady();
    COROUTINE_YIELD();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
    ledModule.flushIncremental();
    COROUTINE_DELAY(5);
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
    ledModule.flush();
    COROUTINE_DELAY(100);
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HT16K33
    ledModule.flush();
    COROUTINE_DELAY(100);
  #else
    #error Unknown LED_DISPLAY_TYPE
  #endif
  }
}

void setupRenderingInterrupt() {
}

//------------------------------------------------------------------
// Create an appropriate controller/presenter pair.
//------------------------------------------------------------------

Presenter presenter(ledModule);
Controller controller(systemClock, persistentStore, presenter, zoneManager,
    DISPLAY_ZONE, BRIGHTNESS_LEVELS, BRIGHTNESS_MIN, BRIGHTNESS_MAX);

//------------------------------------------------------------------
// Update the Presenter Clock periodically.
//------------------------------------------------------------------

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.display() runs as fast as or faster than 1ms for
// all DISPLAY_TYPEs. So we can set this to 100ms without worrying about too
// much overhead.
COROUTINE(updateClock) {
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
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);
#endif

  buttonConfig.setEventHandler(handleButtonEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig.setRepeatPressInterval(150);
}

// Read the buttons in a coroutine with a 5-10ms delay because if analogRead()
// is used on an ESP8266 to read buttons in a resistor ladder, the WiFi
// becomes disconnected after 5-10 seconds. See
// https://github.com/esp8266/Arduino/issues/1634 and
// https://github.com/esp8266/Arduino/issues/5083.
COROUTINE(checkButtons) {
  COROUTINE_LOOP() {
  #if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
    modeButton.check();
    changeButton.check();
  #else
    buttonConfig.checkButtons();
  #endif
    COROUTINE_DELAY(10);
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
  setupClocks();
  setupAceSegment();
  setupRenderingInterrupt();
  controller.setup();

#if ENABLE_SERIAL_DEBUG >= 1
  Serial.println(F("setup(): end"));
#endif
}

void loop() {
  checkButtons.runCoroutine();
  updateClock.runCoroutine();
  renderLed.runCoroutine();

#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  systemClock.loop();
#elif SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_COROUTINE
  systemClock.runCoroutine();
#endif
}
