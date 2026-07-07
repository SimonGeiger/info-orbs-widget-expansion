# Changelog

Notable changes made in this fork ([SimonGeiger/info-orbs-widget-expansion](https://github.com/SimonGeiger/info-orbs-widget-expansion)), on top of the upstream [brettdottech/info-orbs](https://github.com/brettdottech/info-orbs) project. Format loosely follows [Keep a Changelog](https://keepachangelog.com/), versioned independently from upstream.

## [1.3.0] - 2026-07-07

### Added

- **Pomodoro Timer widget** — offline Focus / Short Break / Long Break cycle with a tomato mascot, a 4-session cycle tracker, and a percentage-based countdown ring. See the [README](README.md#pomodoro-timer-widget) for usage and [ADR-002](docs/adr/ADR-002-pomodoro-widget.md) for the full technical design.
  - No network/WiFi involved; state lives in RAM and resets on reboot.
  - Manual phase progression only (no auto-start, no pause); medium press completes a phase early, long press resets the cycle.
  - Since there's no speaker, a completed phase flashes the display and forces it back into view even from another widget.
- `Widget::backgroundTick()` — a new no-op-by-default hook called on every widget every loop (not just the current one), so a widget can keep tracking time while off-screen. Used by the Pomodoro widget to notice phase completion even when another widget is being shown.
- `WidgetSet::switchToWidget()` — lets a widget force itself to become the currently displayed one.

### Changed

- Welcome screen now shows `version: 1.3.0`.
- Extracted the clock-orb layout (date / big `HH:MM` / weekday) shared by the weather, calendar, and Pomodoro widgets into a single `ClockOrb` helper (`firmware/src/core/clockorb/`), replacing three near-identical copies.

### Fixed

- Pomodoro: the countdown display silently showed the wrong time for configured durations of 100+ minutes (e.g. `120:00` rendered as `12:00`); it's now clamped to the `99:59` the digit layout can show.
- Pomodoro: completing a phase early within its first second left the orb 5 progress ring empty instead of full.
- Pomodoro: the completion flash could be skipped entirely in the rare case a phase finished within ~1.2s of `millis()` wrapping around (~49.7 days of uptime).
- Pomodoro: removed a redundant phase-expiry check that ran twice per loop iteration when the widget was on screen.

## [1.2.0] - 2026-07-06

### Added

- **Calendar widget** — clock, month grid, and a two-mode agenda (next 3 events / next 3 days) synced from any `.ics` feed (Google Calendar, Outlook/Microsoft 365 tested so far). See the [README](README.md#calendar-widget) for setup and [ADR-001](docs/adr/ADR-001-calendar-widget.md) for the full technical design.
  - `RRULE` recurrence expansion: `DAILY`/`WEEKLY`/`MONTHLY`/`YEARLY`, including ordinal `BYDAY` patterns (e.g. "2nd Wednesday of the month").
  - Deduplication of individually-rescheduled or cancelled recurring occurrences (`RECURRENCE-ID`/`EXDATE`).
  - Timezone-aware conversion for a curated set of common named timezones (`TZID`), independent of the device's own configured timezone.
  - Declined and cancelled events are filtered out automatically.
- Embedded Roboto Bold TTF font (`ROBOTO_BOLD`), used to render event titles in bold in the Calendar widget's agenda views.

### Changed

- Welcome screen now shows `version: 1.2.0`.
