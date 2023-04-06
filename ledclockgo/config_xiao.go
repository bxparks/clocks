//go:build xiao

package main

import (
	"github.com/bxparks/AceSegmentGo/tm1637"
	"machine"
)

// TM1637 LED
const (
	clkPin      = machine.D3
	dioPin      = machine.D2
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
	modePin   = machine.D1
	changePin = machine.D0
)

// SAMD21 has 6 hardware I2C ports.
var i2c = machine.I2C0

func setupI2C() {
	i2c.Configure(machine.I2CConfig{Frequency: 400e3})
}
