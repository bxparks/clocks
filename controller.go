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
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewYear
	case modeChangeMonth:
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewMonth
	case modeChangeDay:
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewDay
	case modeChangeHour:
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewHourMinute
	case modeChangeMinute:
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewHourMinute
	case modeChangeSecond:
		c.currInfo = c.changingInfo
		c.currInfo.clockMode = modeViewSecond
	}

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
