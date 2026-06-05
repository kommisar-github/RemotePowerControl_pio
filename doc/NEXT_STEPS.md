# remotepowercontrol — Next Steps
**Last Updated:** 2026-06-05

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

## Backlog
(PM populates as work is identified — sourced from `README.txt` TODO/VERIFY and the ROADMAP phases.)
