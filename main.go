// An LED clock, bring togehter: AceTimeGo/acetime, button, and tm1637.

package main

import (
	//"github.com/bxparks/AceTimeGo/acetime"
	"gitlab.com/bxparks/coding/tinygo/button"
	"gitlab.com/bxparks/coding/tinygo/segwriter"
	"gitlab.com/bxparks/coding/tinygo/tm1637"
	"machine"
	"time"
)

//-----------------------------------------------------------------------------
// Buttons
//-----------------------------------------------------------------------------

const (
	modePin   = machine.GPIO15
	changePin = machine.GPIO14
)

type ButtonHandler struct{}

func (h *ButtonHandler) Handle(
	b *button.Button, event button.Event, state bool) {

	switch b.GetPin() {
	case modePin:
		print("Mode ")
		numWriter.WriteHexChar(1, 0)
	case changePin:
		print("Change ")
		numWriter.WriteHexChar(1, 1)
	default:
		print("Unknown ")
	}

	tm.SetDecimalPoint(1, true)

	switch event {
	case button.EventPressed:
		println("Pressed")
		numWriter.WriteHexChar(3, 1)
	case button.EventReleased:
		println("Released")
		numWriter.WriteHexChar(3, 0)
	case button.EventClicked:
		println("Clicked")
	case button.EventDoubleClicked:
		println("DoubleClicked")
	case button.EventLongPressed:
		println("LongPressed")
	case button.EventRepeatPressed:
		println("RepeatPressed")
		numWriter.WriteHexChar(3, 2)
	case button.EventLongReleased:
		println("LongReleased")
		numWriter.WriteHexChar(3, 0)
	default:
		println("Unexpected")
	}
}

// Configure buttons
var config = button.NewConfig(&ButtonHandler{})
var modeButton = button.NewButton(&config, modePin)
var changeButton = button.NewButton(&config, changePin)

func setupButtons() {
	modePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})
	changePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	config.SetFeature(button.FeatureLongPress)
	config.SetFeature(button.FeatureRepeatPress)
	config.SetFeature(button.FeatureSuppressAfterLongPress)
	config.SetFeature(button.FeatureSuppressAfterRepeatPress)
}

var lastButtonMillis uint16

// Check buttons every 5 milliseconds
func checkButtons() {
	millis := uint16(time.Now().UnixMilli())
	if millis-lastButtonMillis > 5 {
		modeButton.Check()
		changeButton.Check()
		lastButtonMillis = millis
	}
}

//-----------------------------------------------------------------------------
// TM1637 LED Module
//-----------------------------------------------------------------------------

const (
	clkPin      = machine.GPIO33
	dioPin      = machine.GPIO32
	delayMicros = 4
	numDigits   = 4
)

var tm = tm1637.New(clkPin, dioPin, delayMicros, numDigits)
var numWriter = segwriter.NewNumberWriter(&tm)

func setupDisplay() {
	tm.Configure()
	tm.Clear()
}

var lastFlushMillis uint16

// Flush LED display every 100 millis
func flushDisplay() {
	millis := uint16(time.Now().UnixMilli())
	if millis-lastFlushMillis > 100 {
		tm.Flush()
		lastFlushMillis = millis
	}
}

//-----------------------------------------------------------------------------
// Main loop
//-----------------------------------------------------------------------------

func main() {
	setupButtons()
	setupDisplay()

	time.Sleep(time.Millisecond * 500)
	println("Checking for buttons...")

	tm.Flush()
	for {
		checkButtons()
		flushDisplay()
		time.Sleep(time.Millisecond * 1)
	}
}
