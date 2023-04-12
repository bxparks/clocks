SRCS := main.go controller.go presenter.go clockinfo.go

#------------------------------------------------------------------------------

buildnano: $(SRCS) config_nano.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=arduino-nano \
		-o nano.out \
		> nano.size.txt

flashnano:
	tinygo flash -x -target=arduino-nano

#------------------------------------------------------------------------------

buildzero: $(SRCS) config_zero.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=arduino-zero \
		-o zero.out \
		> zero.size.txt

flashzero:
	tinygo flash -x -target=arduino-zero

#------------------------------------------------------------------------------

buildxiao: $(SRCS) config_xiao.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=xiao \
		-o xiao.out \
		> xiao.size.txt

flashxiao:
	tinygo flash -x -target=xiao

#------------------------------------------------------------------------------
# D1Mini large board

buildd1mini: $(SRCS) config_d1mini.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=d1mini \
		-tags=d1mini \
		-o d1mini.out \
		> d1mini.size.txt

flashd1mini:
	tinygo flash -x -target=d1mini -tags=d1mini

#------------------------------------------------------------------------------
# ESP32 box with ezSBC ESP32 using time.Sleep()

buildesp32box: $(SRCS) config_esp32box.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-tags=esp32box \
		-o esp32.out \
		> esp32.size.txt

flashesp32box:
	tinygo flash -x -target=esp32-coreboard-v2 -tags=esp32box

#------------------------------------------------------------------------------
# ESP32 box with ezSBC ESP32, using Goroutines.

buildesp32go: $(SRCS) config_esp32.go Makefile
	tinygo build \
		--tags goroutine \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o esp32.out \
		> esp32go.size.txt

flashesp32go:
	tinygo flash -x -target=esp32-coreboard-v2 -tags=goroutine

#------------------------------------------------------------------------------
# ESP32 dev board with DevKit V1

buildesp32board: $(SRCS) config_esp32board.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-tags=esp32board \
		-o esp32.out \
		> esp32.size.txt

flashesp32board:
	tinygo flash -x -target=esp32-coreboard-v2 -tags=esp32board

#------------------------------------------------------------------------------

clean:
	rm -f *.out
