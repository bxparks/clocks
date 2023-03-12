//go:build arduino_zero

package main

import (
	"machine"
	"tinygo.org/x/drivers/i2csoft"
)

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

// I2C: Use software I2C because hardware I2C does not seem to be defined for
// Arduino Zero.
var i2c = i2csoft.New(machine.SCL_PIN, machine.SDA_PIN)

func setupI2C() {
	i2c.Configure(i2csoft.I2CConfig{Frequency: 400e3})
}
