package main

import (
	"gitlab.com/bxparks/coding/tinygo/segwriter"
)

type Presenter struct {
	numWriter *segwriter.NumberWriter
	currInfo  ClockInfo
	prevInfo  ClockInfo
}

func NewPresenter(numWriter *segwriter.NumberWriter) Presenter {
	return Presenter{
		numWriter: numWriter,
	}
}

func (p *Presenter) setClockInfo(info *ClockInfo) {
	p.currInfo = *info
}

func (p *Presenter) updateDisplay() {
	if p.prevInfo == p.currInfo {
		return
	}
	println("Presenter(): clockMode: ", p.currInfo.clockMode)
	p.prevInfo = p.currInfo
	zdt := &p.currInfo.dateTime

	switch p.currInfo.clockMode {
	case modeViewYear, modeChangeYear:
		p.numWriter.WriteDec4(0, uint16(zdt.Year), 0)
	case modeViewMonth, modeChangeMonth:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Month, 0)
	case modeViewDay, modeChangeDay:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Day, 0)
	case modeViewHourMinute, modeChangeHour, modeChangeMinute:
		p.numWriter.WriteHourMinute24(zdt.Hour, zdt.Minute)
	case modeViewSecond, modeChangeSecond:
		p.numWriter.WriteHexChar(0, segwriter.HexCharSpace)
		p.numWriter.WriteHexChar(1, segwriter.HexCharSpace)
		p.numWriter.WriteDec2(2, zdt.Second, 0)
		p.numWriter.WriteColon(true)
	}
}
