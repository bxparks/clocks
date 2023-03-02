/*
 * A simple digital clock using:
 *   * a DS3231 RTC chip and/or NTP server
 *   * an SSD1306 OLED display (I2C) or a PCD8554 LCD display (SPI)
 *   * 2 push buttons
 *
 * Tested using:
 *   * Arduino Nano
 *   * Arduino Pro Mini
 *   * Arduino Pro Micro
 *   * ESP8266
 *
 * Dependencies:
 *   *  AceTime (https://github.com/bxparks/AceTime)
 *   *  AceRoutine (https://github.com/bxparks/AceRoutine)
 *   *  AceButton (https://github.com/bxparks/AceButton)
 *   *  AceCommon (https://github.com/bxparks/AceCommon)
 *   *  AceUtils (https://github.com/bxparks/AceUtils)
 *   *  AceCRC (https://github.com/bxparks/AceCRC)
 *   *  SSD1306Ascii (https://github.com/greiman/SSD1306Ascii)
 *
 * If ENABLE_SERIAL_DEBUG is set to 1, it prints diagnostics.
 */

#include "config.h"
#include <AceWire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceTimeClock.h>
#include <AceUtils.h>
#include <mode_group/mode_group.h> // from AceUtils
#include <SPI.h>
#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  #include <SSD1306AsciiAceWire.h>
#else
  #include <Adafruit_GFX.h>
  #include <Adafruit_PCD8544.h>
#endif

#if ENABLE_LED_DISPLAY
  #include <AceTMI.h>
  #include <AceSegment.h>
  #include <AceSegmentWriter.h>
  using ace_tmi::SimpleTmi1637Interface;
  using ace_segment::ClockWriter;
  using ace_segment::LedModule;
  using ace_segment::Tm1637Module;
#endif

#include "PersistentStore.h"
#include "Controller.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;

//-----------------------------------------------------------------------------
// Configure AceWire
//-----------------------------------------------------------------------------

#if DS3231_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE \
    || OLED_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE
  #include <Wire.h> // TwoWire, Wire
  using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
  WireInterface wireInterface(Wire);
#elif DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE \
    || OLED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE
  using WireInterface = ace_wire::SimpleWireInterface;
  WireInterface wireInterface(SDA_PIN, SCL_PIN, WIRE_BIT_DELAY);
#elif DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST \
    || OLED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST
  #include <digitalWriteFast.h>
  #include <ace_wire/SimpleWireFastInterface.h>
  using WireInterface = ace_wire::SimpleWireFastInterface<
      SDA_PIN, SCL_PIN, WIRE_BIT_DELAY>;
  WireInterface wireInterface;
#else
  #error Unknown DS3231_INTERFACE_TYPE or OLED_INTERFACE_TYPE
#endif

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
// Create clocks
//------------------------------------------------------------------

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231 \
    || BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  DS3231Clock<WireInterface> dsClock(wireInterface);
  Clock* refClock = &dsClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  Clock* refClock = &ntpClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP
  EspSntpClock espSntpClock;
  Clock* refClock = &espSntpClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STMRTC
  StmRtcClock stmClock;
  Clock* refClock = &stmClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  Stm32F1Clock stm32F1Clock;
  Clock* refClock = &stm32F1Clock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  Clock* refClock = nullptr;
#else
  #error Unknown TIME_SOURCE_TYPE
#endif

#if BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  Clock* backupClock = nullptr;
#elif BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  Clock* backupClock = &dsClock;
#elif BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STMRTC
  Clock* backupClock = &stmRtcClock;
#elif BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  Clock* backupClock = &stm32F1Clock;
#else
  #error Unknown BACKUP_TIME_SOURCE_TYPE
#endif

SYSTEM_CLOCK systemClock(refClock, backupClock, 60 /*syncPeriod*/);

void setupClocks() {
#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231 \
    || BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  dsClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  ntpClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP
  espSntpClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STMRTC
  stmClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  stm32F1Clock.setup();
#endif

  systemClock.setup();
}

//------------------------------------------------------------------
// Configure DHT22 temperature and humidity sensor
//------------------------------------------------------------------

#if ENABLE_DHT22

#include <DHT.h>

DHT dht(DHT22_PIN, DHT22);

void setupDht22() {
  pinMode(DHT22_PIN, INPUT_PULLUP);
  dht.begin();
}

#endif

//------------------------------------------------------------------
// Configure main OLED display or LCD display.
//------------------------------------------------------------------

