# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := LedClock
ARDUINO_LIBS := EpoxyEepromEsp EpoxyMockDigitalWriteFast \
	AceCommon AceCRC AceButton AceTime \
	AceRoutine AceUtils SSD1306Ascii AceSegment AUnit
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
