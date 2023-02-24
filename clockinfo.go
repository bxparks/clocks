package main

import (
	"github.com/bxparks/AceTimeGo/acetime"
)

const (
	hourMode12 = 0 // 12:00:00 AM to 12:00:00 PM
	hourMode24 = 1 // 00:00:00 - 23:59:59
)

type ClockInfo struct {
	hourMode   uint8                 // 12/24 mode
	brightness uint8                 // [0,7] for Tm1637Display
	zoneId     uint32                // desired timeZoneData.
	dateTime   acetime.ZonedDateTime // DateTime
}
