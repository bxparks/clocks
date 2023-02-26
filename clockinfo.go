package main

import (
	"github.com/bxparks/AceTimeGo/acetime"
)

type HourMode uint8

const (
	hourMode12 HourMode = 0 // 12:00:00 AM to 12:00:00 PM
	hourMode24 HourMode = 1 // 00:00:00 - 23:59:59
)

type ClockMode uint8

const (
	modeUnknown = iota
	modeViewYear
	modeViewMonth
	modeViewDay
	modeViewHourMinute
	modeViewSecond
	modeViewTemperature

	modeChangeYear
	modeChangeMonth
	modeChangeDay
	modeChangeHour
	modeChangeMinute
	modeChangeSecond
)

type ClockInfo struct {
	dateTime        acetime.ZonedDateTime // DateTime
	tempCentiC      int16                 // centi Celsius
	tempCentiF      int16                 // centi Fahrenheit
	clockMode       ClockMode             // clock display mode
	hourMode        HourMode              // 12/24 mode
	brightness      uint8                 // [0,7] for Tm1637Display
	blinkShowState  bool                  // true = show element
	blinkSuppressed bool                  // true = suppress blinking
}
