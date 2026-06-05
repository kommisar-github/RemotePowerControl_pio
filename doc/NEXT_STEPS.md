# remotepowercontrol — Next Steps
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Current action items for remotepowercontrol. PM updates after each implementation/verification cycle.

**Load when:** current work, next steps, action items, todo, what to do next

**Key facts:**
- PM owns this file per DOC_OWNERSHIP_MATRIX.md
- Items flow: NEXT_STEPS → ROADMAP (in-progress) → MEMORY (complete)

**Owner:** `/pm` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `ROADMAP.md`, `DOC_OWNERSHIP_MATRIX.md`

---

## Immediate (Phase 0 — bootstrap complete, author design-doc bodies)
- [x] Agent system bootstrapped — 9 agents, matrix, design-doc stubs, `main.cpp` C6 banners, seeded `MEMORY.md`
- [ ] Install the VS Code Task Router extension (`.vsix`); confirm `/mcp` shows task-router Connected
- [ ] Start the PM terminal → `/pm` registers and lists 9 agents → `/pm ping` → expect 8 specialists healthy
- [ ] Wave 0.2 — `/arch` authors `ARCHITECTURE.md` body (module map, init order, loop contract)
- [ ] Wave 0.3 — `/firmware` authors `BUILD.md` + `HARDWARE.md` bodies from `platformio.ini` + `Application.h`
- [ ] Wave 0.4 — `/power` authors `POWER_STATE.md`, `/mqtt` authors `MQTT_API.md` from current code
- [ ] Wave 0.5 — `/firmware` verifies clean build on `RELEASE_OTA_esp8266` + `RELEASE_COM_esp32dv`

## Open decisions (deferred §12 — resolve before they gate work; see `doc/MEMORY.md` Open Questions)
- [ ] Office vs home build split — active or legacy? (gates `BUILD.md`)
- [ ] Dormant `detect_power_status_new()` — replace `_old()` or delete? (gates `POWER_STATE.md`)
- [ ] Refactor blocking relay pulses to non-blocking within v1? (gates a Phase 1 `/power` wave)
- [ ] HA MQTT auto-discovery in Phase 1, or stay manually configured? (gates a `/mqtt` wave)
- [ ] Target boards beyond `d1_mini` / `wemos_d1_mini32`? (gates `HARDWARE.md`)

## PM follow-up
- [ ] Archive `doc/BOOTSTRAP_PLAN.md` per `PM.md` → "Bootstrap plan — lifecycle and archive protocol" now that live artifacts exist (keep §7 contracts + §8 waves live or migrate to `ARCHITECTURE.md`).

## Audit remediation (from 2026-06-06 legacy-code audit — analysis complete, FIXES NOT YET STARTED)
Source of truth: `doc/audit/2026-06-06-audit.md` (`/review`-verified). Per-domain durable lessons captured in each `doc/<agent>_GUIDELINES.md`. `/review` readiness verdict: **4/10, NEEDS REVISION — not ESP32-release-ready until the 3 P0s land** (ESP8266 build passes boot-safe-GPIO). All fixes require explicit approval before any specialist edits source.

### P0 — blocks ESP32 release
- [ ] **F-01** ESP32 `POWER_SWITCH2=GPIO5` strapping pin → relay fires on every reset/flash. Reassign to a non-strapping pin (GPIO13/14/27), polarity sign-off `/power`. Owner `/firmware` (+`/power`). *(boot-safe-GPIO invariant)*
- [ ] **BUG-01** `RSwitch.cpp:308` `longPress()` dead `else if` → `UNDEFINED` on force-off. Owner `/power`. *(C2)*
- [ ] **C-1** `main.cpp:499` MQTT callback payload read without length guard (`if(length<3)return;`). Owner `/mqtt`. *(C3)*

### P1
- [ ] **net H2** `Test.cpp` reset `mqtt_conn_timestamp` in WiFi-lost branch → fixes dropped `EVENT_MQTT_RESTORED` / HA reconnect recovery. Owner `/net`. *(C4)*
- [ ] **net H1 / firmware C7** `#ifdef X && …` → `#if defined(X) && …` (OTA.cpp, MdnsClient.cpp). Owner `/net`.
- [ ] **info-JSON overflow** (mqtt M-2/M-3 + firmware F-07) — pool + 1024 B frame both overflow silently; raise to 2048 or split; check `publish()` return. Owner `/mqtt`.
- [ ] **BUG-03** `setSwitch(false)` double-pulse guard → `isPowerOn()` only. Owner `/power`. *(C3)*
- [ ] **F-03/F-04** Log macro name + const mismatches (`LOG_DDEBUG`/`LOG_DEBUG`, `LOG_ERROR`). Owner `/firmware`.
- [ ] **F-13** include-case `log.h` → `Log.h` repo-wide (9 includes) — **P0 if Linux CI is adopted**. Owner `/firmware`.

### P2
- [ ] Dead-code + parity + timing-edge cleanup (web `PAGE_BUTTONS_OLD`, `detect_power_status_new()`, `send_status()` decl, `inTopic` sub, `/getstatus` MIME, etc.). Various owners.
- [ ] web C1 placeholder substitution (cosmetic device-header labels — DOWNGRADED). Owner `/web`.

### Cross-cutting (from the audit's "silently-ignored-config" theme)
- [ ] **CI guard action item (PM-owned):** add `-Wundef`/`-Werror` (or targeted warnings) to `platformio.ini`; audit all `begin()` overloads for full arg pass-through; standardize on `#if defined()`. Design rationale lives in `ARCHITECTURE.md → Cross-cutting failure patterns` (`/arch`).
- [ ] git-history secret scan (`/scm`) — source clean; history scan pending (repo public since v1.0.0).

## Backlog
(PM populates as work is identified — sourced from `README.txt` TODO/VERIFY and the ROADMAP phases.)
