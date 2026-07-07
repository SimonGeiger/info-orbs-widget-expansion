#include "PomodoroWidget.h"

#include "ClockOrb.h"
#include "Utils.h"
#include "config_helper.h"

namespace {
// Per-phase accent colors (RGB565)
constexpr uint32_t COLOR_PREPARATION = 0xFEA0; // warm yellow
constexpr uint32_t COLOR_FOCUS = TFT_RED;
constexpr uint32_t COLOR_SHORT_BREAK = 0x4DE8; // soft green
constexpr uint32_t COLOR_LONG_BREAK = 0x3D7F; // sky blue

// Mascot palette
constexpr uint32_t COLOR_TOMATO_BODY = 0xE1E5; // ripe tomato red
constexpr uint32_t COLOR_TOMATO_BODY_LIGHT = 0xFBCB; // gloss highlight on the red body
constexpr uint32_t COLOR_TOMATO_UNRIPE = 0x7DA8; // green (unripe) body for Preparation
constexpr uint32_t COLOR_TOMATO_UNRIPE_LIGHT = 0xA68D; // gloss highlight on the green body
constexpr uint32_t COLOR_LEAF = 0x2D07; // calyx/stem green
constexpr uint32_t COLOR_SWEAT_DROP = 0x65BF; // light blue
constexpr uint32_t COLOR_MOON = 0xFF92; // pale yellow
constexpr uint32_t COLOR_RING_TRACK = 0x2965; // dark gray background ring on orb 5

constexpr unsigned long FLASH_DURATION_MS = 1200;
constexpr unsigned long FLASH_TOGGLE_MS = 200;

// Shared clock layout (same as the weather widget's clock orb)
constexpr int CLOCK_Y = 120;
constexpr int CLOCK_FONT_SIZE = 66;
constexpr int CLOCK_COLON_GAP = 10;
} // namespace

PomodoroWidget::PomodoroWidget(ScreenManager &manager, WidgetSet &widgetSet) : Widget(manager), m_widgetSet(widgetSet) {
}

PomodoroWidget::~PomodoroWidget() {
}

void PomodoroWidget::setup() {
    // Note: setup() is called every time this widget becomes the current one (see
    // WidgetSet::switchWidget), not just once at boot - so it must only reset redraw
    // caches, never the phase/timer state itself.
    m_lastClockStamp = -1;
    m_mascotDrawn = false;
    m_lastTrackerCompletedCount = -1;
    m_labelDrawn = false;
    m_countdownDrawn = false;
}

unsigned long PomodoroWidget::focusDurationMs() {
    return (unsigned long)POMODORO_FOCUS_MINUTES * 60000UL;
}

unsigned long PomodoroWidget::shortBreakDurationMs() {
    return (unsigned long)POMODORO_SHORT_BREAK_MINUTES * 60000UL;
}

unsigned long PomodoroWidget::longBreakDurationMs() {
    return (unsigned long)POMODORO_LONG_BREAK_MINUTES * 60000UL;
}

void PomodoroWidget::startPhase(Phase phase, unsigned long durationMs) {
    m_phase = phase;
    m_phaseDurationMs = durationMs;
    m_phaseStartMillis = millis();
    m_timerRunning = true;
    m_awaitingAdvance = false;
}

void PomodoroWidget::completePhase() {
    m_timerRunning = false;
    m_awaitingAdvance = true;
    if (m_phase == Phase::FOCUS) {
        m_completedFocusCount++;
    }
    m_flashActive = true;
    m_flashStartMillis = millis();
    if (m_widgetSet.getCurrent() != this) {
        m_widgetSet.switchToWidget(this);
    }
}

void PomodoroWidget::advanceAfterCompletion() {
    switch (m_phase) {
    case Phase::FOCUS:
        if (m_completedFocusCount < 4) {
            startPhase(Phase::SHORT_BREAK, shortBreakDurationMs());
        } else {
            startPhase(Phase::LONG_BREAK, longBreakDurationMs());
        }
        break;
    case Phase::SHORT_BREAK:
        startPhase(Phase::FOCUS, focusDurationMs());
        break;
    case Phase::LONG_BREAK:
        m_phase = Phase::PREPARATION;
        m_completedFocusCount = 0;
        m_timerRunning = false;
        m_awaitingAdvance = false;
        break;
    case Phase::PREPARATION:
        // Not reachable: Preparation never sets m_awaitingAdvance.
        break;
    }
}

