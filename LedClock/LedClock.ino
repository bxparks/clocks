/*
A simple digital clock using:
  * a DS3231 RTC chip
  * a 7-segment LED display, OR an SSD1306 OLED display
  * 2 push buttons

Supported boards are:
  * Arduino Nano
  * Arduino Pro Mini
  * Arduino Leonardo (Pro Micro clone)
*/

#include "config.h"
#include <Wire.h>
#include <AceSegment.h>
#if defined(ARDUINO_ARCH_AVR) || defined(EPOXY_DUINO)
#include <digitalWriteFast.h>
#include <ace_segment/hw/SwSpiAdapterFast.h>
#include <ace_segment/scanning/LedMatrixDirectFast.h>
#include <ace_segment/tm1637/Tm1637DriverFast.h>
#endif
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceUtilsCrcEeprom.h>
#include <SSD1306AsciiWire.h>
#include "Controller.h"

using namespace ace_segment;
using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;
using ace_utils::crc_eeprom::CrcEeprom;
using ace_utils::crc_eeprom::AvrEepromAdapter;
using ace_utils::crc_eeprom::EspEepromAdapter;

//------------------------------------------------------------------
// Configure CrcEeprom.
//------------------------------------------------------------------

const int EEPROM_SIZE = CrcEeprom::toSavedSize(sizeof(StoredInfo));

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
  #error No EEPROM
#endif
CrcEeprom crcEeprom(eepromAdapter, CrcEeprom::toContextId('l', 'c', 'l', 'k'));

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
  systemClock.setupCoroutine(F("clock"));
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
const uint8_t NUM_SUBFIELDS = 16;
const uint8_t NUM_DIGITS = 4;
const uint8_t DIGIT_PINS[NUM_DIGITS] = {4, 5, 6, 7};

#if LED_MATRIX_MODE == LED_MATRIX_MODE_DIRECT
  // 4 digits, resistors on segments on Pro Micro.
  const uint8_t NUM_SEGMENTS = 8;
  const uint8_t SEGMENT_PINS[NUM_SEGMENTS] = {8, 9, 10, 16, 14, 18, 19, 15};
#else
  const uint8_t LATCH_PIN = 10; // ST_CP on 74HC595
  const uint8_t DATA_PIN = MOSI; // DS on 74HC595
  const uint8_t CLOCK_PIN = SCK; // SH_CP on 74HC595
#endif

#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  const uint8_t CLK_PIN = 10;
  const uint8_t DIO_PIN = 9;
  const uint16_t BIT_DELAY = 100;
#endif

// The chain of resources.
#if LED_MATRIX_MODE == LED_MATRIX_MODE_DIRECT
  // Common Anode, with transitions on Group pins
  using LedMatrix = LedMatrixDirect<>;
  LedMatrix ledMatrix(
      LedMatrix::kActiveLowPattern /*groupOnPattern*/,
      LedMatrix::kActiveLowPattern /*elementOnPattern*/,
      NUM_DIGITS,
      DIGIT_PINS,
      NUM_SEGMENTS,
      SEGMENT_PINS);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_DIRECT_FAST
  // Common Anode, with transitions on Group pins
  using LedMatrix = LedMatrixDirectFast<
    4, 5, 6, 7,
    8, 9, 10, 16, 14, 18, 19, 15
  >;
  LedMatrix ledMatrix(
      LedMatrix::kActiveLowPattern /*groupOnPattern*/,
      LedMatrix::kActiveLowPattern /*elementOnPattern*/);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_PARIAL_SW_SPI
  // Common Cathode, with transistors on Group pins
  SwSpiAdapter spiAdapter(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  using LedMatrix = LedMatrixSingleShiftRegister<SwSpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveHighPattern /*groupOnPattern*/,
      LedMatrix::kActiveHighPattern /*elementOnPattern*/,
      NUM_DIGITS,
      DIGIT_PINS):
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_SINGLE_SW_SPI_FAST
  // Common Cathode, with transistors on Group pins
  using SpiAdapter = SwSpiAdapterFast<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
  SpiAdapter spiAdapter;
  using LedMatrix = LedMatrixSingleShiftRegister<SpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveHighPattern /*groupOnPattern*/,
      LedMatrix::kActiveHighPattern /*elementOnPattern*/,
      NUM_DIGITS,
      DIGIT_PINS);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_SINGLE_HW_SPI
  // Common Cathode, with transistors on Group pins
  HwSpiAdapter spiAdapter(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  using LedMatrix = LedMatrixSingleShiftRegister<HwSpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveHighPattern /*groupOnPattern*/,
      LedMatrix::kActiveHighPattern /*elementOnPattern*/,
      NUM_DIGITS,
      DIGIT_PINS);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_SW_SPI
  // Common Anode, with transistors on Group pins
  SwSpiAdapter spiAdapter(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  using LedMatrix = LedMatrixDualShiftRegister<SwSpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveLowPattern /*groupOnPattern*/,
      LedMatrix::kActiveLowPattern /*elementOnPattern*/);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_SW_SPI_FAST
  // Common Anode, with transistors on Group pins
  using SpiAdapter = SwSpiAdapterFast<LATCH_PIN, DATA_PIN, CLOCK_PIN>;
  SpiAdapter spiAdapter;
  using LedMatrix = LedMatrixDualShiftRegister<SpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveLowPattern /*groupOnPattern*/,
      LedMatrix::kActiveLowPattern /*elementOnPattern*/);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_HW_SPI
  // Common Anode, with transistors on Group pins
  HwSpiAdapter spiAdapter(LATCH_PIN, DATA_PIN, CLOCK_PIN);
  using LedMatrix = LedMatrixDualShiftRegister<HwSpiAdapter>;
  LedMatrix ledMatrix(
      spiAdapter,
      LedMatrix::kActiveLowPattern /*groupOnPattern*/,
      LedMatrix::kActiveLowPattern /*elementOnPattern*/);
