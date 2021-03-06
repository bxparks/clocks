# Clocks

[![Validate Compile](https://github.com/bxparks/clocks/actions/workflows/validate.yml/badge.svg)](https://github.com/bxparks/clocks/actions/workflows/validate.yml)

These are various Arduino-compatible clock devices that I have built using my
AceTime library (https://github.com/bxparks/AceTime).

* [CommandLineClock](CommandLineClock)
    * A clock that can be accessed through the serial port.
    * Useful for quick debugging.
    * Supported platforms:
        * Arduino Nano
        * SparkFun Micro
        * SAMD21
        * STM32 Blue Pill
        * ESP8266
        * ESP32
        * Teensy 3.2
        * EpoxyDuino
* [OneZoneClock](OneZoneClock)
    * A clock with a single display showing a single timezone selected from
      a menu of timezones.
    * Supports the following displays:
        * SSD1306 OLED
        * PCD8544 LCD (Nokia 5110).
    * Useful for debugging various software libraries (AceButton, AceTime)
      and various hardware configurations (OLED display, LCD display, buttons,
      DS3231, etc).
    * Supported platforms:
        * Arduino Nano
        * SparkFun Micro
        * SAMD21
        * STM32 Blue Pill
        * ESP8266
        * ESP32
        * Teensy 3.2
* [WorldClock](WorldClock)
    * A clock with the following hardware and features:
        * SparkFun Pro Micro 3.3V, 8 MHz
        * 3 SSD1306 SPI OLED displays
        * shows 3 concurrent timezones
* [MultiZoneClock](MultiZoneClock)
    * A clock with a single display that can show multiple timezones from
      a menu of timezones. The "WorldClockLcd" example is configured to show 4
      concurrent timezones.
    * Supports the following displays:
        * PCD8544 LCD (Nokia 5110)
        * SSD1306 OLED
    * Supported platforms:
        * Arduino Nano
        * SparkFun Micro
        * SAMD21
        * STM32 Blue Pill
        * ESP8266
        * ESP32
        * Teensy 3.2
* [MedMinder](MedMinder)
    * A medication reminder device.
* [LedClock](LedClock)
    * Similar to OneZoneClock, but using an 4-digit-7-segment LED display.
    * Hardware
        * SparkFun Pro Micro
        * 7-segment LED
        * DS3231
        * 2 buttons
    * This is one of my earliest Arduino projects. It has a lot of rough
      edges, so I'm not quite happy with it.
    * It was starting to bit-rot due to changes to the various libraries that it
      depends on. So I pulled it into this repo and added to the GitHub Actions
      continuous integration, which should mitigate or slow down some of the
      bit-rotting.

All of these will compile under Linux and probably MacOS using EpoxyDuino
(https://github.com/bxparks/EpoxyDuino). So I was able to add them to the
Continuous Integration pipeline under GitHub Actions.
