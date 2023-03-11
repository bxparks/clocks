//go:build esp32

package main

import "machine"

const (
	clkPin      = machine.GPIO33
	dioPin      = machine.GPIO32
	delayMicros = 4
	numDigits   = 4
)
