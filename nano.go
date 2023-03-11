//go:build arduino_nano

// Does not work yet.

package main

import "machine"

const (
	clkPin      = machine.D2
	dioPin      = machine.D3
	delayMicros = 4
	numDigits   = 4
)
