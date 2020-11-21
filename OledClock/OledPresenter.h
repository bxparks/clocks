#ifndef OLED_CLOCK_OLED_PRESENTER_H
#define OLED_CLOCK_OLED_PRESENTER_H

#include <SSD1306AsciiWire.h>
#include "Presenter.h"

/**
 * Subclass of Presenter that knows how to write to an SSD1306 OLED display
 * using the SSD1306Ascii driver. This driver overwrites the background pixels
 * when writing a single character, so we don't need to clear the entire
 * display before rendering a new version of the frame. However, we must make
 * sure that we clear the rest of the line with a clearToEOL() to prevent
 * unwanted clutter from the prior frame.
 */
class OledPresenter : public Presenter {
  public:
    OledPresenter(SSD1306Ascii& oled) :
        Presenter(oled, true /*isOverwriting*/),
        mOled(oled)
      {}

    void clearDisplay() override {
      mOled.clear();
    }

    void home() override {
      mOled.home();
    }

    void renderDisplay() override {}

    void setFont() override {
      mOled.setFont(fixed_bold10x15);
    }

    void clearToEOL() override {
      mOled.clearToEOL();
    }

  private:
    SSD1306Ascii& mOled;
};

#endif
