#ifndef POMODOROWIDGET_H
#define POMODOROWIDGET_H

#include "GlobalTime.h"
#include "Widget.h"
#include "WidgetSet.h"

// ADR-002: Pomodoro Timer Widget. No WiFi/network involved - all state lives in RAM and
// is timed off millis(), and resets on reboot (no persistence across power cycles).
class PomodoroWidget : public Widget {
public:
    PomodoroWidget(ScreenManager &manager, WidgetSet &widgetSet);
    ~PomodoroWidget() override;
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    void backgroundTick() override;
    String getName() override;

private:
    enum class Phase {
        PREPARATION,
        FOCUS,
        SHORT_BREAK,
        LONG_BREAK
    };

    // -- state machine --
    void startPhase(Phase phase, unsigned long durationMs);
    void completePhase(); // shared by natural expiry and medium-press early completion
    void advanceAfterCompletion(); // short press while m_awaitingAdvance
    void resetCycle(); // long press
    void checkExpiry();
    unsigned long focusDurationMs();
    unsigned long shortBreakDurationMs();
    unsigned long longBreakDurationMs();

    // -- rendering --
    void drawClock(bool force);
    void drawMascot(bool force);
    void drawCycleTracker(bool force);
    void drawPhaseLabel(bool force);
    void drawCountdown(bool force);
    void drawCountdownFull(uint32_t accent, int sweep, const char *digits);
    void computeCountdownSlots();
    void drawCountdownDigit(int slot, char digit, uint32_t color);
    void drawPreparationHelp();
    void drawTomato(int cx, int cy, int r, uint32_t bodyColor, uint32_t bodyLight, uint32_t leafColor);
    void drawMiniTomato(int x, int y, bool filled, bool highlighted);
    uint32_t accentColorForPhase(Phase phase);
    String phaseRunningLabel();
    String phaseCompletedLabel();
    bool isFlashing();
    int flashFrame(); // -1 when not flashing, else 0/1 alternating every FLASH_TOGGLE_MS

    WidgetSet &m_widgetSet;

    Phase m_phase = Phase::PREPARATION;
    bool m_timerRunning = false;
    bool m_awaitingAdvance = false;
    int m_completedFocusCount = 0; // 0-4, focus sessions completed in the current cycle
    int m_currentFocusNumber = 0; // 1-4, which focus session is running/just finished

    unsigned long m_phaseStartMillis = 0;
    unsigned long m_phaseDurationMs = 0;
    unsigned long m_flashEndMillis = 0; // completion pulse cue runs until this millis() timestamp

    // Redraw caches, so each orb is only repainted when something it shows actually changed
    int m_lastClockStamp = -1; // hour*60+minute
    bool m_mascotDrawn = false;
    Phase m_lastMascotPhase = Phase::PREPARATION;
    int m_lastMascotFlashFrame = -1;
    int m_lastTrackerCompletedCount = -1;
    bool m_lastTrackerCurrent = false;
    bool m_labelDrawn = false;
    Phase m_lastLabelPhase = Phase::PREPARATION;
    bool m_lastLabelAwaiting = false;
    int m_lastLabelFlashFrame = -1;
    bool m_countdownDrawn = false;
    Phase m_lastCountdownPhase = Phase::PREPARATION;
    int m_lastCountdownSeconds = -1;
    int m_lastRingSweep = 0; // degrees of the progress arc already painted
    char m_lastCountdownDigits[5] = ""; // "MMSS" as last drawn, one fixed slot per digit
    int m_countdownDigitX[4] = {0, 0, 0, 0}; // slot centers, measured from the font on first draw
};
#endif // POMODOROWIDGET_H
