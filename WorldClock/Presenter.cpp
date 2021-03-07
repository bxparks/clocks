#include <AceTime.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include "Presenter.h"

void Presenter::displayAbout() const {
  mOled.set1X();

  // For smallest flash memory size, use F() macros for these longer strings,
  // but no F() for shorter version strings.

  mOled.print(F("TZDB:"));
  mOled.println(zonedb::kTzDatabaseVersion);
  mOled.println(F("ATim:" ACE_TIME_VERSION_STRING));
  mOled.println(F("ABut:" ACE_BUTTON_VERSION_STRING));
  mOled.println(F("ARou:" ACE_ROUTINE_VERSION_STRING));
}

const uint8_t Presenter::kContrastValues[Presenter::kNumContrastValues] = {
  // Can't start from 0 because that would turn off the display
  // completely, and prevent us from doing anything else.
  25, 50, 75, 100, 125, 150, 175, 200, 225, 255
};
