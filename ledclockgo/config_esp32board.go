//go:build esp32 && esp32board

package main

import (
	"github.com/bxparks/AceSegmentGo/tm1637"
	"machine"
	"tinygo.org/x/drivers/i2csoft"
)

// TM1637 LED
const (
	clkPin      = machine.GPIO14
	dioPin      = machine.GPIO13
	delayMicros = 4
	numDigits   = 4
)

var tmi = tm1637.TMISoft{clkPin, dioPin, delayMicros}
var ledModule = tm1637.Device{
	TMI: &tmi,
	NumDigits: numDigits,
	DigitRemap: nil,
	Brightness: 4,
}

// Digital Buttons
const (
	modePin   = machine.GPIO19
	changePin = machine.GPIO18
)

// I2C
var i2c = i2csoft.New(machine.SCL_PIN, machine.SDA_PIN)

func setupI2C() {
	i2c.Configure(i2csoft.I2CConfig{Frequency: 400e3})
}
