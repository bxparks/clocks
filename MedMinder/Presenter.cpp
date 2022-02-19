#include "Presenter.h"

const uint8_t Presenter::kOledContrastValues[10] = {
  // Can't start from 0 because that would turn off the display
  // completely, and prevent us from doing anything else.
  25, 50, 75, 100, 125, 150, 175, 200, 225, 255
};
