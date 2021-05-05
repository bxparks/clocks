/*
A simple digital clock using:
  * a DS3231 RTC chip
  * a 7-segment LED display
  * 2 push buttons

Supported boards are:
  * SparkFun Pro Micro w/ Led Modules
  * ESP8266 dev board w/ Led Modules
*/

#include "config.h"
#include <Wire.h>
#include <AceSegment.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include <SSD1306AsciiWire.h>
#include "Controller.h"

#if defined(ARDUINO_ARCH_AVR) || defined(EPOXY_DUINO)
#include <digitalWriteFast.h>
#include <ace_segment/hw/SoftSpiFastInterface.h>
#include <ace_segment/hw/SoftWireFastInterface.h>
#include <ace_segment/direct/DirectFast4Module.h>
#endif

using namespace ace_segment;
using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;
using ace_utils::crc_eeprom::AvrStyleEeprom;
using ace_utils::crc_eeprom::EspStyleEeprom;
using ace_utils::crc_eeprom::CrcEeprom;

//------------------------------------------------------------------
// Configure CrcEeprom.
//------------------------------------------------------------------

#if defined(EPOXY_DUINO)
  #include <EpoxyEepromEsp.h>
  EspStyleEeprom<EpoxyEepromEsp> eepromInterface(EpoxyEepromEspInstance);
#elif defined(ESP32) || defined(ESP8266) 
  #include <EEPROM.h>
  EspStyleEeprom<EEPROMClass> eepromInterface(EEPROM);
#elif defined(ARDUINO_ARCH_AVR)
  #include <EEPROM.h>
  AvrStyleEeprom<EEPROMClass> eepromInterface(EEPROM);
#elif defined(ARDUINO_ARCH_STM32)
  #include <AceUtilsBufferedEepromStm32.h>
  EspStyleEeprom<BufferedEEPROMClass> eepromInterface(BufferedEEPROM);
#else
  #error No EEPROM
#endif

CrcEeprom crcEeprom(
    eepromInterface, CrcEeprom::toContextId('l', 'c', 'l', 'k'));

