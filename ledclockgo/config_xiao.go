//go:build xiao

package main

import (
	"machine"
)

// TM1637 LED
const (
	clkPin      = machine.D3
	dioPin      = machine.D2
	delayMicros = 100
	numDigits   = 4
)

// Buttons
const (
	modePin   = machine.D1
	changePin = machine.D0
)

// SAMD21 has 6 hardware I2C ports.
var i2c = machine.I2C0

func setupI2C() {
	i2c.Configure(machine.I2CConfig{Frequency: 400e3})
}
