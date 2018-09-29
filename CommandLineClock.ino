/*
 * A simple command line (i.e. ascii clock using the Serial line. Useful for
 * debugging. Requires no additional hardware in the simple case. Optionally
 * depends on the DS3231 RTC chip, or an NTP client.
 *
 * Depends on:
 *    * Wire
 *    * AceTime
 *    * AceRoutine
 *
 * Supported boards are:
 *    * Arduino Nano
 *    * Arduino Pro Mini
 *    * Arduino Leonardo (Pro Micro clone)
 *    * ESP8266
 *    * ESP32
 */

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <Wire.h>
#include <AceRoutine.h>
#include <ace_routine/cli/CommandManager.h>
#include <AceTime.h>
#include "config.h"
#include "Controller.h"
#include "PersistentStore.h"

using namespace ace_routine;
using namespace ace_routine::cli;
using namespace ace_time;

//---------------------------------------------------------------------------
// Compensate for buggy F() implementation in ESP8266.
//---------------------------------------------------------------------------

#if defined(ESP8266)
  #define FF(x) (x)
#else
  #define FF(x) F(x)
#endif

//---------------------------------------------------------------------------
// Configure RTC and TimeKeeper
//---------------------------------------------------------------------------

#if defined(USE_DS3231)
  DS3231TimeKeeper dsTimeKeeper;
  SystemTimeKeeper systemTimeKeeper(&dsTimeKeeper, &dsTimeKeeper /*backup*/);
#elif defined(USE_NTP)
  NtpTimeProvider ntpTimeProvider;
  SystemTimeKeeper systemTimeKeeper(&ntpTimeProvider, nullptr /*backup*/);
#elif defined(USE_SYSTEM)
  SystemTimeKeeper systemTimeKeeper(nullptr /*sync*/, nullptr /*backup*/);
#else
  #error Unknown time keeper option
#endif

#if SYNC_TYPE == SYNC_TYPE_COROUTINE
  SystemTimeSyncCoroutine systemTimeSync(systemTimeKeeper);
  SystemTimeHeartbeatCoroutine systemTimeHeartbeat(systemTimeKeeper);
#else
  SystemTimeLoop systemTimeLoop;
#endif

//---------------------------------------------------------------------------
// Create a controller retrieves or modifies the underlying clock.
//---------------------------------------------------------------------------

PersistentStore persistentStore;
Controller controller(persistentStore, systemTimeKeeper);

//---------------------------------------------------------------------------
// AceRoutine CLI commands
//---------------------------------------------------------------------------

#define SHIFT do { argv++; argc--; } while (false)

/** List the coroutines known by the CoroutineScheduler. */
class ListCommand: public CommandHandler {
  public:
    ListCommand():
        CommandHandler("list", nullptr) {}

    virtual void run(Print& printer, int /* argc */, const char** /* argv */)
            const override {
      CoroutineScheduler::list(printer);
    }
};

/**
 * Date command.
 * Usage:
 *    date - print current date
 *    date -s {iso8601} - set current date
 */
class DateCommand: public CommandHandler {
  public:
    DateCommand():
        CommandHandler("date", "[-s dateString]") {}

    virtual void run(Print& printer, int argc, const char** argv)
        const override {
      // parse the command line arguments
      const char* newDateString = nullptr;
      SHIFT;
      while (argc > 0) {
        if (strcmp(*argv, "-s") == 0) {
          SHIFT;
          if (argc == 0) {
            printer.println(FF("No date after -s flag"));
            return;
          }
          newDateString = *argv;
        } else if (**argv == '-') {
          printer.print(FF("Unknown flag: "));
          printer.println(*argv);
          return;
        } else {
          break;
        }
        SHIFT;
      }

      if (newDateString != nullptr) {
        DateTime newDate = DateTime::forDateString(newDateString);
        if (newDate.isError()) {
          printer.print(FF("Invalid date: "));
          printer.println(newDateString);
          return;
        }
        controller.setNow(newDate);
        printer.print(FF("Date set to: "));
        newDate.printTo(printer);
        printer.println();
      } else {
        DateTime now = controller.getNow();
        now.printTo(printer);
        printer.println();
      }
    }
};

/**
 * Timezone command.
 * Usage:
 *    timezone - print current timezone
 *    timezone -s {code} - set current timezone
 */
class TimezoneCommand: public CommandHandler {
  public:
    TimezoneCommand():
      CommandHandler("timezone", "[-s utcOffset]") {}

    virtual void run(Print& printer, int argc, const char** argv)
        const override {
      // parse the command line arguments
      const char* newTimeZoneString = nullptr;
      SHIFT;
      while (argc > 0) {
        if (strcmp(*argv, "-s") == 0) {
          SHIFT;
          if (argc == 0) {
            printer.println(FF("No tzCode after -s flag"));
            return;
          }
          newTimeZoneString = *argv;
        } else if (**argv == '-') {
          printer.print(FF("Unknown flag: "));
          printer.println(*argv);
          return;
        } else {
          break;
        }
        SHIFT;
      }

      if (newTimeZoneString != nullptr) {
        TimeZone tz = TimeZone::forOffsetString(newTimeZoneString);
        if (tz.isError()) {
          printer.println(FF("Invalid time zone"));
          return;
        }
        controller.setTimeZone(tz);
        printer.print(FF("Time zone set to: UTC"));
        tz.printTo(printer);
        printer.println();
      } else {
        TimeZone timeZone = controller.getTimeZone();
        printer.print(FF("UTC"));
        timeZone.printTo(printer);
        printer.println();
      }
    }
};