void PomodoroWidget::resetCycle() {
    m_phase = Phase::PREPARATION;
    m_timerRunning = false;
    m_awaitingAdvance = false;
    m_completedFocusCount = 0;
    m_flashActive = false;
}

void PomodoroWidget::checkExpiry() {
    if (m_timerRunning && millis() - m_phaseStartMillis >= m_phaseDurationMs) {
        completePhase();
    }
}

void PomodoroWidget::backgroundTick() {
    checkExpiry();
}

void PomodoroWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId != BUTTON_OK) {
        return;
    }
    switch (state) {
    case BTN_SHORT:
        if (m_awaitingAdvance) {
            advanceAfterCompletion();
        } else if (m_phase == Phase::PREPARATION) {
            startPhase(Phase::FOCUS, focusDurationMs());
        }
        break;
    case BTN_MEDIUM:
        if (m_timerRunning) {
            completePhase();
        }
        break;
    case BTN_LONG:
        resetCycle();
        break;
    default:
        break;
    }
}

void PomodoroWidget::update(bool force) {
    // Nothing to do here: backgroundTick() (called on every widget every loop, this one
    // included) is the sole owner of checkExpiry() - it covers this widget whether or
    // not it's currently on screen, so there's no separate "while visible" check needed.
}

uint32_t PomodoroWidget::accentColorForPhase(Phase phase) {
    switch (phase) {
    case Phase::FOCUS:
        return COLOR_FOCUS;
    case Phase::SHORT_BREAK:
        return COLOR_SHORT_BREAK;
    case Phase::LONG_BREAK:
        return COLOR_LONG_BREAK;
    default:
        return COLOR_PREPARATION;
    }
}

int PomodoroWidget::activeFocusNumber() {
    // m_completedFocusCount only advances when a Focus session completes, so while one
    // is running it's still one behind the session in progress; during the Short Break
    // that follows, it has already caught up to that same session's number.
    return m_phase == Phase::SHORT_BREAK ? m_completedFocusCount : m_completedFocusCount + 1;
}

String PomodoroWidget::phaseRunningLabel() {
    switch (m_phase) {
    case Phase::FOCUS:
        return "Focus Time #" + String(activeFocusNumber());
    case Phase::SHORT_BREAK:
        return "Short Break #" + String(activeFocusNumber());
    case Phase::LONG_BREAK:
        return "Long Break";
    default:
        return "Preparation";
    }
}

String PomodoroWidget::phaseCompletedLabel() {
    switch (m_phase) {
    case Phase::FOCUS:
        return "Focus completed";
    case Phase::SHORT_BREAK:
        return "Short Break completed";
    case Phase::LONG_BREAK:
        return "Long Break completed";
    default:
        return "";
    }
}

bool PomodoroWidget::isFlashing() {
    // Wrap-safe: unsigned subtraction handles millis() wrapping around after ~49.7
    // days, unlike a direct comparison against a precomputed end timestamp would.
    return m_flashActive && millis() - m_flashStartMillis < FLASH_DURATION_MS;
}

int PomodoroWidget::flashFrame() {
    if (!isFlashing()) {
        return -1;
    }
    return (millis() / FLASH_TOGGLE_MS) % 2;
}

void PomodoroWidget::draw(bool force) {
    drawClock(force);
    drawMascot(force);
    drawCycleTracker(force);
    drawPhaseLabel(force);
    drawCountdown(force);
}

// ---- Orb 1: clock (same layout as the weather widget's clock orb) ----

void PomodoroWidget::drawClock(bool force) {
    GlobalTime *time = GlobalTime::getInstance();
    int stamp = ClockOrb::stamp(*time);
    if (!force && stamp == m_lastClockStamp) {
        return;
    }
    m_lastClockStamp = stamp;
    ClockOrb::draw(m_manager, 0, *time);
}

// ---- Orb 2: title + mascot ----

