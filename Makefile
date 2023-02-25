CMD_NAME := main
TARGETS_UNIX := $(CMD_NAME).out
TARGETS_TINY := $(CMD_NAME).tiny.out
TARGETS_TINY_ESP32 := $(CMD_NAME).esp32.out
SRCS := $(CMD_NAME).go controller.go presenter.go clockinfo.go

.PHONY := build tiny tinyesp32 flashesp32 all clean help

tinyesp32: $(TARGETS_TINY_ESP32)

flashesp32:
	tinygo flash -x -target=esp32-coreboard-v2

clean:
	rm -f $(TARGETS_UNIX) $(TARGETS_TINY) $(TARGETS_TINY_ESP32)

#------------------------------------------------------------------------------

# Use TinyGo to compile to ESP32.
$(CMD_NAME).esp32.out: $(SRCS) Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o $@ \
		> $(CMD_NAME).esp32.size.txt
