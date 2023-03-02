# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := OneZoneClock
ARDUINO_LIBS := EpoxyEepromEsp EpoxyMockSTM32RTC EpoxyMockDigitalWriteFast \
	AceCommon AceCRC AceButton AceSorting \
	AceTime AceTimeClock AceRoutine AceUtils AceWire \
	SSD1306Ascii
DEPS:= ClockInfo.h \
	Controller.h \
	PersistentStore.h \
	Presenter.h \
	RenderingInfo.h \
	StoredInfo.h \
	config.h \
	Presenter.cpp
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
