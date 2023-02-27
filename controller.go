package main

import (
	"github.com/bxparks/AceTimeGo/acetime"
	"gitlab.com/bxparks/coding/tinygo/ds3231"
	"gitlab.com/bxparks/coding/tinygo/tm1637"
	"runtime"
	"time"
)

type Controller struct {
	presenter    *Presenter
	rtc          *ds3231.Device
	tm           *tm1637.Device
	currInfo     ClockInfo
	changingInfo ClockInfo
}

func NewController(
	presenter *Presenter, rtc *ds3231.Device, tm *tm1637.Device) Controller {

	return Controller{
		presenter: presenter,
		rtc:       rtc,
		tm:        tm,
		currInfo: ClockInfo{
			hourMode:   hourMode24,
			clockMode:  modeViewHourMinute,
			brightness: 1,
		},
	}
}

func (c *Controller) HandleModePress() {
	switch c.currInfo.clockMode {
	case modeViewYear:
		c.currInfo.clockMode = modeViewMonth
	case modeViewMonth:
		c.currInfo.clockMode = modeViewDay
	case modeViewDay:
		c.currInfo.clockMode = modeViewHourMinute
	case modeViewHourMinute:
		c.currInfo.clockMode = modeViewSecond
	case modeViewSecond:
		c.currInfo.clockMode = modeViewTimeZone
	case modeViewTimeZone:
		c.currInfo.clockMode = modeViewBrightness
	case modeViewBrightness:
		c.currInfo.clockMode = modeViewTemperature
	case modeViewTemperature:
		c.currInfo.clockMode = modeViewYear

	case modeChangeYear:
		c.currInfo.clockMode = modeChangeMonth
	case modeChangeMonth:
		c.currInfo.clockMode = modeChangeDay
	case modeChangeDay:
		c.currInfo.clockMode = modeChangeHour
	case modeChangeHour:
		c.currInfo.clockMode = modeChangeMinute
	case modeChangeMinute:
		c.currInfo.clockMode = modeChangeSecond
	case modeChangeSecond:
		c.currInfo.clockMode = modeChangeYear
	}

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()
}

func (c *Controller) HandleModeLongPress() {
	switch c.currInfo.clockMode {
	// Change to edit mode
	case modeViewYear:
		c.currInfo.clockMode = modeChangeYear
		c.changingInfo = c.currInfo
	case modeViewMonth:
		c.currInfo.clockMode = modeChangeMonth
		c.changingInfo = c.currInfo
	case modeViewDay:
		c.currInfo.clockMode = modeChangeDay
		c.changingInfo = c.currInfo
	case modeViewHourMinute:
		c.currInfo.clockMode = modeChangeHour
		c.changingInfo = c.currInfo
	case modeViewSecond:
		c.currInfo.clockMode = modeChangeSecond
		c.changingInfo = c.currInfo
	case modeViewTimeZone:
		c.currInfo.clockMode = modeChangeTimeZone
		c.changingInfo = c.currInfo
	case modeViewBrightness:
		c.currInfo.clockMode = modeChangeBrightness
		c.changingInfo = c.currInfo

	// Save edit and change back to normal mode.
	case modeChangeYear:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewYear
	case modeChangeMonth:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewMonth
	case modeChangeDay:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewDay
	case modeChangeHour:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewHourMinute
	case modeChangeMinute:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewHourMinute
	case modeChangeSecond:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewSecond
	case modeChangeTimeZone:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewTimeZone
	case modeChangeBrightness:
		c.saveClockInfo()
		c.currInfo.clockMode = modeViewBrightness
	}

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()
}

func (c *Controller) HandleChangePress() {
	c.currInfo.blinkSuppressed = true
	c.changingInfo.blinkSuppressed = true

	switch c.currInfo.clockMode {
	case modeChangeYear:
		c.changingInfo.dateTime.Year++
		if c.changingInfo.dateTime.Year >= 2100 {
			c.changingInfo.dateTime.Year = 2000
		}
	case modeChangeMonth:
		c.changingInfo.dateTime.Month++
		if c.changingInfo.dateTime.Month > 12 {
			c.changingInfo.dateTime.Month = 1
		}
	case modeChangeDay:
		c.changingInfo.dateTime.Day++
		if c.changingInfo.dateTime.Day > 31 {
			c.changingInfo.dateTime.Day = 1
		}
	case modeChangeHour:
		c.changingInfo.dateTime.Hour++
		if c.changingInfo.dateTime.Hour >= 24 {
			c.changingInfo.dateTime.Hour = 0
		}
	case modeChangeMinute:
		c.changingInfo.dateTime.Minute++
		if c.changingInfo.dateTime.Minute >= 60 {
			c.changingInfo.dateTime.Minute = 0
		}
	case modeChangeSecond:
		c.changingInfo.dateTime.Second++
		if c.changingInfo.dateTime.Second >= 60 {
			c.changingInfo.dateTime.Second = 0
		}
	case modeChangeTimeZone:
		c.changingInfo.zoneIndex++
		if int(c.changingInfo.zoneIndex) >= len(zones) {
			c.changingInfo.zoneIndex = 0
		}
	case modeChangeBrightness:
		c.changingInfo.brightness++
		if int(c.changingInfo.brightness) >= 8 {
			c.changingInfo.brightness = 0
		}
	}

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()

	// Flush the TM1637 immediately because if we wait until the next tm.Flush()
	// from main.go, the beat frequency between the button.Handle() events and the
	// tm.Flush() events causes a noticeable irregularity in the display update.
	c.tm.Flush()
}

