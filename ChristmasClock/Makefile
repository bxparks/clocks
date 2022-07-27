# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := ChristmasClock
ARDUINO_LIBS := EpoxyEepromEsp EpoxyMockSTM32RTC \
	AceButton \
	AceCRC \
	AceCommon \
	AceRoutine \
	AceSegment \
	AceSegmentWriter \
	AceSorting \
	AceTMI \
	AceTime \
	AceTimeClock \
	AceUtils \
	AceWire
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
