//go:build tinygo

package main

import (
	"github.com/bxparks/AceButtonGo/button"
	"github.com/bxparks/AceSegmentGo/tm1637"
	"github.com/bxparks/AceSegmentGo/writer"
	"github.com/bxparks/AceTimeClockGo/ds3231"
	"github.com/bxparks/AceTimeGo/acetime"
	"github.com/bxparks/AceTimeGo/zonedb2000"
	"machine"
)

//-----------------------------------------------------------------------------
// Presenter using:
// TM1637 LED Module
//-----------------------------------------------------------------------------

var tm = tm1637.New(clkPin, dioPin, delayMicros, numDigits)
var patternWriter = writer.NewPatternWriter(&tm)
var numWriter = writer.NewNumberWriter(&patternWriter)
var charWriter = writer.NewCharWriter(&patternWriter)

func setupDisplay() {
	println("setupDisplay()")
	tm.Configure()
	tm.Clear()
}

//-----------------------------------------------------------------------------
// DS3231 RTC
//-----------------------------------------------------------------------------

var rtc = ds3231.New(i2c)

func setupRTC() {
	println("setupRTC()")
	rtc.Configure()
}

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------

var presenter = NewPresenter(&numWriter, &charWriter)
var controller = NewController(&presenter, &rtc, &tm)

//-----------------------------------------------------------------------------
// AceTimeGo
//-----------------------------------------------------------------------------

// An entry of the timezone supported by this app. The `name` field allows the
// application to display a human-readable name of the timezone, that is short
// and stable, and does not require the TimeZone object to be constructed. This
// is important because some time zones have abbreviations which have changed
// over time.
type ZoneInfo struct {
	tz   *acetime.TimeZone
	name string
}

// This implementation allocates the 4 TimeZone objects at startup time. The
// TinyGo compiler (through LLVM probably) seems able to determine that only a
// small subset of the zonedb database is accessed, and loads into flash memory
// only the required subset. This reduces the zonedb flash size from ~70kB to
// ~9kB.
//
// If instead we store only the zoneIDs in the `zones` array and dynamically
// create the TimeZone objects on-demand (which at first glance seems to be more
// memory efficient), then the compiler is not able to optimize away the zones
// which are never used, and must load the entire zonedb database (~70kB) into
// flash.
//
// For small number of time zones, it seems better to preallocate the timezones
// which will be used by the application. But if the application needs to
// support a significant number of timezones, potentiallly determined by user
// input, then it's better to let the ZoneManager create the TimeZone objects
// on-demand.
var manager = acetime.NewZoneManager(&zonedb2000.DataContext)
var tz0 = manager.TimeZoneFromZoneID(zonedb2000.ZoneIDAmerica_Los_Angeles)
var tz1 = manager.TimeZoneFromZoneID(zonedb2000.ZoneIDAmerica_Denver)
var tz2 = manager.TimeZoneFromZoneID(zonedb2000.ZoneIDAmerica_Chicago)
var tz3 = manager.TimeZoneFromZoneID(zonedb2000.ZoneIDAmerica_New_York)
var zones = []ZoneInfo{
	{&tz0, "PST"},
	{&tz1, "MST"},
	{&tz2, "CST"},
	{&tz3, "EST"},
}

//-----------------------------------------------------------------------------
// Buttons
//-----------------------------------------------------------------------------

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
var config = button.ButtonConfig{Handler: &ButtonHandler{}}
var modeButton = button.NewButton(&config, modePin)
var changeButton = button.NewButton(&config, changePin)

func setupButtons() {
	println("setupButtons()")
	modePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})
	changePin.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	config.Configure()
	config.SetFeature(button.FeatureLongPress)
	config.SetFeature(button.FeatureRepeatPress)
	config.SetFeature(button.FeatureSuppressAfterLongPress)
	config.SetFeature(button.FeatureSuppressAfterRepeatPress)
	//config.RepeatPressInterval = 100
}
