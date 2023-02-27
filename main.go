// An LED clock, bring together: AceTimeGo/acetime, button, and tm1637.

package main

import (
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
var charWriter = segwriter.NewCharWriter(&tm)

func setupDisplay() {
	println("setupDisplay()")
	tm.Configure()
	tm.Clear()
}

var lastFlushTime = time.Now()

// Flush LED display every 100 millis
func flushDisplay() {
	now := time.Now()
	elapsed := now.Sub(lastFlushTime)
	if elapsed.Milliseconds() >= 100 {
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
	println("setupRTC()")
	i2c.Configure(i2csoft.I2CConfig{Frequency: 400e3})
	rtc.Configure()
}

//-----------------------------------------------------------------------------
// Temperature, read every 10 seconds. The DS3231 updates every 64 seconds. But
// sometimes, the first ReadTemp() fails, so it's nice to try again about 10
// seconds after the failure.
//-----------------------------------------------------------------------------

var lastReadTempTime = time.Now()

func readTemperature() {
	now := time.Now()
	elapsed := now.Sub(lastReadTempTime)
	if elapsed.Milliseconds() >= 10000 {
		lastReadTempTime = now
		controller.ReadTemp()
	}
}

//-----------------------------------------------------------------------------
// System Time
//-----------------------------------------------------------------------------

var lastSyncRTCTime = time.Now()

func syncSystemTime() {
	now := time.Now()
	elapsed := now.Sub(lastSyncRTCTime)
	if elapsed.Milliseconds() >= 100 {
		lastSyncRTCTime = now
		controller.SyncSystemTime()
	}
}

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------

var presenter = NewPresenter(&numWriter, &charWriter)
var controller = NewController(&presenter, &rtc, &tm)

var lastUpdateDisplayTime = time.Now()

func updateDisplay() {
	now := time.Now()
	elapsed := now.Sub(lastUpdateDisplayTime)
	if elapsed.Milliseconds() >= 100 {
		lastUpdateDisplayTime = now
		presenter.UpdateDisplay()
	}
}

//-----------------------------------------------------------------------------
// AceTimeGo
//-----------------------------------------------------------------------------

// An entry of the timezone supported by this app. The `name` field allows the
// application to display a human-readable name of the timezone, that is short
// and stable, and does not require the TimeZone object to be constructed. This
// is important because some time zones have abbreviations which have changed
// over time.
type ZoneInfo struct {
	tz	 *acetime.TimeZone
	name string
}

// This implementation allocates the 4 TimeZone objects at startup time. The
// TinyGo compiler (through LLVM probably) seems able to determine that the only
// a small subset of the zonedb database is accessed, and allocates flash memory
// only to the required subset. This reduces the zonedb flash size from ~70kB to
// ~9kB.
//
// If instead we do not pre-allocate the timezones and store only the zoneIDs
// in the `zones` array, (which at first glance seems to be more memory
// efficient), then the compiler is not able to optimize away the zones which
// are never used, and must load the entire zonedb database (~70kB) into flash.
//
// For small number of time zones, it seems better to preallocate the timezones
// which will be used by the application. But if the application needs to
// support a significant number of timezones, potentiallly determined by user
// input, then it's better to let the ZoneManager create the TimeZone objects
// on-demand.
var manager = acetime.NewZoneManager(&zonedb.DataContext)
var tz0 = manager.TimeZoneFromZoneID(zonedb.ZoneIDAmerica_Los_Angeles)
var tz1 = manager.TimeZoneFromZoneID(zonedb.ZoneIDAmerica_Denver)
var tz2 = manager.TimeZoneFromZoneID(zonedb.ZoneIDAmerica_Chicago)
var tz3 = manager.TimeZoneFromZoneID(zonedb.ZoneIDAmerica_New_York)
var zones = []ZoneInfo{
	{&tz0, "PST"},
	{&tz1, "MST"},
	{&tz2, "CST"},
	{&tz3, "EST"},
}

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
			controller.HandleModePress()
		case button.EventLongPressed:
			controller.HandleModeLongPress()
		default:
		}
	case changePin:
		switch e {
		case button.EventPressed, button.EventRepeatPressed:
			controller.HandleChangePress()
		case button.EventReleased, button.EventLongReleased:
			controller.HandleChangeRelease()
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
	println("setupButtons()")
	modePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})
	changePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	config.SetFeature(button.FeatureLongPress)
	config.SetFeature(button.FeatureRepeatPress)
	config.SetFeature(button.FeatureSuppressAfterLongPress)
	config.SetFeature(button.FeatureSuppressAfterRepeatPress)
	//config.RepeatPressInterval = 100
}

var lastCheckButtonsTime = time.Now()

// Check buttons every 5 milliseconds
func checkButtons() {
	now := time.Now()
	elapsed := now.Sub(lastCheckButtonsTime)
	if elapsed.Milliseconds() >= 5 {
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
	time.Sleep(time.Millisecond * 500)

	setupButtons()
	setupDisplay()
	setupRTC()
	controller.SetupSystemTimeFromRTC()
	tm.Flush()

	println("Entering event loop...")
	for {
		checkButtons()
		syncSystemTime()
		readTemperature()
		blinkDisplay()
		updateDisplay()
		flushDisplay()
		time.Sleep(time.Millisecond * 1)
	}
}