#elif LED_MATRIX_MODE == LED_MATRIX_MODE_NONE
  // Do nothing
#else
  #error Unsupported LED_MATRIX_MODE
#endif

#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_SCANNING
  ScanningModule<LedMatrix, NUM_DIGITS, 1> module(
      ledMatrix, FRAMES_PER_SECOND);
  LedDisplay display(module);

#elif LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  #if TM1637_DRIVER_TYPE == TM1637_DRIVER_TYPE_NORMAL
    using TmDriver = Tm1637Driver
    TmDriver driver(CLK_PIN, DIO_PIN, BIT_DELAY);
    Tm1637Module<TmDriver, 4> module(driver);
  #else
    using TmDriver = Tm1637DriverFast<CLK_PIN, DIO_PIN, BIT_DELAY>;
    TmDriver driver;
    Tm1637Module<TmDriver, 4> module(driver);
  #endif
  LedDisplay display(module);

#else
  #error Unknown LED_DISPLAY_TYPE
#endif

// Setup the various resources.
void setupAceSegment() {
  #if LED_MATRIX_MODE == LED_MATRIX_MODE_PARIAL_SW_SPI \
      || LED_MATRIX_MODE == LED_MATRIX_MODE_SINGLE_HW_SPI \
      || LED_MATRIX_MODE == LED_MATRIX_MODE_SINGLE_SW_SPI_FAST \
      || LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_SW_SPI \
      || LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_HW_SPI \
      || LED_MATRIX_MODE == LED_MATRIX_MODE_DUAL_SW_SPI_FAST
    spiAdapter.begin();
  #endif

  #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
    driver.begin();
    module.begin();
  #else
    ledMatrix.begin();
    module.begin();
  #endif
}

#if USE_INTERRUPT == 1
  // interrupt handler for timer 2
  ISR(TIMER2_COMPA_vect) {
    module.renderFieldNow();
  }
#else
  COROUTINE(renderLed) {
    COROUTINE_LOOP() {
    #if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_SCANNING
      module.renderFieldWhenReady();
    #else
      module.flushIncremental();
    #endif
      COROUTINE_YIELD();
    }
  }
#endif

void setupRenderingInterrupt() {
#if USE_INTERRUPT == 1
  // set up Timer 2
  uint8_t timerCompareValue =
      (long) F_CPU / 1024 / module.getFieldsPerSecond() - 1;
  #if ENABLE_SERIAL_DEBUG == 1
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
// Render the Clock periodically.
//------------------------------------------------------------------

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.display() runs as fast as or faster than 1ms for
// all DISPLAY_TYPEs. So we can set this to 100ms without worrying about too
// much overhead.
COROUTINE(displayClock) {
  COROUTINE_LOOP() {
    controller.update();
    COROUTINE_DELAY(100);
  }
}

//------------------------------------------------------------------
// Configure AceButton.
//------------------------------------------------------------------

ButtonConfig modeButtonConfig;
AceButton modeButton(&modeButtonConfig);

ButtonConfig changeButtonConfig;
AceButton changeButton(&changeButtonConfig);

void handleModeButton(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  if (ENABLE_SERIAL_DEBUG >= 2) {
    SERIAL_PORT_MONITOR.print(F("handleModeButton(): eventType="));
    SERIAL_PORT_MONITOR.println(eventType);
  }

  switch (eventType) {
    case AceButton::kEventReleased:
      controller.modeButtonPress();
      break;
    case AceButton::kEventLongPressed:
      controller.modeButtonLongPress();
      break;
  }
}

void handleChangeButton(AceButton* /* button */, uint8_t eventType,
    uint8_t /* buttonState */) {
  if (ENABLE_SERIAL_DEBUG >= 2) {
    SERIAL_PORT_MONITOR.print(F("handleChangeButton(): eventType="));
    SERIAL_PORT_MONITOR.println(eventType);
  }

  switch (eventType) {
    case AceButton::kEventPressed:
      controller.changeButtonPress();
      break;
    case AceButton::kEventReleased:
      controller.changeButtonRelease();
      break;
    case AceButton::kEventRepeatPressed:
      controller.changeButtonRepeatPress();
      break;
  }
}

void setupAceButton() {
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);

  modeButton.init(MODE_BUTTON_PIN);
  changeButton.init(CHANGE_BUTTON_PIN);

  modeButtonConfig.setEventHandler(handleModeButton);
  modeButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  modeButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  changeButtonConfig.setEventHandler(handleChangeButton);
  changeButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  changeButtonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  changeButtonConfig.setRepeatPressInterval(150);
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

#if ENABLE_SERIAL_DEBUG == 1
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // Wait until Serial is ready - Leonardo/Micro
  Serial.println(F("setup(): begin"));
#endif

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231 \
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  Wire.begin();
  Wire.setClock(400000L);
#endif

  crcEeprom.begin(EEPROM_SIZE);
  setupAceButton();
  setupClocks();
  setupAceSegment();
  setupRenderingInterrupt();
  controller.setup();

  CoroutineScheduler::setup();

#if ENABLE_SERIAL_DEBUG == 1
  Serial.println(F("setup(): end"));
#endif
}

void loop() {
  CoroutineScheduler::loop();

  modeButton.check();
  changeButton.check();
}
