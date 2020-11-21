#ifndef OLED_CLOCK_LCD_PRESENTER_H
#define OLED_CLOCK_LCD_PRESENTER_H

#include <Adafruit_PCD8544.h>
#include "Presenter.h"

/**
 * Subclass of Presenter that knows how to render to a PCD8554 LCD display
 * (aka Nokia 5110) using Adafruit's driver. Unlike the SSD1306 driver, the
 * Adafruit driver does not overwrite the existing background bits of a
 * character (i.e. bit are only turns on, not turned off.) We can either use a
 * double-buffered canvas (did not want to take the time right now to figure
 * that out), or we can erase the entire screen before rendering each frame.
 * Normally, this would cause a flicker in the display. However, it seems like
 * the LCD pixels have so much latency that I don't see any flickering at all.
 * It works, so I'll just keep it like that for now.
 */
class LcdPresenter : public Presenter {
  public:
    LcdPresenter(Adafruit_PCD8544& lcd) :
        Presenter(lcd, false /*isOverwriting*/),
        mLcd(lcd)
      {}

    void clearDisplay() override {
      mLcd.clearDisplay();
    }

    void home() override {
      mLcd.setCursor(0, 0);
    }

    void renderDisplay() override {
      mLcd.display();
    }

    void setFont() override {}

    void clearToEOL() override {}

  private:
    Adafruit_PCD8544& mLcd;
};

#endif
