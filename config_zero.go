//go:build arduino_zero

// Does not work, I2C is not defined for Arduino Zero.

package main

import "machine"

// TM1637 LED

const (
	clkPin      = machine.D13
	dioPin      = machine.D11
	delayMicros = 100
	numDigits   = 4
)

// Buttons

const (
	modePin   = machine.D8
	changePin = machine.D9
)

// I2C

var i2c = machine.I2C0

func setupI2C() {
	i2c.Configure(machine.I2CConfig{})
}
