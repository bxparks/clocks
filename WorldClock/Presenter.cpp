#include <AceTime.h>
#include <AceButton.h>
#include <AceRoutine.h>
#include "Presenter.h"

void Presenter::displayAbout() const {
  mOled.set1X();

  // For smallest flash memory size, use F() macros for these longer strings,
  // but no F() for shorter version strings.

  mOled.print(F("TZ: "));
  mOled.println(zonedb::kTzDatabaseVersion);
  mOled.println(F("AT: " ACE_TIME_VERSION_STRING));
  mOled.println(F("AB: " ACE_BUTTON_VERSION_STRING));
  mOled.print(F("AR: " ACE_ROUTINE_VERSION_STRING));
}
