# Clocks

[![Validate Compile](https://github.com/bxparks/clocks/actions/workflows/validate.yml/badge.svg)](https://github.com/bxparks/clocks/actions/workflows/validate.yml)

These are various Arduino-compatible clock devices that I have built using my
AceTime library (https://github.com/bxparks/AceTime).

## Generic Clocks

These clock apps support multiple microcontrollers and multiple LED
modules. They are often used for developing and validating the various
sub libraries (e.g. AceTime, AceTimeClock, AceSegment, AceSegmentWriter).

* [CommandLineClock](CommandLineClock)
    * A clock that can be accessed through the serial port.
    * Useful for quick debugging.
* [OneZoneClock](OneZoneClock)
    * A clock with a single display showing a single timezone selected from
      a menu of timezones.
    * Supports the following displays:
        * SSD1306 OLED
        * PCD8544 LCD (Nokia 5110).
    * Useful for debugging various software libraries (AceButton, AceTime)
      and various hardware configurations (OLED display, LCD display, buttons,
      DS3231, etc).
* [MultiZoneClock](MultiZoneClock)
    * A clock with a single display that can show multiple timezones from
      a menu of timezones. The "WorldClockLcd" example is configured to show 4
      concurrent timezones.
    * Supports the following displays:
        * PCD8544 LCD (Nokia 5110)
        * SSD1306 OLED
* [LedClock](LedClock)
    * Similar to OneZoneClock, but using an 4-digit-7-segment LED display.
    * Hardware:
        * various 7-segment LED modules
        * various microcontrollers
        * DS3231
        * 2 buttons

## Specialized Clocks

These are specialized devices with firmware designed explicitly for that device:

* [WorldClock](WorldClock)
    * A clock that shows 3 concurrent timezones using 3 OLED displays.
    * 1 x SparkFun Pro Micro 3.3V, 8 MHz
    * 3 x SSD1306 SPI OLED displays
    * 1 x DS3231
* [MedMinder](MedMinder)
    * A battery powered medication reminder device.
    * 1 x SparkFun Pro Micro 3.3V, 8 MHz
    * 1 x SSD1306 SPI OLED displays
    * 1 x DS3231
    * 3 x AAA NiMH batteries
* [ChristmasClock](ChristmasClock)
    * A clock that shows the number of days until the next Christmas.
    * 1 x D1 Mini
    * 1 x TM1637-4 LED module
    * 1 x DS3231

## Continuous Integration

All of these will compile under Linux and probably MacOS using EpoxyDuino
(https://github.com/bxparks/EpoxyDuino). So I was able to add them to the
Continuous Integration pipeline under GitHub Actions.

The pipeline can be run manually by using the following commands from the
top-level directory:

```
$ make clean
$ make all
```
