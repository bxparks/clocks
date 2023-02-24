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
	clockModeYear ClockMode = iota
	clockModeMonth
	clockModeDay
	clockModeHourMinute
	clockModeSecond
)

type ClockInfo struct {
	hourMode   HourMode              // 12/24 mode
	clockMode  ClockMode             // clock display mode
	brightness uint8                 // [0,7] for Tm1637Display
	zoneId     uint32                // desired timeZoneData.
	dateTime   acetime.ZonedDateTime // DateTime
}
