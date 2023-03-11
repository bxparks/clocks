//go:build arduino_nano

// Does not work yet.

package main

import "machine"

// TM1637 LED

const (
	clkPin      = machine.D2
	dioPin      = machine.D3
	delayMicros = 4
	numDigits   = 4
)

// Buttons

const (
	modePin   = machine.D2
	changePin = machine.D3
)

// I2C

var i2c = machine.I2C0

func setupI2C() {
	i2c.Configure(machine.I2CConfig{})
}
