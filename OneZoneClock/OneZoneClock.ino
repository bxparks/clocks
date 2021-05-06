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
#include <Wire.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include <AceTime.h>
#include <AceUtilsModeGroup.h>
#include <SPI.h>
#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  #include <SSD1306AsciiWire.h>
#else
  #include <Adafruit_GFX.h>
  #include <Adafruit_PCD8544.h>
#endif
#include "PersistentStore.h"
#include "Controller.h"

using namespace ace_button;
using namespace ace_routine;
using namespace ace_time;
using namespace ace_time::clock;
using ace_utils::mode_group::ModeGroup;

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
  SYSTEM_CLOCK systemClock(&dsClock, &dsClock, 60 /*syncPeriod*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NTP
  NtpClock ntpClock;
  SYSTEM_CLOCK systemClock(&ntpClock, nullptr, 60 /*syncPeriod*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STMRTC
  StmRtcClock stmClock;
  SYSTEM_CLOCK systemClock(&stmClock, nullptr, 60 /*syncPeriod*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_STM32F1RTC
  Stm32F1Clock stm32F1Clock;
  SYSTEM_CLOCK systemClock(&stm32F1Clock, nullptr, 60 /*syncPeriod*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_BOTH
  DS3231Clock dsClock;
  NtpClock ntpClock;
  SYSTEM_CLOCK systemClock(&ntpClock, &dsClock, 60 /*syncPeriod*/);
#elif TIME_SOURCE_TYPE == TIME_SOURCE_TYPE_NONE
  SYSTEM_CLOCK systemClock(nullptr /*sync*/, nullptr /*backup*/);
#else
  #error Unknown clock option
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
#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_COROUTINE
  systemClock.setupCoroutine(F("clock"));
#endif
}

//------------------------------------------------------------------
// Configure OLED display or LCD display.
//------------------------------------------------------------------

#if DISPLAY_TYPE == DISPLAY_TYPE_OLED
  SSD1306AsciiWire oled;

  void setupDisplay() {
    oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
    oled.displayRemap(OLED_REMAP);
    oled.setScrollMode(false);
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
// - View DateTime
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
// - Settings
//    - Change backlight (LCD)
//    - Change contrast (LCD)
//    - Change bias (LCD)
//    - Change contrast (OLED)
//    - Invert display (OLED)
// - View SystemClock
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

// The Arduino compiler becomes confused without this.
extern const ModeGroup ROOT_MODE_GROUP;

// List of DateTime modes.
const uint8_t DATE_TIME_MODES[] = {
  (uint8_t) Mode::kChangeYear,
  (uint8_t) Mode::kChangeMonth,
  (uint8_t) Mode::kChangeDay,
  (uint8_t) Mode::kChangeHour,
  (uint8_t) Mode::kChangeMinute,
  (uint8_t) Mode::kChangeSecond,
};

// ModeGroup for the DateTime modes.
const ModeGroup DATE_TIME_MODE_GROUP = {
  &ROOT_MODE_GROUP /* parentGroup */,
  sizeof(DATE_TIME_MODES) / sizeof(uint8_t),
  DATE_TIME_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of TimeZone modes.
const uint8_t TIME_ZONE_MODES[] = {
#if TIME_ZONE_TYPE == TIME_ZONE_TYPE_MANUAL
  (uint8_t) Mode::kChangeTimeZoneOffset,
  (uint8_t) Mode::kChangeTimeZoneDst,
#else
  (uint8_t) Mode::kChangeTimeZoneName,
#endif
};

// ModeGroup for the TimeZone modes.
const ModeGroup TIME_ZONE_MODE_GROUP = {
  &ROOT_MODE_GROUP /* parentGroup */,
  sizeof(TIME_ZONE_MODES) / sizeof(uint8_t),
  TIME_ZONE_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of Settings modes.
const uint8_t SETTINGS_MODES[] = {
#if DISPLAY_TYPE == DISPLAY_TYPE_LCD
  (uint8_t) Mode::kChangeSettingsBacklight,
  (uint8_t) Mode::kChangeSettingsContrast,
  (uint8_t) Mode::kChangeSettingsBias,
#else
  (uint8_t) Mode::kChangeSettingsContrast,
  (uint8_t) Mode::kChangeInvertDisplay,
#endif
};

// ModeGroup for the Settings modes.
const ModeGroup SETTINGS_MODE_GROUP = {
  &ROOT_MODE_GROUP /* parentGroup */,
  sizeof(SETTINGS_MODES) / sizeof(uint8_t),
  SETTINGS_MODES /* modes */,
  nullptr /* childGroups */,
};

// List of top level modes.
const uint8_t TOP_LEVEL_MODES[] = {
  (uint8_t) Mode::kViewDateTime,
  (uint8_t) Mode::kViewTimeZone,
  (uint8_t) Mode::kViewSettings,
  (uint8_t) Mode::kViewSysclock,
  (uint8_t) Mode::kViewAbout,
};

// List of children ModeGroups for each element in TOP_LEVEL_MODES, in the same
// order.
const ModeGroup* const TOP_LEVEL_CHILD_GROUPS[] = {
  &DATE_TIME_MODE_GROUP,
  &TIME_ZONE_MODE_GROUP,
  &SETTINGS_MODE_GROUP,
  nullptr /* Mode::kViewSysclock has no submodes */,
  nullptr /* Mode::kViewAbout has no submodes */,
};

// Root mode group
const ModeGroup ROOT_MODE_GROUP = {
  nullptr /* parentGroup */,
  sizeof(TOP_LEVEL_MODES) / sizeof(uint8_t),
  TOP_LEVEL_MODES /* modes */,
  TOP_LEVEL_CHILD_GROUPS /* childGroups */,
};

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

Controller controller(systemClock, persistentStore, presenter, zoneManager,
  DISPLAY_ZONE, &ROOT_MODE_GROUP);

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

  Wire.begin();
  Wire.setClock(400000L);

  setupPersistentStore();
  setupAceButton();
  setupClocks();
  setupDisplay();
  setupPresenter();

  // Hold down the Mode button to perform factory reset.
  bool isModePressedDuringBoot = modeButton.isPressedRaw();
  setupController(isModePressedDuringBoot);

  CoroutineScheduler::setup();

  if (ENABLE_SERIAL_DEBUG >= 1) {
    SERIAL_PORT_MONITOR.println(F("setup(): end"));
  }
}

void loop() {
  CoroutineScheduler::loop();

#if SYSTEM_CLOCK_TYPE == SYSTEM_CLOCK_TYPE_LOOP
  systemClock.loop();
#endif
}