void PomodoroWidget::drawTomato(int cx, int cy, int r, uint32_t bodyColor, uint32_t bodyLight, uint32_t leafColor) {
    // Body with a gloss highlight
    m_manager.fillCircle(cx, cy, r, bodyColor);
    m_manager.fillCircle(cx - r / 3, cy - r / 3, r / 6, bodyLight);
    m_manager.fillCircle(cx - r / 2, cy - r / 6, r / 12, bodyLight);

    // Stem + calyx leaves
    m_manager.fillRect(cx - 2, cy - r - 8, 5, 12, leafColor);
    m_manager.fillTriangle(cx - r / 2, cy - r + 16, cx - 4, cy - r - 4, cx - 2, cy - r + 12, leafColor);
    m_manager.fillTriangle(cx + 2, cy - r + 12, cx + 4, cy - r - 4, cx + r / 2, cy - r + 16, leafColor);
    m_manager.fillTriangle(cx - 8, cy - r + 2, cx + 8, cy - r + 2, cx, cy - r + 18, leafColor);
}

void PomodoroWidget::drawMascot(bool force) {
    int frame = flashFrame();
    if (!force && m_mascotDrawn && m_phase == m_lastMascotPhase && frame == m_lastMascotFlashFrame) {
        return;
    }
    m_mascotDrawn = true;
    m_lastMascotPhase = m_phase;
    m_lastMascotFlashFrame = frame;

    uint32_t accent = accentColorForPhase(m_phase);
    // Completion cue: pulse the background white a few times (no audio available)
    uint32_t bg = (frame == 0) ? TFT_WHITE : TFT_BLACK;

    bool unripe = m_phase == Phase::PREPARATION;
    uint32_t body = unripe ? COLOR_TOMATO_UNRIPE : COLOR_TOMATO_BODY;
    uint32_t bodyLight = unripe ? COLOR_TOMATO_UNRIPE_LIGHT : COLOR_TOMATO_BODY_LIGHT;
    uint32_t face = TFT_BLACK;

    m_manager.selectScreen(1);
    m_manager.fillScreen(bg);
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(frame == 0 ? TFT_BLACK : accent, bg);
    m_manager.drawCentreString("Pomodoro", ScreenCenterX, 26, 20);

    const int cx = ScreenCenterX;
    const int cy = ScreenCenterY + 22;
    const int r = 56;
    drawTomato(cx, cy, r, body, bodyLight, COLOR_LEAF);

    const int eyeDx = 22;
    const int eyeY = cy - 8;
    switch (m_phase) {
    case Phase::FOCUS:
        // Determined: thick angled brows, round eyes, straight mouth, sweat drop
        m_manager.drawLine(cx - eyeDx - 10, eyeY - 16, cx - eyeDx + 8, eyeY - 9, face);
        m_manager.drawLine(cx - eyeDx - 10, eyeY - 15, cx - eyeDx + 8, eyeY - 8, face);
        m_manager.drawLine(cx + eyeDx + 10, eyeY - 16, cx + eyeDx - 8, eyeY - 9, face);
        m_manager.drawLine(cx + eyeDx + 10, eyeY - 15, cx + eyeDx - 8, eyeY - 8, face);
        m_manager.fillCircle(cx - eyeDx, eyeY, 6, face);
        m_manager.fillCircle(cx + eyeDx, eyeY, 6, face);
        m_manager.drawLine(cx - 14, cy + 22, cx + 14, cy + 22, face);
        // Sweat drop off the right side of the body
        m_manager.fillTriangle(cx + 40, cy - 46, cx + 48, cy - 46, cx + 44, cy - 56, COLOR_SWEAT_DROP);
        m_manager.fillCircle(cx + 44, cy - 43, 5, COLOR_SWEAT_DROP);
        break;
    case Phase::SHORT_BREAK:
        // Relaxed: happy "^" eyes (3px thick), rosy cheeks, open smile
        for (int t = 0; t < 3; t++) {
            m_manager.drawLine(cx - eyeDx - 8, eyeY + 2 + t, cx - eyeDx, eyeY - 6 + t, face);
            m_manager.drawLine(cx - eyeDx, eyeY - 6 + t, cx - eyeDx + 8, eyeY + 2 + t, face);
            m_manager.drawLine(cx + eyeDx - 8, eyeY + 2 + t, cx + eyeDx, eyeY - 6 + t, face);
            m_manager.drawLine(cx + eyeDx, eyeY - 6 + t, cx + eyeDx + 8, eyeY + 2 + t, face);
        }
        m_manager.fillCircle(cx - 36, cy + 8, 6, bodyLight);
        m_manager.fillCircle(cx + 36, cy + 8, 6, bodyLight);
        // Lower half-ring = smile (TFT arc angles: 0 = 6 o'clock, clockwise)
        m_manager.drawArc(cx, cy + 12, 20, 13, 270, 90, face, body);
        break;
    case Phase::LONG_BREAK:
        // Sleeping: closed eyes, small open mouth, "z z" drifting off, crescent moon
        m_manager.drawLine(cx - eyeDx - 8, eyeY, cx - eyeDx + 8, eyeY, face);
        m_manager.drawLine(cx - eyeDx - 8, eyeY + 1, cx - eyeDx + 8, eyeY + 1, face);
        m_manager.drawLine(cx + eyeDx - 8, eyeY, cx + eyeDx + 8, eyeY, face);
        m_manager.drawLine(cx + eyeDx - 8, eyeY + 1, cx + eyeDx + 8, eyeY + 1, face);
        m_manager.fillCircle(cx, cy + 20, 6, face);
        m_manager.setFontColor(TFT_DARKGREY, bg);
        m_manager.drawString("z", cx + 42, cy - 56, 18, Align::MiddleCenter);
        m_manager.drawString("z", cx + 56, cy - 70, 14, Align::MiddleCenter);
        // Crescent moon top-left: pale circle with an offset bg-colored bite
        m_manager.fillCircle(56, 66, 13, COLOR_MOON);
        m_manager.fillCircle(62, 61, 11, bg);
        break;
    default: // PREPARATION - green (unripe) tomato, round eyes, gentle smile
        m_manager.fillCircle(cx - eyeDx, eyeY, 6, face);
        m_manager.fillCircle(cx + eyeDx, eyeY, 6, face);
        // Shallow lower arc = slight smile (TFT arc angles: 0 = 6 o'clock, clockwise)
        m_manager.drawArc(cx, cy + 8, 18, 15, 310, 50, face, body);
        break;
    }
}

