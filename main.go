// An LED clock, bring together: AceTimeGo/acetime, button, and tm1637.

package main

import (
	//"github.com/bxparks/AceTimeGo/acetime"
	"github.com/bxparks/AceTimeGo/acetime"
	"github.com/bxparks/AceTimeGo/zonedb"
	"gitlab.com/bxparks/coding/tinygo/button"
	"gitlab.com/bxparks/coding/tinygo/ds3231"
	"gitlab.com/bxparks/coding/tinygo/segwriter"
	"gitlab.com/bxparks/coding/tinygo/tm1637"
	"machine"
	"time"
	"tinygo.org/x/drivers/i2csoft"
)

//-----------------------------------------------------------------------------
// Presenter using:
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

var lastFlushTime = time.Now()

// Flush LED display every 100 millis
func flushDisplay() {
	now := time.Now()
	elapsed := now.Sub(lastFlushTime)
	if elapsed.Milliseconds() > 100 {
		lastFlushTime = now
		tm.Flush()
	}
}

//-----------------------------------------------------------------------------
// DS3231 RTC
//-----------------------------------------------------------------------------

var i2c = i2csoft.New(machine.SCL_PIN, machine.SDA_PIN)
var rtc = ds3231.New(i2c)

func setupRTC() {
	i2c.Configure(i2csoft.I2CConfig{Frequency: 400e3})
	rtc.Configure()
	//dt := ds3231.DateTime{23, 2, 25, 9, 51, 0, 7 /*Weekday*/}
	//rtc.SetTime(dt)
}

var lastSyncRTCTime = time.Now()

func syncRTC() {
	now := time.Now()
	elapsed := now.Sub(lastSyncRTCTime)
	if elapsed.Milliseconds() > 100 {
		lastSyncRTCTime = now
		controller.SyncRTC()
	}
}

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------

var presenter = NewPresenter(&numWriter)
var controller = NewController(&presenter, &rtc)

var lastUpdateDisplayTime = time.Now()

func updateDisplay() {
	now := time.Now()
	elapsed := now.Sub(lastUpdateDisplayTime)
	if elapsed.Milliseconds() > 100 {
		lastUpdateDisplayTime = now
		presenter.UpdateDisplay()
	}
}

//-----------------------------------------------------------------------------
// AceTimeGo
//-----------------------------------------------------------------------------

var manager = acetime.NewZoneManager(&zonedb.DataContext)
var tz = manager.TimeZoneFromName("America/Los_Angeles")

//-----------------------------------------------------------------------------
// Buttons
//-----------------------------------------------------------------------------

const (
	modePin   = machine.GPIO15
	changePin = machine.GPIO14
)

type ButtonHandler struct{}

func (h *ButtonHandler) Handle(b *button.Button, e button.Event, state bool) {
	switch b.GetPin() {
	case modePin:
		switch e {
		case button.EventReleased:
			println("Mode Pressed")
			controller.HandleModePress()
		case button.EventLongPressed:
			controller.HandleModeLongPress()
		default:
		}
	case changePin:
		switch e {
		case button.EventPressed, button.EventRepeatPressed:
			println("Change Pressed")
			controller.HandleChangePress()
		default:
		}
	default:
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

var lastCheckButtonsTime = time.Now()

// Check buttons every 5 milliseconds
func checkButtons() {
	now := time.Now()
	elapsed := now.Sub(lastCheckButtonsTime)
	if elapsed.Milliseconds() > 5 {
		lastCheckButtonsTime = now
		modeButton.Check()
		changeButton.Check()
	}
}

//-----------------------------------------------------------------------------
// Blinking
//-----------------------------------------------------------------------------

var lastBlinkTime = time.Now()

func blinkDisplay() {
	now := time.Now()
	elapsed := now.Sub(lastBlinkTime)
	if elapsed.Milliseconds() >= 500 {
		lastBlinkTime = now
		controller.Blink()
	}
}

//-----------------------------------------------------------------------------
// Main loop
//-----------------------------------------------------------------------------

func main() {
	setupButtons()
	setupDisplay()
	setupRTC()

	time.Sleep(time.Millisecond * 500)
	println("Checking for buttons...")

	tm.Flush()
	for {
		checkButtons()
		syncRTC()
		blinkDisplay()
		updateDisplay()
		flushDisplay()
		time.Sleep(time.Millisecond * 1)
	}
}
