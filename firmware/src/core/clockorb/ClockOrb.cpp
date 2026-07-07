#include "ClockOrb.h"

#include "Utils.h"

namespace {
constexpr int DATE_Y = 50;
constexpr int CLOCK_Y = 120;
constexpr int WEEKDAY_Y = 190;
constexpr int FONT_SIZE = 66;
constexpr int COLON_GAP = 10;
} // namespace

int ClockOrb::stamp(GlobalTime &time) {
    return time.getHour() * 60 + time.getMinute();
}

void ClockOrb::draw(ScreenManager &manager, int screenIndex, GlobalTime &time, uint32_t fgColor, uint32_t bgColor) {
    manager.selectScreen(screenIndex);
    manager.fillScreen(bgColor);
    manager.setFontColor(fgColor, bgColor);

    manager.drawCentreString(time.getDayAndMonth(), ScreenCenterX, DATE_Y, 18);
    manager.drawCentreString(time.getWeekday(), ScreenCenterX, WEEKDAY_Y, 22);

    manager.drawString(time.getHourPadded(), ScreenCenterX - COLON_GAP, CLOCK_Y, FONT_SIZE, Align::MiddleRight);
    manager.drawString(":", ScreenCenterX, CLOCK_Y, FONT_SIZE, Align::MiddleCenter);
    manager.drawString(time.getMinutePadded(), ScreenCenterX + COLON_GAP, CLOCK_Y, FONT_SIZE, Align::MiddleLeft);
}