// ---- Orb 3: cycle tracker ----

void PomodoroWidget::drawMiniTomato(int x, int y, bool filled, bool highlighted) {
    if (highlighted) {
        m_manager.drawArc(x, y, 26, 23, 0, 360, TFT_WHITE, TFT_BLACK);
    }
    if (filled) {
        m_manager.fillCircle(x, y, 17, COLOR_TOMATO_BODY);
        m_manager.fillCircle(x - 6, y - 6, 3, COLOR_TOMATO_BODY_LIGHT);
        m_manager.fillTriangle(x - 6, y - 14, x + 6, y - 14, x, y - 24, COLOR_LEAF);
    } else {
        m_manager.drawCircle(x, y, 17, TFT_DARKGREY);
        m_manager.drawTriangle(x - 6, y - 14, x + 6, y - 14, x, y - 24, TFT_DARKGREY);
    }
}

void PomodoroWidget::drawCycleTracker(bool force) {
    // Only an *actively running* Focus session is "current" - while awaiting advance
    // (paused after a Focus session just completed), the next session hasn't started
    // yet and must still show as upcoming, not current.
    bool hasCurrent = m_phase == Phase::FOCUS && m_timerRunning;
    if (!force && m_completedFocusCount == m_lastTrackerCompletedCount && hasCurrent == m_lastTrackerCurrent) {
        return;
    }
    m_lastTrackerCompletedCount = m_completedFocusCount;
    m_lastTrackerCurrent = hasCurrent;

    m_manager.selectScreen(2);
    m_manager.fillScreen(TFT_BLACK);

    const int spacing = 52;
    const int startX = ScreenCenterX - spacing * 3 / 2;
    for (int i = 0; i < 4; i++) {
        bool completed = i < m_completedFocusCount;
        bool isCurrent = hasCurrent && i == m_completedFocusCount;
        drawMiniTomato(startX + i * spacing, ScreenCenterY, completed || isCurrent, isCurrent);
    }
}

// ---- Orb 4: phase label ----

