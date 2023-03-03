# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := MedMinder
ARDUINO_LIBS := EpoxyEepromEsp AceCommon AceCRC AceButton AceSorting \
	AceTime AceTimeClock AceRoutine AceUtils AceWire SSD1306Ascii
DEPS:= \
	ClockInfo.h \
	Controller.cpp \
	Controller.h \
	PersistentStore.h \
	Presenter.cpp \
	Presenter.h \
	RenderingInfo.h \
	StoredInfo.h \
	config.h
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f epoxyeepromdata
