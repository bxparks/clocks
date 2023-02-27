package main

import (
	"github.com/bxparks/AceTimeGo/acetime"
	"gitlab.com/bxparks/coding/tinygo/segwriter"
)

type Presenter struct {
	numWriter  *segwriter.NumberWriter
	charWriter *segwriter.CharWriter
	currInfo   ClockInfo
	prevInfo   ClockInfo
}

func NewPresenter(
	numWriter *segwriter.NumberWriter,
	charWriter *segwriter.CharWriter) Presenter {

	return Presenter{
		numWriter:  numWriter,
		charWriter: charWriter,
	}
}

func (p *Presenter) SetClockInfo(info *ClockInfo) {
	p.currInfo = *info
}

func (p *Presenter) UpdateDisplay() {
	if p.prevInfo == p.currInfo {
		return
	}
	p.prevInfo = p.currInfo
	zdt := &p.currInfo.dateTime

	switch p.currInfo.clockMode {
	case modeViewYear:
		p.numWriter.WriteDec4(0, uint16(zdt.Year), 0)
	case modeChangeYear:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDec4(0, uint16(zdt.Year), 0)
		} else {
			p.numWriter.Module().Clear()
		}
	case modeViewMonth:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Month, 0)
	case modeChangeMonth:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
			p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
			p.numWriter.WriteDec2(2, zdt.Month, 0)
		} else {
			p.numWriter.Module().Clear()
		}
	case modeViewDay:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Day, 0)
	case modeChangeDay:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
			p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
			p.numWriter.WriteDec2(2, zdt.Day, 0)
		} else {
			p.numWriter.Module().Clear()
		}
	case modeViewHourMinute:
		p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
	case modeChangeHour:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
		} else {
			p.numWriter.Module().Clear()
			p.numWriter.WriteDec2(2, zdt.Minute, 0)
			p.numWriter.WriteColon(true)
		}
	case modeChangeMinute:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
		} else {
			p.numWriter.Module().Clear()
			p.numWriter.WriteDec2(0, zdt.Hour, 0)
			p.numWriter.WriteColon(true)
		}
	case modeViewSecond:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Second, 0)
		p.numWriter.WriteColon(true)
	case modeChangeSecond:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
			p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
			p.numWriter.WriteDec2(2, zdt.Second, 0)
			p.numWriter.WriteColon(true)
		} else {
			p.numWriter.Module().Clear()
		}
	case modeViewTimeZone:
		epochSeconds := p.currInfo.dateTime.EpochSeconds()
		extra := acetime.NewZonedExtraFromEpochSeconds(
			epochSeconds, p.currInfo.dateTime.Tz)
		abbrev := extra.Abbrev
		p.charWriter.WriteString(0, abbrev)
		p.charWriter.ClearToEnd(uint8(len(abbrev)))
	case modeViewTemperature:
		t := int8(p.currInfo.tempCentiC / 100)
		p.numWriter.WriteSignedDec3(0, t, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(3, 0xC)
	}
}
