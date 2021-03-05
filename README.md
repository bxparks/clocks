# Clocks

[![Validate Compile](https://github.com/bxparks/clocks/actions/workflows/validate.yml/badge.svg)](https://github.com/bxparks/clocks/actions/workflows/validate.yml)

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
* [WorldClock](WorldClock)
    * A clock with 3 SSD1306 OLED displays, supporting 3 concurrent timezones.
* [WorldClockLcd](WorldClockLcd)
    * A clock with 1 PCD8544 LCD display, supporting 4 concurrent timezones.
* [MedMinder](MedMinder)
    * A medication reminder device.

All of these will compile under Linux and probably MacOS using EpoxyDuino
(https://github.com/bxparks/EpoxyDuino). So I was able to add them to the
Continuous Integration pipeline under GitHub Actions.