#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  SSD1306AsciiAceWire<WireInterface> oled(wireInterface);

  void setupDisplay() {
    oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
    oled.displayRemap(OLED_REMAP);
    oled.setScrollMode(false);
    oled.clear();
    oled.setContrast(OLED_INITIAL_CONTRAST);
  }
#else
  Adafruit_PCD8544 lcd = Adafruit_PCD8544(LCD_SPI_DATA_COMMAND_PIN, -1, -1);

  void setupDisplay() {
    lcd.begin();

    lcd.setContrast(LCD_INITIAL_CONTRAST);
    lcd.setBias(LCD_INITIAL_BIAS);

    lcd.setTextWrap(false);
    lcd.clearDisplay();
    lcd.display();
  }
#endif

//------------------------------------------------------------------
// Configure optional LED display
//------------------------------------------------------------------

#if ENABLE_LED_DISPLAY

TmiInterface tmiInterface(DIO_PIN, CLK_PIN, TMI_BIT_DELAY);
LedDisplay ledModule(tmiInterface);
const uint8_t BRIGHTNESS_LEVELS = 7;
const uint8_t BRIGHTNESS_MIN = 1;
const uint8_t BRIGHTNESS_MAX = 7;

void setupAceSegment() {
  tmiInterface.begin();
  ledModule.begin();
}

#endif

//-----------------------------------------------------------------------------
// Create presenter
//-----------------------------------------------------------------------------

Presenter presenter(
    zoneManager,
  #if ENABLE_LED_DISPLAY
    ledModule,
  #endif
  #if DISPLAY_TYPE == DISPLAY_TYPE_OLED
    oled,
    true /*isOverwriting*/
  #else
    lcd,
    false /*isOverwriting*/
  #endif
);

void setupPresenter() {
}

//-----------------------------------------------------------------------------
// Create persistent store.
//-----------------------------------------------------------------------------

const uint32_t kContextId = 0xc42b2661; // random contextId
const uint16_t kStoredInfoEepromAddress = 0;

PersistentStore persistentStore(kContextId, kStoredInfoEepromAddress);

void setupPersistentStore() {
  persistentStore.setup();
}

//------------------------------------------------------------------
// Create controller.
//------------------------------------------------------------------

#if ENABLE_DHT22
  Controller controller(systemClock, persistentStore, presenter, zoneManager,
    DISPLAY_ZONE, &dht);
#else
  Controller controller(systemClock, persistentStore, presenter, zoneManager,
    DISPLAY_ZONE);
#endif

void setupController(bool factoryReset) {
  controller.setup(factoryReset);
}

#if ENABLE_DHT22

COROUTINE(updateTemperature) {
  COROUTINE_LOOP() {
    controller.updateTemperature();
    COROUTINE_DELAY_SECONDS(60);
  }
}

#endif

//------------------------------------------------------------------
// Render the Clock periodically.
//------------------------------------------------------------------

#if ENABLE_FPS_DEBUG
class RateMonitor {
  public:
    RateMonitor() {
      startMillis = millis();
    }

    void sample() {
      frameCounter++;
    }

    void printFrameRate() {
      unsigned long elapsedMillis = millis() - startMillis;
      Serial.print("elapsed: ");
      Serial.print(elapsedMillis);
      Serial.print("; frame count: ");
      Serial.print(frameCounter);
      float fps = frameCounter * 1000.0f / elapsedMillis;
      Serial.print("; fps: ");
      Serial.println(fps);
    }

    void reset() {
      startMillis = millis();
      frameCounter = 0;
    }

  private:
    unsigned long startMillis;
    int frameCounter = 0;
};

RateMonitor frameMonitor;
#endif

// The RTC has a resolution of only 1s, so we need to poll it fast enough to
// make it appear that the display is tracking it correctly. The benchmarking
// code says that controller.display() runs as fast as or faster than 1ms, so
// we can set this to 100ms without worrying about too much overhead.
COROUTINE(displayClock) {
  COROUTINE_LOOP() {
    controller.update();
    #if ENABLE_FPS_DEBUG
      frameMonitor.sample();
    #endif
    COROUTINE_DELAY(100);
  }
}

#if ENABLE_FPS_DEBUG
COROUTINE(printFrameRate) {
  COROUTINE_LOOP() {
    frameMonitor.reset();
    COROUTINE_DELAY(5000);
    frameMonitor.printFrameRate();
  }
}
#endif


