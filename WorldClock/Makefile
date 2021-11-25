# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := WorldClock
ARDUINO_LIBS := EpoxyEepromEsp AceCommon AceCRC AceButton AceTime AceTimeClock \
	AceRoutine AceUtils AceWire SSD1306Ascii AceSorting
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
