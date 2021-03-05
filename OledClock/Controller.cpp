#include "Controller.h"

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD

const uint16_t Controller::kBacklightValues[10] = {
    0, 64, 91, 128, 181, 256, 362, 512, 724, 1023
};

#else

const uint8_t Controller::kContrastValues[10] = {
    // Can't start from 0 because that would turn off the display
    // completely, and prevent us from doing anything else.
    25, 50, 75, 100, 125, 150, 175, 200, 225, 255
};

#endif

