# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := LedClockTiny
ARDUINO_LIBS := EpoxyEepromEsp EpoxyMockDigitalWriteFast \
	AceCommon AceCRC AceButton AceSorting AceTime AceTimeClock AceRoutine \
	AceUtils AceSPI AceTMI AceWire AceSegment AceSegmentWriter
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
