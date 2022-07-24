# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := ChristmasClock
ARDUINO_LIBS := EpoxyEepromEsp EpoxyMockDigitalWriteFast EpoxyMockSTM32RTC \
	AUnit AceCommon AceCRC AceButton AceSorting AceTime AceTimeClock \
	AceRoutine AceUtils AceSPI AceTMI AceWire AceSegment AceSegmentWriter
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
