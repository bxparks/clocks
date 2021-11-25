# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

# LDFLAGS := -lz
APP_NAME := CommandLineClock
ARDUINO_LIBS := EpoxyEepromEsp AceCommon AceCRC AceTime AceTimeClock \
	AceRoutine AceWire AceUtils AceSorting
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
