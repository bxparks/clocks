SRCS := main.go controller.go presenter.go clockinfo.go

.PHONY := buildesp32 flashesp32

#------------------------------------------------------------------------------

buildnano: $(SRCS) nano.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=arduino-nano \
		-o nano.out \
		> main.nano.size.txt

flashnano:
	tinygo flash -x -target=arduino-nano

#------------------------------------------------------------------------------

buildesp32: $(SRCS) esp32.go Makefile
	tinygo build \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o esp32.out \
		> main.esp32.size.txt

flashesp32:
	tinygo flash -x -target=esp32-coreboard-v2

#------------------------------------------------------------------------------

buildesp32go: $(SRCS) esp32.go Makefile
	tinygo build \
		--tags goroutine \
		-size full \
		-print-allocs=.*bxparks.* \
		-target=esp32-coreboard-v2 \
		-o esp32.out \
		> main.esp32.size.txt

flashesp32go:
	tinygo flash --tags goroutine -x -target=esp32-coreboard-v2

#------------------------------------------------------------------------------

clean:
	rm -f *.out
