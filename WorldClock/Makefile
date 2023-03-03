# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := WorldClock
ARDUINO_LIBS := EpoxyEepromEsp AceCommon AceCRC AceButton AceSorting \
	AceTime AceTimeClock AceRoutine AceUtils AceWire SSD1306Ascii AceSPI
DEPS := \
	ClockInfo.h \
	Controller.h \
	PersistentStore.h \
	Presenter.cpp \
	Presenter.h \
	StoredInfo.h \
	config.h
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
