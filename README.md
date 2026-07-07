

# Info Orbs — Widget Expansion (Simon Geiger's fork)

This is a personal fork of [brettdottech/info-orbs](https://github.com/brettdottech/info-orbs), an open source ESP32-based desk display with five round TFT screens ("orbs"). This fork adds new widgets on top of the upstream project — currently a **Calendar widget** and a **Pomodoro Timer widget** (see below).

> This fork isn't affiliated with or supported by the upstream project's Discord/community — please only reach out there for questions about the base hardware/firmware, not for issues specific to the widgets added here.

## Getting the hardware & base setup

This fork doesn't change the hardware or the base setup process, so refer to the upstream project for all of that:

- [Get a Dev Kit](https://brett.tech/collections/electronics-projects/products/info-orbs-full-dev-kit) — all the parts you need, pre-picked.
- [Firmware Install / Flashing Guide](references/Firmware%20Install%20Guide.md) and [YouTube assembly/flashing walkthrough](https://link.brett.tech/orbsYT).
- [Upstream README](https://github.com/brettdottech/info-orbs) — wiring diagram, dev environment (PlatformIO) setup, and the full list of existing widgets (clock, weather, stocks, MQTT, parqet.com, custom web data) and their configuration.
- [Discord](https://link.brett.tech/discord) for general project support and community.

Once you have the base project cloned, built, and flashing successfully (per the guides above), come back here for the widgets this fork adds.

## Calendar Widget

Your calendar on the orbs: a clock, a month grid, and an agenda — synced from any standard `.ics` feed (Google Calendar, Outlook/Microsoft 365, iCloud, etc.). Full design rationale in [ADR-001](docs/adr/ADR-001-calendar-widget.md).

<!-- TODO: screenshot of the calendar widget in action -->

**Orbs:**
- **1 — Clock.** Reuses the existing clock widget.
- **2 — Month grid.** Current month, today highlighted, plus a last-synced indicator.
- **3–5 — Agenda**, toggleable between *Next 3 events* and *Next 3 days*; an event happening right now is ringed.

Recurring events (daily/weekly/monthly/yearly, including patterns like "2nd Wednesday of the month") are expanded, with individually-rescheduled/cancelled occurrences and declined/cancelled events filtered out automatically. Calendars in a different timezone than your device convert correctly for a curated set of common named timezones.

**Setup:**
1. Get a secret `.ics` URL from your provider (Google Calendar: Settings → your calendar → "Secret address in iCal format"; Outlook: Settings → Shared calendars → "Publish a calendar"). Treat it like a password.
2. In `firmware/config/config.h` (created from `config.h.template` per the upstream setup guide):
   ```c
   #define CALENDAR_ICS_URL "https://your-calendar-provider.example/your-secret-feed.ics"
   ```
3. Optionally raise `CALENDAR_MAX_EVENTS` (default 40) for a busier calendar:
   ```c
   #define CALENDAR_MAX_EVENTS 80
   ```
4. Flash as usual. Syncs at boot and every hour after that.

**Controls:**
- Middle short press — switch between page 1/2 of the current agenda mode.
- Middle medium press — switch orbs 3–5 between "Next 3 events" and "Next 3 days".
- Middle long press — force an immediate re-sync.
- Left/right — cycle to other widgets, same as everywhere else.

**Limitations:** one `.ics` source at a time; timezones handled for a curated list of common named zones only (not full `VTIMEZONE` parsing — see ADR-001); only verified against Google Calendar and Outlook/Microsoft 365 so far.

## Pomodoro Timer Widget

An offline Pomodoro timer — Focus / Short Break / Long Break, entirely visual since the hardware has no speaker, with a tomato mascot whose expression changes per phase. Full design rationale in [ADR-002](docs/adr/ADR-002-pomodoro-widget.md).

<!-- TODO: screenshot of the pomodoro widget in action -->

**Orbs:**
- **1 — Clock.** Same layout as the weather widget's clock.
- **2 — Title + mascot.** Green (unripe) while preparing, determined with a sweat drop during Focus, smiling with rosy cheeks on a Short Break, asleep under a crescent moon on a Long Break.
- **3 — Cycle tracker.** 4 tomato icons tracking the current 4-Focus-session cycle (filled = completed, ringed = current, outlined = upcoming). Resets after a Long Break.
- **4 — Phase label**, e.g. "Focus Time #2", plus the completion message ("Focus completed", etc.) once a phase finishes.
- **5 — Countdown + progress ring.** MM:SS inside a circular arc that fills as the phase progresses. Shows on-device instructions instead during Preparation (no timer yet).

Since there's no audio, a phase finishing (naturally or via early completion) flashes the display and forces it back into view even from another widget — so it's never missed.

**Setup:** nothing required — always enabled. Optionally override the default durations (25 / 5 / 15 minutes) in `firmware/config/config.h`:
```c
#define POMODORO_FOCUS_MINUTES 25
#define POMODORO_SHORT_BREAK_MINUTES 5
#define POMODORO_LONG_BREAK_MINUTES 15
```

**Controls:**
- Middle short press — start from Preparation, or advance to the next phase once one has completed.
- Middle medium press — complete the current phase early (same effect as a natural completion).
- Middle long press — reset to Preparation and clear cycle progress. No confirmation prompt.
- Left/right — cycle to other widgets, same as everywhere else.

There is no pause — a running phase can only be completed or reset.

## Changelog

Notable changes in this fork (current version: **1.3.0**) are tracked in [CHANGELOG.md](CHANGELOG.md).

## License

This project (including this fork's additions) is licensed under AGPLv3 — see [LICENSE.txt](LICENSE.txt).
