# Clocks

These are various Arduino-compatible clock devices that I have built using my
AceTime library (https://github.com/bxparks/AceTime).

* [CommandLineClock](CommandLineClock)
    * A clock that can be accessed through the serial port.
    * Useful for quick debugging.
* [OledClock](OledClock)
    * A clock with a single SSD1306 OLED display, supporting a single timezone.
    * Now supports a PCD8544 LCD display (Nokia 5110), so the name is a bit
      misleading.
    * Useful for debugging various software libraries (AceButton, AceTime)
      and various hardware configurations (OLED display, LCD display, buttons,
      DS3231, etc).
* [WorldClock](OledClock)
    * A clock with 3 SSD1306 OLED displays, supporting 3 concurrent timezones.
* [WorldClockLcd](OledClock)
    * A clock with 1 PCD8544 LCD display, supporting 4 concurrent timezones.
* [MedMinder](MedMinder)
    * A medication reminder device.
