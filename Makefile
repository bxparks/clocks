SRCS := main.go controller.go presenter.go clockinfo.go

all: buildzero buildesp32 buildesp32go

#------------------------------------------------------------------------------

buildnano: $(SRCS) config_nano.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=arduino-nano \
		-o nano.out \
		> main.nano.size.txt

flashnano:
	tinygo flash -x -target=arduino-nano

#------------------------------------------------------------------------------

buildzero: $(SRCS) config_zero.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=arduino-zero \
		-o zero.out \
		> main.zero.size.txt

flashzero:
	tinygo flash -x -target=arduino-zero

#------------------------------------------------------------------------------

buildesp32: $(SRCS) config_esp32.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o esp32.out \
		> main.esp32.size.txt

flashesp32:
	tinygo flash -x -target=esp32-coreboard-v2

#------------------------------------------------------------------------------

buildesp32go: $(SRCS) config_esp32.go Makefile
	tinygo build \
		--tags goroutine \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o esp32.out \
		> main.esp32go.size.txt

flashesp32go:
	tinygo flash --tags goroutine -x -target=esp32-coreboard-v2

#------------------------------------------------------------------------------

clean:
	rm -f *.out