void PomodoroWidget::drawPhaseLabel(bool force) {
    int frame = flashFrame();
    if (!force && m_labelDrawn && m_phase == m_lastLabelPhase && m_awaitingAdvance == m_lastLabelAwaiting && frame == m_lastLabelFlashFrame) {
        return;
    }
    m_labelDrawn = true;
    m_lastLabelPhase = m_phase;
    m_lastLabelAwaiting = m_awaitingAdvance;
    m_lastLabelFlashFrame = frame;

    uint32_t accent = accentColorForPhase(m_phase);
    uint32_t bg = (frame == 0) ? TFT_WHITE : TFT_BLACK;
    String text = m_awaitingAdvance ? phaseCompletedLabel() : phaseRunningLabel();

    m_manager.selectScreen(3);
    m_manager.fillScreen(bg);
    m_manager.drawArc(ScreenCenterX, ScreenCenterY, 120, 114, 0, 360, accent, bg);
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(frame == 0 ? TFT_BLACK : accent, bg);
    m_manager.drawFittedString(text, ScreenCenterX, ScreenCenterY - 10, 190, 55, Align::MiddleCenter);

    String hint;
    if (m_awaitingAdvance) {
        hint = "short press to continue";
    } else if (m_phase == Phase::PREPARATION) {
        hint = "short press to start";
    }
    if (hint.length() > 0) {
        m_manager.setFontColor(frame == 0 ? TFT_BLACK : TFT_DARKGREY, bg);
        m_manager.drawCentreString(hint, ScreenCenterX, 160, 13);
    }
}

// ---- Orb 5: countdown + progress ring ----

void PomodoroWidget::drawPreparationHelp() {
    m_manager.selectScreen(4);
    m_manager.fillScreen(TFT_BLACK);
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);

    String instructions = "Press middle button short to start the Pomodoro timer. Medium press to complete phase early. Long press to reset.";
    String lines[MAX_WRAPPED_LINES];
    int lineCount = Utils::getWrappedLines(lines, instructions, 24);
    int y = ScreenCenterY - (lineCount * 18) / 2 + 9;
    for (int i = 0; i < lineCount && lines[i].length() > 0; i++) {
        m_manager.drawCentreString(lines[i], ScreenCenterX, y + i * 18, 12);
    }
}

void PomodoroWidget::computeCountdownSlots() {
    // Proportional-font digits vary in ink width, so anchoring "MM"/"SS" strings by
    // their bounding box makes digits wander as the value changes. Instead each digit
    // gets a fixed slot (centered), sized from the widest digit glyph.
    m_manager.setFont(DEFAULT_FONT);
    int digitW = m_manager.getTextWidth("8", CLOCK_FONT_SIZE) + 8;
    int colonW = m_manager.getTextWidth(":", CLOCK_FONT_SIZE) + 8;
    m_countdownDigitX[1] = ScreenCenterX - (colonW + digitW) / 2;
    m_countdownDigitX[0] = m_countdownDigitX[1] - digitW;
    m_countdownDigitX[2] = ScreenCenterX + (colonW + digitW) / 2;
    m_countdownDigitX[3] = m_countdownDigitX[2] + digitW;
}

void PomodoroWidget::drawCountdownDigit(int slot, char digit, uint32_t color) {
    m_manager.setFontColor(color, TFT_BLACK);
    m_manager.drawString(String(digit), m_countdownDigitX[slot], CLOCK_Y, CLOCK_FONT_SIZE, Align::MiddleCenter);
}

void PomodoroWidget::drawCountdownFull(uint32_t accent, int sweep, const char *digits) {
    m_manager.selectScreen(4);
    m_manager.fillScreen(TFT_BLACK);

    // Background track ring + already-elapsed progress. TFT arc angles put 0 at
    // 6 o'clock increasing clockwise, so the ring starts at the top (180).
    m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, 0, 360, COLOR_RING_TRACK, TFT_BLACK);
    if (sweep >= 360) {
        m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, 0, 360, accent, TFT_BLACK);
    } else if (sweep > 0) {
        m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, 180, (180 + sweep) % 360, accent, TFT_BLACK);
    }

    computeCountdownSlots();
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    m_manager.drawString(":", ScreenCenterX, CLOCK_Y, CLOCK_FONT_SIZE, Align::MiddleCenter);
    for (int i = 0; i < 4; i++) {
        drawCountdownDigit(i, digits[i], TFT_WHITE);
    }
}

