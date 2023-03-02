# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := MultiZoneClock
ARDUINO_LIBS := EpoxyFS EpoxyEepromEsp EpoxyMockSTM32RTC \
	EpoxyMockDigitalWriteFast \
	AceCommon AceButton AceCRC \
	AceSorting AceTime AceTimeClock AceTimePro AceRoutine AceUtils AceWire \
	SSD1306Ascii
DEPS:= ClockInfo.h \
	Controller.h \
	PersistentStore.h \
	Presenter.h \
	StoredInfo.h \
	config.h \
	Presenter.cpp
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

# Various tool directories on ESP8266 and ESP32.
ARDUINO_IDE_DIR := /home/brian/dev/arduino-1.8.13
ESP8266_TOOLS_DIR := $(ARDUINO_IDE_DIR)/portable/packages/esp8266/tools
ESP8266_HW_TOOLS_DIR := $(ARDUINO_IDE_DIR)/portable/packages/esp8266/hardware/esp8266/2.7.4/tools
ESP32_TOOLS_DIR := $(ARDUINO_IDE_DIR)/portable/packages/esp32/tools
ESP32_HW_TOOLS_DIR := $(ARDUINO_IDE_DIR)/portable/packages/esp32/hardware/esp32/1.0.4/tools

# File system tools on ESP8266 and ESP32.
MKSPIFFS_ESP8266 := $(ESP8266_TOOLS_DIR)/mkspiffs/2.5.0-4-b40a506/mkspiffs
MKLITTLEFS_ESP8266 := $(ESP8266_TOOLS_DIR)/mklittlefs/2.5.0-4-fe5bb56/mklittlefs
MKSPIFFS_ESP32 := $(ESP32_TOOLS_DIR)/mkspiffs/0.2.3/mkspiffs
ESPTOOL_ESP8266 := $(ESP8266_HW_TOOLS_DIR)/esptool/esptool.py
UPLOAD_ESP8266 := $(ESP8266_HW_TOOLS_DIR)/upload.py
ESPTOOL_ESP32 := $(ESP32_HW_TOOLS_DIR)/esptool.py

ACE_TIME_PRO := ../../AceTimePro

TARGETS := data/zonedb_thinz.bin data/zonedbx_thinz.bin

# FileSystem sizes. From boards.txt: (build.spiffs_end - build.spiffs_start).
#FS_SIZE := 1024000 # 4M1M
FS_SIZE := 2072576 # 4M2M
#FS_SIZE := 3121152 # 4M3M

# See https://github.com/esp8266/arduino-esp8266fs-plugin/issues/51.
spiffs.bin: $(TARGETS)
	$(MKSPIFFS_ESP8266) -c data -p 256 -b 8192 -s $(FS_SIZE) $@

# See https://github.com/esp8266/arduino-esp8266fs-plugin/issues/51.
littlefs.bin: $(TARGETS)
	$(MKLITTLEFS_ESP8266) -c data -p 256 -b 8192 -s $(FS_SIZE) $@

# The flags were captured by injecting a print() statement in upload.py.
uploadfs: littlefs.bin
	python3 $(UPLOAD_ESP8266) \
		--chip esp8266 \
		--port /dev/ttyUSB0 \
		--baud 921600 \
		write_flash 0x200000 \
		$<

data: $(TARGETS)

data/zonedb_thinz.bin: $(ACE_TIME_PRO)/tools/zonedb.json
	mkdir -p data
	$(ACE_TIME_PRO)/tools/create_zone_file.py \
		--format binary \
		--link_type thin \
		--compress \
		< $< \
		> $@

data/zonedbx_thinz.bin: $(ACE_TIME_PRO)/tools/zonedbx.json
	mkdir -p data
	$(ACE_TIME_PRO)/tools/create_zone_file.py \
		--format binary \
		--link_type thin \
		--compress \
		< $< \
		> $@

more_clean:
	rm -rf data littlefs.bin spiffs.bin
