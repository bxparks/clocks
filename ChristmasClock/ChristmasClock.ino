/*
A clock that counts down the number of days to Christmas. Uses the following
hardware:

  * 1 x D1 Mini
  * 1 x DS3231 RTC chip
  * 1 x TM1637 4-digit 7-segment LED display
  * 2 x push buttons
*/

#include "config.h"
#include <AceTMI.h>
#include <AceWire.h>
#include <AceSegment.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceTimeClock.h>
#include "PersistentStore.h"
#include "Controller.h"

using namespace ace_segment;
using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;
using namespace ace_tmi;
using namespace ace_wire;

//------------------------------------------------------------------
// Configure PersistentStore
//------------------------------------------------------------------

const uint32_t kContextId = 0x34ce0b3e; // random contextId
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
static BasicZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static BasicZoneManager zoneManager(
    ZONE_REGISTRY_SIZE, ZONE_REGISTRY, zoneProcessorCache);

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
// Configure AceWire
//------------------------------------------------------------------

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231 \
    || BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  #if DS3231_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE
    #include <Wire.h> // TwoWire, Wire
    using WireInterface = ace_wire::TwoWireInterface<TwoWire>;
    WireInterface wireInterface(Wire);
  #elif DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE
    using WireInterface = ace_wire::SimpleWireInterface;
    WireInterface wireInterface(SDA_PIN, SCL_PIN, WIRE_BIT_DELAY);
  #elif DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST
    using WireInterface = ace_wire::SimpleWireFastInterface<
        SDA_PIN, SCL_PIN, WIRE_BIT_DELAY>;
    WireInterface wireInterface;
  #else
    #error Unknown DS3231_INTERFACE_TYPE
  #endif

//------------------------------------------------------------------
// Configure various Clocks
//------------------------------------------------------------------

  DS3231Clock<WireInterface> dsClock(wireInterface);
  Clock* refClock = &dsClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  Clock* refClock = &ntpClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP
  EspSntpClock espSntpClock;
  Clock* refClock = &espSntpClock;
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  Clock* refClock = nullptr;
#else
  #error Unknown TIME_SOURCE_TYPE
#endif

#if BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  Clock* backupClock = nullptr;
#elif BACKUP_TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  Clock* backupClock = &dsClock;
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
#endif

  systemClock.setup();
}

//------------------------------------------------------------------
// Configure LED display using AceSegment.
//------------------------------------------------------------------

// The chain of resources.
#if LED_DISPLAY_TYPE == LED_DISPLAY_TYPE_TM1637
  #if LED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_TMI
    using TmiInterface = SimpleTmi1637Interface;
    TmiInterface tmiInterface(DIO_PIN, CLK_PIN, TMI_BIT_DELAY);
  #elif LED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_TMI_FAST
    using TmiInterface = SimpleTmi1637FastInterface<
        DIO_PIN, CLK_PIN, TMI_BIT_DELAY>;
    TmiInterface tmiInterface;
  #else
    #error Unknown LED_INTERFACE_TYPE
  #endif

  const uint8_t NUM_DIGITS = 4;
  Tm1637Module<TmiInterface, NUM_DIGITS> ledModule(tmiInterface);
  const uint8_t BRIGHTNESS_LEVELS = 8;
  const uint8_t BRIGHTNESS_MIN = 0;
  const uint8_t BRIGHTNESS_MAX = 7;

#else
  #error Unknown LED_DISPLAY_TYPE

#endif

// Setup the various resources.
void setupAceSegment() {
  tmiInterface.begin();
  ledModule.begin();
}

COROUTINE(renderLed) {
  COROUTINE_LOOP() {
    ledModule.flushIncremental();
    COROUTINE_DELAY(20);
  }
}

//------------------------------------------------------------------
// Create an appropriate controller/presenter pair.
//------------------------------------------------------------------

Presenter presenter(zoneManager, ledModule);
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
        controller.handleModeButtonPress();
        break;
      case AceButton::kEventLongPressed:
        controller.handleModeButtonLongPress();
        break;
    }

  } else if (pin == CHANGE_BUTTON_PIN) {
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
// Setup WiFi if necessary.
//------------------------------------------------------------------

#if defined(ESP8266) || defined(ESP32)
#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP \
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP

// Number of millis to wait for WiFi connection before doing a software reboot.
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
#endif

//------------------------------------------------------------------
// Setup Wire.
//------------------------------------------------------------------

void setupWire() {
#if DS3231_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE \
    || LED_INTERFACE_TYPE == INTERFACE_TYPE_TWO_WIRE
  Wire.begin();
  wireInterface.begin();
#elif LED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE \
    || LED_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST \
    || DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FAST \
    || DS3231_INTERFACE_TYPE == INTERFACE_TYPE_SIMPLE_WIRE_FASt
  wireInterface.begin();
#endif
}

//------------------------------------------------------------------
// Main setup and loop
//------------------------------------------------------------------

void setup() {
  // Wait for stability on some boards.
  // 1000ms needed for Serial.
  // 1500ms needed for Wire, I2C or SSD1306 (don't know which one).
  delay(2000);

#if ENABLE_SERIAL_DEBUG >= 1
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // Wait until Serial is ready - Leonardo/Micro
  Serial.println(F("setup(): begin"));
#endif

#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP \
    || TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_ESP_SNTP
  setupWiFi();
#endif
  setupWire();
  setupPersistentStore();
  setupAceButton();
  setupClocks();
  setupAceSegment();
  controller.setup();

#if ENABLE_SERIAL_DEBUG >= 1
  Serial.println(F("setup(): end"));
#endif
}

void loop() {
  checkButtons.runCoroutine();
  blinker.runCoroutine();
  updateClock.runCoroutine();
  renderLed.runCoroutine();

#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  systemClock.loop();
#elif SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_COROUTINE
  systemClock.runCoroutine();
#endif
}