func (c *Controller) HandleChangeRelease() {
	c.currInfo.blinkSuppressed = false
	c.changingInfo.blinkSuppressed = false
}

func (c *Controller) SyncSystemTime() {
	now := time.Now()
	tz := zones[c.currInfo.zoneIndex].tz
	zdt := acetime.NewZonedDateTimeFromEpochSeconds(
		acetime.ATime(now.Unix()), tz)
	c.currInfo.dateTime = zdt
	c.updatePresenter()
}

// Read the RTC and adjust the system time to match.
func (c *Controller) SetupSystemTimeFromRTC() {
	dt, err := rtc.ReadTime()
	if err != nil {
		return
	}
	nowRtc := time.Date(
		2000+int(dt.Year),
		time.Month(dt.Month),
		int(dt.Day),
		int(dt.Hour),
		int(dt.Minute),
		int(dt.Second),
		0, /*ns*/
		time.UTC)
	nowSystem := time.Now()
	offset := nowRtc.Sub(nowSystem)

	// See https://github.com/tinygo-org/tinygo/pull/3402 for usage info.
	runtime.AdjustTimeOffset(int64(offset))
}

func (c *Controller) Blink() {
	c.currInfo.blinkShowState = !c.currInfo.blinkShowState
	c.changingInfo.blinkShowState = !c.changingInfo.blinkShowState
	c.updatePresenter()
}

func (c *Controller) ReadTemp() {
	rawTemp, err := c.rtc.ReadTemp()
	if err != nil {
		return
	}
	c.currInfo.tempCentiC = ds3231.ToCentiC(rawTemp)
	c.currInfo.tempCentiF = ds3231.ToCentiF(rawTemp)
	c.updatePresenter()
}

//-----------------------------------------------------------------------------

func (c *Controller) saveClockInfo() {
	if c.currInfo.zoneIndex != c.changingInfo.zoneIndex {
		newTz := zones[c.changingInfo.zoneIndex].tz
		newZdt := c.currInfo.dateTime.ConvertToTimeZone(newTz)
		c.currInfo = c.changingInfo
		c.currInfo.dateTime = newZdt
	} else {
		c.currInfo = c.changingInfo
	}
	c.saveRTC(&c.currInfo)
	c.SetupSystemTimeFromRTC()
}

func (c *Controller) saveRTC(info *ClockInfo) {
	// Convert to UTC DateTime.
	udt := info.dateTime.ConvertToTimeZone(&acetime.TimeZoneUTC)
	ldt := udt.LocalDateTime()

	// Convert to DS3231 DateTime.
	year := ldt.Year - 2000
	var century uint8
	if year >= 100 {
		century = 1
	}
	dt := ds3231.DateTime{
		Year:    uint8(year),
		Month:   ldt.Month,
		Day:     ldt.Day,
		Hour:    ldt.Hour,
		Minute:  ldt.Minute,
		Second:  ldt.Second,
		Weekday: acetime.LocalDateToDayOfWeek(ldt.Year, ldt.Month, ldt.Day),
		Century: century,
	}
	err := c.rtc.SetTime(dt)
	if err != nil {
		println("saveRTC(): Error writing to RTC")
	}
}

// Update the presenter
func (c *Controller) updatePresenter() {
	var clockInfo *ClockInfo
	switch c.currInfo.clockMode {
	case modeChangeYear, modeChangeMonth, modeChangeDay, modeChangeHour,
		modeChangeMinute, modeChangeSecond, modeChangeTimeZone,
		modeChangeBrightness:
		clockInfo = &c.changingInfo
	default:
		clockInfo = &c.currInfo
	}
	c.presenter.SetClockInfo(clockInfo)
}
