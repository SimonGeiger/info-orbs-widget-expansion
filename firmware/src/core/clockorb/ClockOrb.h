#ifndef CLOCKORB_H
#define CLOCKORB_H

#include "GlobalTime.h"
#include "ScreenManager.h"

// Shared "ambient clock" orb layout (date / big HH:MM / weekday), used by every widget
// whose orb 1 (or similar) shows the clock - weather, calendar, pomodoro - so the
// layout and its minute-resolution change-detection only live in one place.
class ClockOrb {
public:
    static int stamp(GlobalTime &time); // hour*60+minute; compare between calls to know when a redraw is needed
    static void draw(ScreenManager &manager, int screenIndex, GlobalTime &time, uint32_t fgColor = TFT_WHITE, uint32_t bgColor = TFT_BLACK);
};

#endif // CLOCKORB_H
