# RemotePowerControl — Project Memory
**Last Updated:** 2026-06-05

## Abstract

**TL;DR:** Cross-phase project memory for RemotePowerControl — non-obvious, bug-causing hardware/firmware facts and the decisions/lessons that should survive across phases. PM writes here when a phase closes or a non-obvious lesson is learned.

**Load when:** memory, project history, lessons, hardware facts, optocoupler polarity, detect pin, pin map, dual-MCU, known bug, why was this decided

**Key facts:**
- This file holds *durable* project facts; live action items live in `NEXT_STEPS.md`, phase status in `plans/ROADMAP.md`.
- Owner `/pm` per `DOC_OWNERSHIP_MATRIX.md`; specialists read it, PM updates it.
- Seeded at bootstrap (2026-06-05) with three non-obvious facts that have caused or can cause inverted-state / boot bugs.

**Owner:** `/pm` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `plans/ROADMAP.md`, `NEXT_STEPS.md`, `design/POWER_STATE.md`, `design/HARDWARE.md`

---

## Hardware & firmware facts (seeded at bootstrap)

### 1. Detect-GPIO polarity — LOW = power ON (the #1 inversion trap)
The optocoupler detect input is `INPUT_PULLUP` and the optocoupler pulls it **LOW when the controlled device's power is ON** (power LED high), **HIGH when OFF**. Rapid pulsing on the detect line = the device's sleep/standby LED. Inverting this reading flips **every** reported power state. Owner of the electrical rationale: `/power`; full detail in `design/POWER_STATE.md` + `design/HARDWARE.md`.

### 2. Open bug — detect pin disconnected while device is ON
When the controlled PC is ON and the power-detect pin is physically disconnected, the power-on status **sticks ON on one channel (D0) but not another (D5)** — a floating-input edge case in the validation timer (`README.txt` VERIFY block). Reproduce before "fixing." Phase 1 work item, owner `/power`.

### 3. Dual-MCU boot-safe pin maps (`Application.h`)
Relay/switch output pins **must be high-impedance during reset/boot** or the relay fires on every flash/reset (boot-safe-GPIO invariant). Maps are per-MCU:
- **ESP8266** (`d1_mini`): SWITCH1=D1, SWITCH2=D8, SWITCH3=D2; DETECT1=D7, DETECT2=D6, DETECT3=D5; BUTTON=D0; LED=D4.
- **ESP32** (`wemos_d1_mini32`): SWITCH1=22, SWITCH2=5, SWITCH3=21; DETECT1=23, DETECT2=19, DETECT3=18; BUTTON=26; LED=16.

D1/GPIO5 & D2/GPIO4 on ESP8266 were chosen specifically because they stay high-impedance through reset. Pin map owner `/firmware`; electrical rationale `/power`.

---

## Decisions

- 2026-06-05 — Agent system bootstrapped from BOOTSTRAP_PLAN: 9 agents (pm/arch/review/scm + power/mqtt/net/web/firmware). `main.cpp` C6 section banners inserted. `doc/MEMORY.md` seeded (Q-MEMORY=yes).

## Open Questions (deferred from §12 — user decisions, not yet resolved)

- Office vs home build split (`build_flags_office`/`SMARTHOME_*_OFFICE`) — active or legacy? Affects `BUILD.md`.
- Dormant `detect_power_status_new()` — intended replacement for `_old()`, or delete? Affects `POWER_STATE.md` + a likely Phase 1 cleanup wave.
- Refactor the blocking relay pulses (`short_push` 800 ms / `long_push` 10 s) to non-blocking within v1? Affects a `/power` Phase 1 wave.
- HA MQTT auto-discovery in Phase 1, or stay manually-configured? Affects a `/mqtt` wave.
- Target boards beyond `d1_mini` / `wemos_d1_mini32`? Affects `HARDWARE.md` per-board sections.
