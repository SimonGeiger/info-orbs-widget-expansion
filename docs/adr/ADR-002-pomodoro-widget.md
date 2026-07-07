## ADR-002: Pomodoro Timer Widget

- **Status:** Accepted
- **Date:** 2026-07-04
- **Project:** brettdottech/info-orbs (ESP32 + 5x TFT desk widget)

### Context

Info Orbs is an ESP32-based desk display with five round TFT screens, cycled between via left/right buttons, with a middle button available for per-widget actions (short/medium/long press). The hardware has no audio output — display and buttons only. We want to add a Pomodoro timer as a new widget, requiring no network connectivity.

### Decisions

**1. No audio — visual-only transition cues**

The hardware has no buzzer/speaker. Phase transitions must be communicated entirely visually — e.g. a background color change and/or a brief flash/pulse at the moment a phase ends. See decision 8 for how this is escalated so it isn't missed while another widget is on screen.

**2. Four-phase model, with Preparation occurring once per cycle**

- Preparation — occurs once per full cycle: at the very start, and again after each Long Break. A short "get ready" phase with no timer; advancing out of it is a manual action (button press), not time-based.
- Focus (work session) — timer-driven, default 25 min.
- Short Break — timer-driven, default 5 min; follows each of the first three Focus sessions in a cycle.
- Long Break — timer-driven, default 15 min; follows the 4th Focus session in a cycle.

**Full cycle sequence:** Preparation → Focus #1 → Short Break → Focus #2 → Short Break → Focus #3 → Short Break → Focus #4 → Long Break → repeat (back to Preparation). Preparation does not recur between every Focus/Break pair — only once per full cycle.

**3. Manual phase progression — no auto-start, no pause**

The next phase (including leaving Preparation, and moving from a completed Short/Long Break into the next phase) only begins on an explicit button press; the timer never automatically rolls into the next phase at zero. There is no pause capability — a running Focus / Short Break / Long Break timer can only be completed (naturally, or early via medium press) or reset; it cannot be paused and resumed.

**4. No persistence across power cycles**

Session/cycle counters reset on reboot — explicitly decided against persisting history.

**5. Button behavior**

- Left / right press: cycle between widgets (unchanged, system-wide).
- Middle — short press: advance to the next phase (start). Leaves Preparation into Focus #1, and — since progression is never automatic — begins the next phase any time one has just completed.
- Middle — medium press: complete the current phase early. Behaves identically to the phase reaching zero naturally: the same completion signal fires (decisions 1 & 8), and cycle progress (orb 3) advances exactly as on natural completion.
- Middle — long press: reset (back to Preparation, clears current cycle progress). No confirmation step — see Risks.

**6. Five-orb layout**

| Orb | Content |
| --- | --- |
| 1 | Clock — reuse existing clock rendering |
| 2 | Pomodoro title + tomato mascot — static title; mascot appearance changes per phase (4 visual variants) |
| 3 | Cycle tracker — 4 tomato icons showing progress through the current 4-Focus-session cycle: completed filled solid, current highlighted, upcoming outlined. Resets after a Long Break. |
| 4 | Phase label (text) — e.g. "Focus Time #1", "Short Break #1", "Long Break"; also carries the completion message from decision 8 |
| 5 | Countdown + progress ring — MM:SS centered inside a circular progress arc |

Rationale for orb 2 vs orb 3 split: orb 2 answers "which phase am I in" (mascot), orb 3 answers "where am I in the 4-session cycle" (progress toward the long break) — kept distinct rather than overlapping.

**7. Progress ring: percentage-based, not fixed segment count**

The orb 5 ring fills based on percentage of the current phase's duration elapsed (continuous arc), rather than a fixed segment count — keeps fill behavior visually consistent across Focus/Short Break/Long Break with one code path.

**8. Auto-return to the Pomodoro widget on phase completion**

When a timer-driven phase (Focus, Short Break, Long Break) reaches zero — naturally or via early completion (medium press) — the display is forced back to the Pomodoro widget if the user is currently viewing a different one. This overrides normal widget cycling so a completion can't be missed just because another widget was on screen. Completion is signaled with the flash/pulse cue from decision 1, plus a text message on orb 4 naming the phase that just finished (e.g. "Focus completed", "Short Break completed"). This resolves the silent-completion risk below.

**Preparation-phase state (resolved):** since there's no timer to visualize, orb 5 shows instructional text during Preparation instead of the progress ring:

> Press middle button short to start the Pomodoro timer. Medium press to complete phase early. Long press to reset.
>

This doubles as an on-device help screen and matches the button mapping in decision 5.

### Risks

- **Silent completion (resolved by decision 8):** without an override, a phase could finish while the user was viewing a different widget, with no audio to signal it. Resolved by forcing a return to the Pomodoro widget and flashing + labeling the completed phase on arrival.
- **No confirmation on medium/long press (accepted):** completing a phase early or resetting the whole cycle happens on a single button press with no "hold to confirm" step. Accepted as a reasonable risk for a personal, single-user desk device.

### Components built

1. State machine — full cycle sequence (Preparation once per cycle; Focus/Short Break repeated across the cycle; Long Break on the 4th), no pause, medium-press early-completion treated identically to natural completion, reset handling. See `PomodoroWidget`.
2. Per-phase color/visual mapping — accent color and mascot variant (facial expression) per phase; the transition/completion cue uses a brief black/white flash (no audio available).
3. Orb renderers: simple digital clock reusing `GlobalTime` (orb 1, same approach `CalendarWidget` uses for its own clock orb), title + vector-drawn tomato mascot (orb 2), 4-icon cycle tracker (orb 3), phase label text incl. completion message (orb 4), countdown + progress ring (orb 5). Mascot and cycle-tracker icons are drawn with `ScreenManager` vector primitives (circles/triangles/arcs) rather than embedded bitmap assets.
4. Button handling — short press = advance/start, medium press = complete phase early, long press = reset. No pause handling needed.
5. Widget-switch override — `WidgetSet::switchToWidget()` forces a return to the Pomodoro widget when any timer-driven phase completes while another widget is active. Because `WidgetSet` only calls `update()` on the *current* widget, a new `Widget::backgroundTick()` hook (default no-op, called on every widget every loop) lets Pomodoro keep checking for phase expiry even while off-screen — otherwise a completed phase would never be noticed until the user manually switched back.
6. Config — default durations (25/5/15 min) as `#define`s in `config.h` (`POMODORO_FOCUS_MINUTES`, `POMODORO_SHORT_BREAK_MINUTES`, `POMODORO_LONG_BREAK_MINUTES`).

### Open items / future work

- Adjustable durations without reflashing would depend on the separately tracked Config Web Server feature idea — not required for v1.
- Daily/session history was considered and explicitly deferred (see decision 4).
- Mascot/icons are currently vector-drawn rather than illustrated bitmaps; revisit if a hand-drawn tomato asset is wanted later.
