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
 *   * [AceTime](https://github.com/bxparks/AceTime)
 *   * [AceRoutine](https://github.com/bxparks/AceRoutine)
 *   * [AceButton](https://github.com/bxparks/AceButton)
 *   * [FastCRC](https://github.com/FrankBoesing/FastCRC)
 *   * [SSD1306Ascii](https://github.com/greiman/SSD1306Ascii)
 *
 * If ENABLE_SERIAL_DEBUG is set to 1, it prints diagnostics.
 */

#include "config.h"
#include <Wire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <SPI.h>
#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  #include <SSD1306AsciiWire.h>
#else
  #include <Adafruit_GFX.h>
  #include <Adafruit_PCD8544.h>
#endif
#include "PersistentStore.h"
#include "Controller.h"
#include "ModeGroup.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;

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
// Create clocks
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

void setupClocks() {
#if TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_DS3231
  dsClock.setup();
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  ntpClock.setup(AUNITER_SSID, AUNITER_PASSWORD);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  dsClock.setup();
  ntpClock.setup(AUNITER_SSID, AUNITER_PASSWORD);
#endif
  systemClock.setup();
  systemClock.setupCoroutine(F("clock"));
}

//------------------------------------------------------------------
// Configure OLED display or LCD display.
//------------------------------------------------------------------

#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  SSD1306AsciiWire oled;

  void setupDisplay() {
    oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
    oled.displayRemap(OLED_REMAP);
    oled.setScroll(false);
    oled.clear();
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

//-----------------------------------------------------------------------------
// Create presenter
//-----------------------------------------------------------------------------

#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  Presenter presenter(zoneManager, oled, true /*isOverwriting*/);
#else
  Presenter presenter(zoneManager, lcd, false /*isOverwriting*/);
#endif

void setupPresenter() {
}

//-----------------------------------------------------------------------------
// Create mode groups that define the navigation path for the Mode button.
// It forms a recursive tree structure that looks like this:
//
// - View date time
//    - Change hour
//    - Change minute
//    - Change second
//    - Change day
//    - Change month
//    - Change year
// - View TimeZone
//    - Change zone offset
//    - Change zone dst
//    - Change zone name
// - About
//
// Operation:
//
// * A Press of the Mode button cycles through the sibling modes.
// * A LongPress of the Mode button goes down or up the hierarchy. Since the
// hierarchy is only 2-levels deep, we can use LongPress to alternate going up
// or down the mode tree. In the general case, we would need a different button
// event (e.g. double click?) to distinguish between going up or going down the
// tree.
//
// Previous version of this encoded the navigation tree within the Controller.h
// class itself, in the various switch statements. However, I found it to be
// too difficult to maintain when modes or their ordering changed. This
// solution defines the mode hierarchy in a data-driven way, so should be
// easier to maintain.
//-----------------------------------------------------------------------------

// List of DateTime modes.
const uint8_t DATE_TIME_MODES[] = {
  MODE_CHANGE_YEAR,
  MODE_CHANGE_MONTH,
  MODE_CHANGE_DAY,
  MODE_CHANGE_HOUR,
  MODE_CHANGE_MINUTE,
  MODE_CHANGE_SECOND,
  0,
};

// List of TimeZone modes.
const uint8_t TIME_ZONE_MODES[] = {
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
  MODE_CHANGE_TIME_ZONE_OFFSET,
  MODE_CHANGE_TIME_ZONE_DST,
#else
  MODE_CHANGE_TIME_ZONE_NAME,
#endif
  0,
};

extern const ModeGroup rootModeGroup;

// ModeGroup for the DateTime modes.
const ModeGroup dateTimeModeGroup = {
  &rootModeGroup /* parentGroup */,
  DATE_TIME_MODES /* modes */,
  nullptr /* childGroups */,
};

// MOdeGroup for the TimeZone modes.
const ModeGroup timeZoneModeGroup = {
  &rootModeGroup /* parentGroup */,
  TIME_ZONE_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of top level modes.
const uint8_t TOP_LEVEL_MODES[] = {
  MODE_DATE_TIME,
  MODE_TIME_ZONE,
  MODE_ABOUT,
  0,
};

// List of children ModeGroups for each element in TOP_LEVEL_MODES, in the same
// order.
const ModeGroup* const TOP_LEVEL_CHILD_GROUPS[] = {
  &dateTimeModeGroup,
  &timeZoneModeGroup,
  nullptr /* About mode has no submodes */,
};

// Root mode group
const ModeGroup rootModeGroup = {
  nullptr /* parentGroup */,
  TOP_LEVEL_MODES /* modes */,
  TOP_LEVEL_CHILD_GROUPS /* childGroups */,
};

//-----------------------------------------------------------------------------
// Create persistent store.
//-----------------------------------------------------------------------------

PersistentStore persistentStore;

void setupPersistentStore() {
  persistentStore.setup();
}

//------------------------------------------------------------------
// Create controller.
//------------------------------------------------------------------

Controller controller(persistentStore, systemClock, presenter, zoneManager,
  DISPLAY_ZONE, &rootModeGroup);

void setupController(bool factoryReset) {
  controller.setup(factoryReset);
}

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
    const uint16_t LEVELS[] = {0, 128, 255};
  #elif ANALOG_BITS == 10
    const uint16_t LEVELS[] = {0, 512, 1023};
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

if (ENABLE_SERIAL_DEBUG == 1 || ENABLE_FPS_DEBUG == 1) {
  SERIAL_PORT_MONITOR.begin(115200);
  while (!SERIAL_PORT_MONITOR); // Leonardo/Micro
}

if (ENABLE_SERIAL_DEBUG == 1) {
  SERIAL_PORT_MONITOR.println(F("setup(): begin"));
  SERIAL_PORT_MONITOR.print(F("sizeof(ClockInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(ClockInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(StoredInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(StoredInfo));
  SERIAL_PORT_MONITOR.print(F("sizeof(RenderingInfo): "));
  SERIAL_PORT_MONITOR.println(sizeof(RenderingInfo));
}

  Wire.begin();
  Wire.setClock(400000L);

  setupAceButton();
  setupClocks();
  setupDisplay();
  setupPresenter();
  setupPersistentStore();

  // Hold down the Mode button to perform factory reset.
  bool isModePressedDuringBoot = modeButton.isPressedRaw();
  setupController(isModePressedDuringBoot);

  CoroutineScheduler::setup();

  if (ENABLE_SERIAL_DEBUG == 1) {
    SERIAL_PORT_MONITOR.println(F("setup(): end"));
  }
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
