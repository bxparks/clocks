# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

LDFLAGS := -lz
APP_NAME := CommandLineClock
ARDUINO_LIBS := AceCommon AceTime AceRoutine AceUtils
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

more_clean:
	rm -f commandline.dat
