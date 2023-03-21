package main

import (
	"github.com/bxparks/AceSegmentGo/writer"
	"github.com/bxparks/AceTimeGo/acetime"
)

type Presenter struct {
	numWriter  *writer.NumberWriter
	charWriter *writer.CharWriter
	currInfo   ClockInfo
	prevInfo   ClockInfo
}

func NewPresenter(
	numWriter *writer.NumberWriter,
	charWriter *writer.CharWriter) Presenter {

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
	if p.prevInfo.brightness != p.currInfo.brightness {
		p.numWriter.Module().SetBrightness(p.currInfo.brightness)
	}

	p.prevInfo = p.currInfo
	zdt := &p.currInfo.dateTime
	p.numWriter.Home()

	switch p.currInfo.clockMode {
	case modeViewHourMinute:
		p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
	case modeChangeHour:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
		} else {
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDec2(zdt.Minute, 0)
			p.numWriter.WriteColon(true)
		}
	case modeChangeMinute:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
		} else {
			p.numWriter.WriteDec2(zdt.Hour, 0)
			p.numWriter.WriteColon(true)
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDigit(writer.DigitSpace)
		}
	case modeViewSecond:
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteColon(true)
		p.numWriter.WriteDec2(zdt.Second, 0)
	case modeChangeSecond:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDec2(zdt.Second, 0)
			p.numWriter.WriteColon(true)
		} else {
			p.numWriter.Clear()
		}
	case modeViewYear:
		p.numWriter.WriteDec4(uint16(zdt.Year), 0)
	case modeChangeYear:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDec4(uint16(zdt.Year), 0)
		} else {
			p.numWriter.Clear()
		}
	case modeViewMonth:
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteDec2(zdt.Month, 0)
	case modeChangeMonth:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDec2(zdt.Month, 0)
		} else {
			p.numWriter.Clear()
		}
	case modeViewDay:
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteDigit(writer.DigitSpace)
		p.numWriter.WriteDec2(zdt.Day, 0)
	case modeChangeDay:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDigit(writer.DigitSpace)
			p.numWriter.WriteDec2(zdt.Day, 0)
		} else {
			p.numWriter.Clear()
		}
	case modeViewWeekday:
		dt := p.currInfo.dateTime
		weekday := acetime.LocalDateToWeekday(dt.Year, dt.Month, dt.Day)
		weekdayName := weekday.Name()[:3]
		p.charWriter.WriteString(weekdayName)
		p.charWriter.ClearToEnd()
	case modeViewTimeZone:
		tzName := zones[p.currInfo.zoneIndex].name
		p.charWriter.WriteString(tzName)
		p.charWriter.ClearToEnd()
	case modeChangeTimeZone:
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			tzName := zones[p.currInfo.zoneIndex].name
			p.charWriter.WriteString(tzName)
			p.charWriter.ClearToEnd()
		} else {
			p.numWriter.Clear()
		}
	case modeViewBrightness:
		p.charWriter.WriteString("br")
		p.charWriter.SetDecimalPoint(1, true)
		p.numWriter.WriteDec2(p.currInfo.brightness, writer.DigitSpace)
	case modeChangeBrightness:
		p.charWriter.WriteString("br")
		p.charWriter.SetDecimalPoint(1, true)
		if p.currInfo.blinkShowState || p.currInfo.blinkSuppressed {
			p.numWriter.WriteDec2(p.currInfo.brightness, writer.DigitSpace)
		} else {
			p.charWriter.ClearToEnd()
		}
	case modeViewTempC:
		t := int8(p.currInfo.tempCentiC / 100)
		p.numWriter.WriteSignedDec3(t, writer.DigitSpace)
		p.numWriter.WriteDigit(0xC)
	case modeViewTempF:
		t := int8(p.currInfo.tempCentiF / 100)
		p.numWriter.WriteSignedDec3(t, writer.DigitSpace)
		p.numWriter.WriteDigit(0xF)
	}
}