void PomodoroWidget::drawCountdown(bool force) {
    bool phaseChanged = m_phase != m_lastCountdownPhase;

    if (m_phase == Phase::PREPARATION) {
        if (force || !m_countdownDrawn || phaseChanged) {
            drawPreparationHelp();
            m_countdownDrawn = true;
            m_lastCountdownPhase = m_phase;
            m_lastCountdownSeconds = -1;
        }
        return;
    }

    // A phase that is no longer running is over, whether it ran out naturally or was
    // completed early via medium press - show 00:00 and a full ring, not the time that
    // would have remained.
    unsigned long elapsed;
    if (!m_timerRunning) {
        elapsed = m_phaseDurationMs;
    } else {
        elapsed = millis() - m_phaseStartMillis;
        if (elapsed > m_phaseDurationMs) {
            elapsed = m_phaseDurationMs;
        }
    }
    int remainingSeconds = (int)((m_phaseDurationMs - elapsed) / 1000);
    int sweep = m_phaseDurationMs == 0 ? 360 : (int)((uint64_t)elapsed * 360 / m_phaseDurationMs);

    bool fullRedraw = force || !m_countdownDrawn || phaseChanged;
    if (!fullRedraw && remainingSeconds == m_lastCountdownSeconds && sweep == m_lastRingSweep) {
        return;
    }

    // The digit layout only has room for MM:SS (2+2 digits) - a configured duration of
    // 100+ minutes would otherwise silently truncate (e.g. 120:00 rendering as "12:00").
    // Clamp what's *displayed* to 99:59; the ring's sweep is unaffected and keeps
    // reflecting the real elapsed fraction of the phase.
    int displaySeconds = remainingSeconds > 5999 ? 5999 : remainingSeconds;
    char digits[5];
    snprintf(digits, sizeof(digits), "%02d%02d", displaySeconds / 60, displaySeconds % 60);
    uint32_t accent = accentColorForPhase(m_phase);

    if (fullRedraw) {
        drawCountdownFull(accent, sweep, digits);
    } else {
        // Incremental per-second update: paint only the newly-elapsed slice of the
        // ring, and re-draw only the digits that changed. Each digit lives in a fixed
        // slot and is erased by re-drawing the same glyph in black at the same anchor
        // (pixel-identical, so no anti-aliasing remainders) - the trick ClockWidget
        // uses - avoiding the flash a full-screen clear every second would cause.
        m_manager.selectScreen(4);
        if (sweep > m_lastRingSweep) {
            if (sweep >= 360) {
                // TFT_eSPI's drawArc() no-ops when startAngle == endAngle, which
                // (180 + 0) % 360 == 180 would hit if nothing had been drawn yet (e.g.
                // completing a phase early within its first ring-degree) - draw the
                // full circle directly in that case instead of trying to "close" it.
                if (m_lastRingSweep <= 0) {
                    m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, 0, 360, accent, TFT_BLACK);
                } else {
                    m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, (180 + m_lastRingSweep) % 360, 180, accent, TFT_BLACK);
                }
            } else {
                m_manager.drawArc(ScreenCenterX, ScreenCenterY, 116, 102, (180 + m_lastRingSweep) % 360, (180 + sweep) % 360, accent, TFT_BLACK);
            }
        }
        m_manager.setFont(DEFAULT_FONT);
        for (int i = 0; i < 4; i++) {
            if (digits[i] != m_lastCountdownDigits[i]) {
                drawCountdownDigit(i, m_lastCountdownDigits[i], TFT_BLACK);
                drawCountdownDigit(i, digits[i], TFT_WHITE);
            }
        }
    }

    m_countdownDrawn = true;
    m_lastCountdownPhase = m_phase;
    m_lastCountdownSeconds = remainingSeconds;
    m_lastRingSweep = sweep;
    memcpy(m_lastCountdownDigits, digits, sizeof(m_lastCountdownDigits));
}

String PomodoroWidget::getName() {
    return "Pomodoro";
}
