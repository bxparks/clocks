//go:build goroutine && tinygo

// An LED clock, bringing together: AceTimeGo/acetime, button, and tm1637.
// This version uses Go routines.

package main

import (
	"runtime"
	"time"
)

//-----------------------------------------------------------------------------
// Presenter using:
// TM1637 LED Module
//-----------------------------------------------------------------------------

var lastFlushTime = time.Now()

// Flush LED display every 100 millis
func flushDisplay() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastFlushTime)
		if elapsed.Milliseconds() >= 100 {
			lastFlushTime = now
			tm.Flush()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Temperature, read every 10 seconds. The DS3231 updates every 64 seconds. But
// sometimes, the first ReadTemp() fails, so it's nice to try again about 10
// seconds after the failure.
//-----------------------------------------------------------------------------

var lastReadTempTime = time.Now()

func readTemperature() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastReadTempTime)
		if elapsed.Milliseconds() >= 10000 {
			lastReadTempTime = now
			controller.ReadTemp()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// System Time
//-----------------------------------------------------------------------------

var lastSyncRTCTime = time.Now()

func syncSystemTime() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastSyncRTCTime)
		if elapsed.Milliseconds() >= 100 {
			lastSyncRTCTime = now
			controller.SyncSystemTime()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------

var lastUpdateDisplayTime = time.Now()

func updateDisplay() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastUpdateDisplayTime)
		if elapsed.Milliseconds() >= 100 {
			lastUpdateDisplayTime = now
			presenter.UpdateDisplay()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Buttons
//-----------------------------------------------------------------------------

var lastCheckButtonsTime = time.Now()

// Check buttons every 5 milliseconds
func checkButtons() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastCheckButtonsTime)
		if elapsed.Milliseconds() >= 5 {
			lastCheckButtonsTime = now
			buttonGroup.CheckButtons()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Blinking
//-----------------------------------------------------------------------------

var lastBlinkTime = time.Now()

func blinkDisplay() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastBlinkTime)
		if elapsed.Milliseconds() >= 500 {
			lastBlinkTime = now
			controller.Blink()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Print debug info on the serial monitor port every few seconds.
//-----------------------------------------------------------------------------

var lastDebugTime = time.Now()

func printSerialMonitor() {
	for {
		now := time.Now()
		elapsed := now.Sub(lastDebugTime)
		if elapsed.Milliseconds() >= 2000 {
			lastDebugTime = now
			controller.printSerialMonitor()
		}
		runtime.Gosched()
	}
}

//-----------------------------------------------------------------------------
// Main loop
//-----------------------------------------------------------------------------

func main() {
	time.Sleep(time.Millisecond * 500)

	setupButtons()
	setupDisplay()
	setupI2C()
	setupRTC()
	controller.SetupSystemTimeFromRTC()
	tm.Flush()

	println("Creating go routines...")

	go checkButtons()
	go syncSystemTime()
	go readTemperature()
	go blinkDisplay()
	go updateDisplay()
	go flushDisplay()
	go printSerialMonitor()

	for {
		runtime.Gosched()
	}
}