#if defined(USE_NTP)

/**
 * WiFi manager command.
 * Usage:
 *    wifi status - print current wifi parameters
 *    wifi config - print current ssid and password
 *    wifi config {ssid} {password} - set new ssid and password
 */
class WifiCommand: public CommandHandler {
  public:
    WifiCommand(
        Controller& controller,
        NtpTimeProvider& ntpTimeProvider):
      CommandHandler(
          "wifi", "status | (config [ssid password]) | connect " ),
      mController(controller),
      mNtpTimeProvider(ntpTimeProvider)
      {}

    virtual void run(Print& printer, int argc, const char** argv)
        const override {
      SHIFT;
      if (argc == 0) {
        printer.println(FF("Must give either 'status' or 'config' command"));
      } else if (strcmp(argv[0], "config") == 0) {
        SHIFT;
        if (argc == 0) {
          if (mController.isStoredInfoValid()) {
            // print ssid and password
            const StoredInfo& storedInfo = mController.getStoredInfo();
            printer.print(FF("ssid: "));
            printer.println(storedInfo.ssid);
            printer.print(FF("password: "));
            printer.println(storedInfo.password);
          } else {
            printer.println(
              FF("Invalid ssid and password in persistent storage"));
          }
        } else if (argc == 2) {
          const char* ssid = argv[0];
          const char* password = argv[1];
          mController.setWiFi(ssid, password);
          connect(printer);
        } else {
          printer.println(FF("Wifi config command requires 2 arguments"));
        }
      } else if (strcmp(argv[0], "status") == 0) {
        printer.print(FF("NtpTimeProvider::isSetup(): "));
        printer.println(mNtpTimeProvider.isSetup() ? FF("true") : FF("false"));
        printer.print(FF("NTP Server: "));
        printer.println(mNtpTimeProvider.getServer());
        printer.print(FF("WiFi IP address: "));
        printer.println(WiFi.localIP());
      } else if (strcmp(argv[0], "connect") == 0) {
        connect(printer);
      } else {
        printer.print(FF("Unknown wifi command: "));
        printer.println(argv[0]);
      }
    }

    void connect(Print& printer) const {
      const StoredInfo& storedInfo = mController.getStoredInfo();
      const char* ssid = storedInfo.ssid;
      const char* password = storedInfo.password;
      mNtpTimeProvider.setup(ssid, password);
      if (mNtpTimeProvider.isSetup()) {
        printer.println(F("Connection succeeded."));
      } else {
        printer.println(F("Connection failed... run 'wifi connect' again"));
      }
    }

  private:
    Controller& mController;
    NtpTimeProvider& mNtpTimeProvider;
};

#endif

// Create an instance of the CommandManager.
const uint8_t MAX_COMMANDS = 10;
const uint8_t BUF_SIZE = 64;
const uint8_t ARGV_SIZE = 5;
CommandManager<MAX_COMMANDS, BUF_SIZE, ARGV_SIZE> commandManager(Serial, "> ");

ListCommand listCommand;
DateCommand dateCommand;
TimezoneCommand timezoneCommand;
#if defined(USE_NTP)
WifiCommand wifiCommand(controller, ntpTimeProvider);
#endif

//---------------------------------------------------------------------------
// Main setup and loop
//---------------------------------------------------------------------------

void setup() {
  // Wait for stability on some boards.
  // 1000ms needed for Serial.
  // 2000ms needed for Wire, I2C or SSD1306 (don't know which one).
  delay(2000);

#if defined(ARDUINO_AVR_LEONARDO)
  RXLED0; // LED off
  TXLED0; // LED off
#endif

  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // Wait until Serial is ready - Leonardo/Micro
  Serial.println(FF("setup(): begin"));

  Serial.print(FF("sizeof(StoredInfo): "));
  Serial.println(sizeof(StoredInfo));

  Wire.begin();
  Wire.setClock(400000L);

#if defined(USE_DS3231)
  dsTimeKeeper.setup();
#endif

  persistentStore.setup();
  systemTimeKeeper.setup();
  controller.setup();

#if defined(USE_NTP)
/*
  if (controller.isStoredInfoValid()) {
    const StoredInfo& storedInfo = controller.getStoredInfo();
    ntpTimeProvider.setup(storedInfo.ssid, storedInfo.password);
  }
*/
#endif

  // add commands
  commandManager.add(&listCommand);
  commandManager.add(&dateCommand);
  commandManager.add(&timezoneCommand);
#if defined(USE_NTP)
  commandManager.add(&wifiCommand);
#endif
  commandManager.setupCommands();

  // insert coroutines into the scheduler
#if SYNC_TYPE == SYNC_TYPE_COROUTINE
  systemTimeSync.setupCoroutine(FF("systemTimeSync"));
  systemTimeHeartbeat.setupCoroutine(FF("systemTimeHeartbeat"));
#endif
  commandManager.setupCoroutine(FF("commandManager"));
  CoroutineScheduler::setup();

  Serial.println(FF("setup(): end"));
}

void loop() {
  CoroutineScheduler::loop();
#if SYNC_TYPE == SYNC_TYPE_MANUAL
  systemTimeLoop.loop();
#endif
}
