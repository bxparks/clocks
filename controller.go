package main

import (
	"github.com/bxparks/AceTimeGo/acetime"
	"gitlab.com/bxparks/coding/tinygo/ds3231"
)

type Controller struct {
	presenter    *Presenter
	rtc          *ds3231.Device
	currInfo     ClockInfo
	changingInfo ClockInfo
}

func NewController(presenter *Presenter, rtc *ds3231.Device) Controller {
	return Controller{
		presenter: presenter,
		rtc:       rtc,
		currInfo: ClockInfo{
			hourMode:   hourMode24,
			clockMode:  modeViewHourMinute,
			brightness: 1,
		},
	}
}

func (c *Controller) handleModePress() {
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
	println("Controller: clockMode: ", c.currInfo.clockMode)

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()
}

func (c *Controller) handleModeLongPress() {
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
	}

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()
}

func (c *Controller) handleChangePress() {
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
	}

	c.changingInfo.clockMode = c.currInfo.clockMode
	c.updatePresenter()
}

func (c *Controller) syncRTC() {
	dt, err := c.rtc.ReadTime()
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
		0, /*Fold*/
	}
	zdt := acetime.NewZonedDateTimeFromLocalDateTime(&ldt, &tz)
	c.currInfo.dateTime = zdt
	c.updatePresenter()
}

func (c *Controller) saveClockInfo() {
	c.currInfo = c.changingInfo
	c.saveRTC(&c.currInfo)
}

func (c *Controller) saveRTC(info *ClockInfo) {
	println("saveRTC()")
	ldt := info.dateTime.LocalDateTime()
	dt := ds3231.DateTime{
		Year:    uint8(ldt.Year - 2000),
		Month:   ldt.Month,
		Day:     ldt.Day,
		Hour:    ldt.Hour,
		Minute:  ldt.Minute,
		Second:  ldt.Second,
		Weekday: acetime.LocalDateToDayOfWeek(ldt.Year, ldt.Month, ldt.Day),
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
		modeChangeMinute, modeChangeSecond:
		clockInfo = &c.changingInfo
	default:
		clockInfo = &c.currInfo
	}
	c.presenter.setClockInfo(clockInfo)
}
