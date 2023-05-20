module github.com/bxparks/clocks

go 1.19

replace github.com/bxparks/AceButtonGo => ../AceButtonGo

replace github.com/bxparks/AceSegmentGo => ../AceSegmentGo

replace github.com/bxparks/acetimego => ../acetimego

require (
	github.com/bxparks/AceButtonGo v0.0.0-00010101000000-000000000000
	github.com/bxparks/AceSegmentGo v0.0.0-00010101000000-000000000000
	github.com/bxparks/acetimego v0.0.0-00010101000000-000000000000
	tinygo.org/x/drivers v0.24.0
)
