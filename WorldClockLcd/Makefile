# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_NAME := WorldClockLcd
ARDUINO_LIBS := EpoxyFS EpoxyPromEsp AceCommon AceButton AceCRC \
	AceTime AceTimePro AceRoutine AceUtils
MORE_CLEAN := more_clean
include ../../EpoxyDuino/EpoxyDuino.mk

ACE_TIME_PRO := ../../AceTimePro

TARGETS := data/zonedb_thinz.bin data/zonedbx_thinz.bin

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
