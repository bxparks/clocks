//go:build xiao_max7219

package main

import (
	"machine"
)

// MAX7219 8-digit LED
const (
	cs        = machine.D6
	numDigits = 8
)

// Digital Buttons
const (
	modePin   = machine.D1
	changePin = machine.D0
)

// SAMD21 has multiple SPI ports
var spi = machine.SPI0

func setupSPI() {
	machine.SPI0.Configure(machine.SPIConfig{
		SDO:       machine.D10, // default MOSI pin
		SCK:       machine.D8, // default SCK pin
		LSBFirst:  false,
		Frequency: 10000000,
	})
}

var ledModule = max7219.New(spi, cs, numDigits)
