#ifndef WIDGET_H
#define WIDGET_H

#include "Button.h"
#include "ScreenManager.h"
#include "config_helper.h"

class Widget {
public:
    Widget(ScreenManager &manager);
    virtual ~Widget() = default;
    virtual void setup() = 0;
    virtual void update(bool force = false) = 0;
    virtual void draw(bool force = false) = 0;
    virtual void buttonPressed(uint8_t buttonId, ButtonState state) = 0;
    virtual String getName() = 0;
    // Called every loop iteration for every widget, regardless of which one is currently
    // shown. Widgets that only care about elapsed time while visible can ignore this;
    // widgets that need to keep tracking state while off-screen (e.g. a countdown timer
    // that must notice completion even if the user switched away) override it.
    virtual void backgroundTick() {}
    void setBusy(bool busy);

protected:
    ScreenManager &m_manager;
};
#endif // WIDGET_H