void setupEeprom() {
#if defined(EPOXY_DUINO)
  EpoxyEepromEspInstance.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
#elif defined(ESP32) || defined(ESP8266)
  EEPROM.begin(CrcEeprom::toSavedSize(sizeof(StoredInfo)));
#elif defined(ARDUINO_ARCH_AVR)
  // no setup required
#elif defined(ARDUINO_ARCH_STM32)
  BufferedEEPROM.begin();
#endif
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
  DS3231Clock dsClock;
  SYSTEM_CLOCK systemClock(&dsClock /*ref*/, &dsClock /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  SYSTEM_CLOCK systemClock(&ntpClock /*ref*/, nullptr /*backup*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  DS3231Clock dsClock;
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

// Use polling or interrupt for AceSegment
#define USE_INTERRUPT 0

const uint8_t FRAMES_PER_SECOND = 60;

// The chain of resources.
#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using WireInterface = SoftWireInterface;
    WireInterface wireInterface(CLK_PIN, DIO_PIN, BIT_DELAY);
    Tm1637Module<WireInterface, NUM_DIGITS> ledModule(wireInterface);
  #else
    using WireInterface = SoftWireFastInterface<CLK_PIN, DIO_PIN, BIT_DELAY>;
    WireInterface wireInterface;
    Tm1637Module<WireInterface, NUM_DIGITS> ledModule(wireInterface);
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
      spiInterface, kDigitRemapArray8);

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT
  // Common Anode, with transitions on Group pins
  const uint8_t NUM_DIGITS = 4;
  const uint8_t NUM_SEGMENTS = 8;
  const uint8_t SEGMENT_PINS[NUM_SEGMENTS] = {8, 9, 10, 16, 14, 18, 19, 15};
  const uint8_t DIGIT_PINS[NUM_DIGITS] = {4, 5, 6, 7};
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    DirectModule<NUM_DIGITS> ledModule(
        LedMatrixBase::kActiveLowPattern /*segmentOnPattern*/,
        LedMatrixBase::kActiveLowPattern /*digitOnPattern*/,
        FRAMES_PER_SECOND,
        SEGMENT_PINS,
        DIGIT_PINS);
  #else
    DirectFast4Module<
        8, 9, 10, 16, 14, 18, 19, 15, // segment pins
        4, 5, 6, 7, // digit pins
        NUM_DIGITS
    > ledModule(
        LedMatrixBase::kActiveLowPattern /*segmentOnPattern*/,
        LedMatrixBase::kActiveLowPattern /*digitOnPattern*/,
        FRAMES_PER_SECOND);

  #endif

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_SINGLE
  // Common Cathode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 4;
  const uint8_t DIGIT_PINS[NUM_DIGITS] = {4, 5, 6, 7};
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SoftSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SoftSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  SingleHc595Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      LedMatrixBase::kActiveHighPattern /*segmentOnPattern*/,
      LedMatrixBase::kActiveHighPattern /*digitOnPattern*/,
      FRAMES_PER_SECOND,
      DIGIT_PINS
  );

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_DUAL
  // Common Anode, with transistors on Group pins
  const uint8_t NUM_DIGITS = 4;
  #if INTERFACE_TYPE == INTERFACE_TYPE_NORMAL
    using SpiInterface = SoftSpiInterface;
    SpiInterface spiInterface(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  #else
    using SpiInterface = SoftSpiFastInterface<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
    SpiInterface spiInterface;
  #endif
  DualHc595Module<SpiInterface, NUM_DIGITS> ledModule(
      spiInterface,
      LedMatrixBase::kActiveLowPattern /*segmentOnPattern*/,
      LedMatrixBase::kActiveLowPattern /*digitOnPattern*/,
      FRAMES_PER_SECOND
  );

#else
  #error Unknown LED_DISPLAY_TYPE

#endif

LedDisplay display(ledModule);

// Setup the various resources.
void setupAceSegment() {
  #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_SINGLE \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_DUAL \
      || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
    spiInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
    wireInterface.begin();
  #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT
  #else
    #error Unknown LED_DISPLAY_TYPE
  #endif
  ledModule.begin();
}

#if USE_INTERRUPT == 1
  // interrupt handler for timer 2
  ISR(TIMER2_COMPA_vect) {
    ledModule.renderFieldNow();
  }
#else
  COROUTINE(renderLed) {
    COROUTINE_LOOP() {
    #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_DIRECT \
        || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_SINGLE \
        || LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_HC595_DUAL
      ledModule.renderFieldWhenReady();
    #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
      ledModule.flushIncremental();
    #elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_MAX7219
      ledModule.flush();
    #endif

      COROUTINE_YIELD();
    }
  }
#endif

void setupRenderingInterrupt() {
#if USE_INTERRUPT == 1
  // set up Timer 2
  uint8_t timerCompareValue =
      (long) F_CPU / 1024 / ledModule.getFieldsPerSecond() - 1;
  #if ENABLE_SERIAL_DEBUG >= 1
    Serial.print(F("Timer 2, Compare A: "));
    Serial.println(timerCompareValue);
  #endif

  noInterrupts();
  TCNT2  = 0;	// Initialize counter value to 0
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2A |= bit(WGM21); // CTC
  TCCR2B |= bit(CS22) | bit(CS21) | bit(CS20); // prescale 1024
  TIMSK2 |= bit(OCIE2A); // interrupt on Compare A Match
  OCR2A =  timerCompareValue;
  interrupts();
#endif
}

//------------------------------------------------------------------
// Create an appropriate controller/presenter pair.
//------------------------------------------------------------------

Presenter presenter(display);
Controller controller(systemClock, crcEeprom, presenter, zoneManager,
    DISPLAY_ZONE);

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

  #if ANALOG_BUTTON_COUNT == 2
    #if ANALOG_BITS == 8
      const uint16_t LEVELS[] = {0, 128, 255};
    #elif ANALOG_BITS == 10
      const uint16_t LEVELS[] = {0, 512, 1023};
    #else
      #error Unknown number of ADC bits
    #endif
  #elif ANALOG_BUTTON_COUNT == 4
    #if ANALOG_BITS == 8
      const uint16_t LEVELS[] = {
        0 /*short to ground*/,
        82 /*32%, 4.7k*/,
        128 /*50%, 10k*/,
        210 /*82%, 47k*/,
        255 /*100%, open*/
      };
    #elif ANALOG_BITS == 10
      const uint16_t LEVELS[] = {
        0 /*short to ground*/,
        327 /*32%, 4.7k*/,
        512 /*50%, 10k*/,
        844 /*82%, 47k*/,
        1023 /*100%, open*/
      };
    #else
      #error Unknown number of ADC bits
    #endif
  #else
    #error Unknown ANALOG_BUTTON_COUNT
  #endif

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
    COROUTINE_DELAY(5);
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
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  Wire.begin();
  Wire.setClock(400000L);
#endif

  setupEeprom();
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