COROUTINE(blinker) {
  COROUTINE_LOOP() {
    controller.updateBlinkState();
    COROUTINE_DELAY(500);
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

  AceButton modeButton(nullptr, (uint8_t) MODE_BUTTON_PIN);
  AceButton changeButton(nullptr, (uint8_t) CHANGE_BUTTON_PIN);
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

void handleButton(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  uint8_t pin = button->getPin();

  if (pin == CHANGE_BUTTON_PIN) {
    switch (eventType) {
      // Advance editable field by one step.
      case AceButton::kEventPressed:
        controller.handleChangeButtonPress();
        break;

      // Repeatedly advance the editable field.
      case AceButton::kEventRepeatPressed:
        controller.handleChangeButtonRepeatPress();
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
        controller.handleChangeButtonRelease();
        break;
    }
  } else if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      // Advance to the next mode.
      case AceButton::kEventReleased:
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
#if BUTTON_TYPE == BUTTON_TYPE_DIGITAL
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);
#endif

  buttonConfig.setEventHandler(handleButton);
  buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig.setRepeatPressInterval(150);

  // Shorten the DoubleClick delay to prevent accidental interpretation of
  // rapid Mode button presses as a DoubleClick.
  buttonConfig.setDoubleClickDelay(250);
}

// Read the buttons in a coroutine with a 5-10ms delay because if analogRead()
// is used on an ESP8266 to read buttons in a resistor ladder, the WiFi
// becomes disconnected after 5-10 seconds. See
// https://github.com/esp8266/Arduino/issues/1634 and
// https://github.com/esp8266/Arduino/issues/5083.
COROUTINE(readButtons) {
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
// Setup WiFi if necessary.
//------------------------------------------------------------------

#if defined(ESP8266) || defined(ESP32)

// Number of millis to wait for a WiFi connection before doing a software
// reboot.
static const unsigned long REBOOT_TIMEOUT_MILLIS = 15000;

// Connect to WiFi. Sometimes the board will connect instantly. Sometimes it
// will struggle to connect. I don't know why. Performing a software reboot
// seems to help, but not always.
void setupWiFi() {
  Serial.print(F("Connecting to WiFi"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startMillis = millis();
  while (true) {
    Serial.print('.');
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F(" Done."));
      break;
    }

    // Detect timeout and reboot.
    unsigned long nowMillis = millis();
    if ((unsigned long) (nowMillis - startMillis) >= REBOOT_TIMEOUT_MILLIS) {
    #if defined(ESP8266)
      Serial.println(F("FAILED! Rebooting.."));
      delay(1000);
      ESP.reset();
    #elif defined(ESP32)
      Serial.println(F("FAILED! Rebooting.."));
      delay(1000);
      ESP.restart();
    #else
      Serial.println(F("FAILED! But cannot reboot.. continuing.."));
      delay(1000);
      startMillis = nowMillis;
    #endif
    }

    delay(500);
  }
}

#endif

//------------------------------------------------------------------
// Setup Wire.
//------------------------------------------------------------------

void setupWire() {
  #if OLED_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE \
      || DS3231_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE
    Wire.begin();
    Wire.setClock(400000L);
  #endif

  wireInterface.begin();
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

if (ENABLE_SERIAL_DEBUG >= 1 || ENABLE_FPS_DEBUG >= 1) {
  SERIAL_PORT_MONITOR.begin(115200);
  while (!SERIAL_PORT_MONITOR); // Leonardo/Micro
}

if (ENABLE_SERIAL_DEBUG >= 1) {
  SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  SERIAL_PORT_MONITOR.print(F("sizeof(ClockInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(ClockInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(StoredInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(StoredInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(RenderingInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(RenderingInfo));
}

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP \
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP
  setupWiFi();
#endif
  setupWire();
  setupPersistentStore();
  setupAceButton();
  setupClocks();
  setupDisplay();
  setupPresenter();

#if ENABLE_DHT22
  setupDht22();
#endif

#if ENABLE_LED_DISPLAY
  setupAceSegment();
#endif

  // Hold down the Mode button to perform factory reset.
  bool isModePressedDuringBoot = modeButton.isPressedRaw();
  setupController(isModePressedDuringBoot);

  if (ENABLE_SERIAL_DEBUG >= 1) {
    SERIAL_PORT_MONITOR.println(F("setup(): end"));
  }
}

void loop() {
  readButtons.runCoroutine();
  blinker.runCoroutine();

#if ENABLE_DHT22
  updateTemperature.runCoroutine();
#endif

  displayClock.runCoroutine();

#if ENABLE_FPS_DEBUG
  printFrameRate.runCoroutine();
#endif

#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  systemClock.loop();
#else
  systemClock.runCoroutine();
#endif
}
