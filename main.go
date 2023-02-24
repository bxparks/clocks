// An LED clock, bring togehter: AceTimeGo/acetime, button, and tm1637.

package main

import (
	//"github.com/bxparks/AceTimeGo/acetime"
	"gitlab.com/bxparks/coding/tinygo/button"
	"gitlab.com/bxparks/coding/tinygo/ds3231"
	"gitlab.com/bxparks/coding/tinygo/segwriter"
	"gitlab.com/bxparks/coding/tinygo/tm1637"
	"github.com/bxparks/AceTimeGo/acetime"
	"github.com/bxparks/AceTimeGo/zonedb"
	"machine"
	"time"
	"tinygo.org/x/drivers/i2csoft"
)

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
		lastFlushMillis = millis
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
	dt := ds3231.DateTime{23, 2, 24, 11, 21, 0, 5 /*Weekday*/}
	rtc.SetTime(dt)
}

var lastRTCMillis uint16

func syncRTC() {
	millis := uint16(time.Now().UnixMilli())
	if millis - lastRTCMillis > 100 {
		lastRTCMillis = millis
		dt, err := rtc.ReadTime()
		if err != nil {
			return
		}

		ldt := acetime.LocalDateTime{
			2000 + int16(dt.Year),
			dt.Month,
			dt.Day,
			dt.Hour,
			dt.Minute,
			dt.Second,
			0 /*Fold*/,
		}
		clockInfo.dateTime = acetime.NewZonedDateTimeFromLocalDateTime(&ldt, &tz)
		updateDisplay()
	}
}

//-----------------------------------------------------------------------------
// Presenter
//-----------------------------------------------------------------------------

var clockInfo = ClockInfo{
	hourMode: hourMode24,
	clockMode: clockModeHourMinute,
	brightness: 1,
}

var prevClockInfo ClockInfo

func nextClockMode() {
	clockInfo.clockMode = ClockMode(uint8(clockInfo.clockMode + 1) % 5)
	println("clockMode: ", clockInfo.clockMode)
}

func updateDisplay() {
	if prevClockInfo == clockInfo {
		return
	}
	prevClockInfo = clockInfo
	zdt := &clockInfo.dateTime

	switch clockInfo.clockMode {
	case clockModeYear:
		numWriter.WriteDec4(0, uint16(zdt.Year), 0)
	case clockModeMonth:
		numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		numWriter.WriteDec2(2, zdt.Month, 0)
	case clockModeDay:
		numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		numWriter.WriteDec2(2, zdt.Day, 0)
	case clockModeHourMinute:
		numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
	case clockModeSecond:
		numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		numWriter.WriteDec2(2, zdt.Second, 0)
		numWriter.WriteColon(true)
	default:
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

func (h *ButtonHandler) Handle(
	b *button.Button, event button.Event, state bool) {

	switch b.GetPin() {
	case modePin:
		switch event {
		case button.EventPressed:
		case button.EventReleased:
			println("Mode Pressed")
			nextClockMode()
		case button.EventLongPressed:
		case button.EventRepeatPressed:
		case button.EventLongReleased:
		default:
		}
	case changePin:
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

var lastButtonMillis uint16

// Check buttons every 5 milliseconds
func checkButtons() {
	millis := uint16(time.Now().UnixMilli())
	if millis-lastButtonMillis > 5 {
		lastButtonMillis = millis
		modeButton.Check()
		changeButton.Check()
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
		flushDisplay()
		time.Sleep(time.Millisecond * 1)
	}
}
