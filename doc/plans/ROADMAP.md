# remotepowercontrol — Roadmap
**Last Updated:** 2026-06-05

## Abstract

**TL;DR:** High-level phase overview for remotepowercontrol. PM keeps status badges current.

**Load when:** planning, phase overview, roadmap, status, next phase

**Key facts:**
- PM owns this file per DOC_OWNERSHIP_MATRIX.md
- Status badges: PLANNED / IN_PROGRESS / VERIFIED / COMPLETE

**Owner:** `/pm` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `NEXT_STEPS.md`, `DOC_OWNERSHIP_MATRIX.md`

---

Phases derived from `README.txt` (TODO + VERIFY) and the v1 boundary. Wave maps are mostly **serial** (solo operator; most work converges on `main.cpp`). Full wave detail: `doc/BOOTSTRAP_PLAN/08_wave_mapping.md`.

## Phase 0 — Bootstrap & baseline
> Status: IN_PROGRESS (agent system bootstrapped 2026-06-05; design-doc bodies pending)

Active: pm, arch, firmware, power, mqtt.
- [x] 0.1 Apply bootstrap (skills, agents.json, matrix, design-doc stubs) — `pm`
- [ ] 0.2 Author `ARCHITECTURE.md` (module map, init order, loop contract) — `arch`
- [ ] 0.3 Author `BUILD.md` + `HARDWARE.md` (env matrix, pin maps, secrets) — `firmware`
- [ ] 0.4 Author `POWER_STATE.md` + `MQTT_API.md` from current code — `power`, `mqtt`
- [ ] 0.5 Verify clean build on `RELEASE_OTA_esp8266` and `RELEASE_COM_esp32dv` — `firmware`

## Phase 1 — MQTT state reliability & HA correctness (README VERIFY)
> Status: PLANNED

Active: pm, mqtt, power, net, review.
- [ ] 1.1 Verify `EVENT_MQTT_RESTORED` re-publishes **all 3** retained switch states (C4 + "resend all status" TODO) — `mqtt`
- [ ] 1.2 Verify LWT marks device offline in HA on power-off/restart — `mqtt`
- [ ] 1.3 Reproduce + diagnose the detect-pin-disconnected → status-sticks-ON bug — `power`
- [ ] 1.4 Fix the floating-detect-input validation-timer edge case — `power`
- [ ] 1.5 Audit retained-flag correctness + heap impact — `review`

## Phase 2 — Web UI: unavailable/offline states (README TODO)
> Status: PLANNED

Active: pm, web, power, review.
- [ ] 2.1 Extend `/getstatus` JSON + `decodePowerStat` for offline/disabled marker (C5) — `power`, `web`
- [ ] 2.2 Render greyed/Unavailable card + disabled toggle when offline — `web`
- [ ] 2.3 Reflect device-level offline (no recent poll) as all-grey — `web`
- [ ] 2.4 Review ESP8266/ESP32 `WebServer` parity + PROGMEM size — `review`

## Phase 3 — Persistence & WiFi setup portal (README TODO)
> Status: PLANNED

Active: pm, arch, net, firmware, review. *Likely fires a technology-boundary trigger (a `/config` agent).*
- [ ] 3.1 Design persistence (LittleFS/EEPROM) + setup-portal flow → `PHASE_3_DESIGN.md` — `arch`
- [ ] 3.2 Implement persistent config store (load on boot) — `firmware`
- [ ] 3.3 Implement WiFi setup/captive portal + button trigger — `net`
- [ ] 3.4 Wire persisted config into `setup()` — `firmware`
- [ ] 3.5 Review secrets-at-rest, boot-time fallback, dual-MCU flash layout — `review`

## Phase 4 — Telephone / IVR interface (post-v1, design only)
> Status: PLANNED

Active: pm, arch (+ possible new `/ivr` specialist).
- [ ] 4.1 Scope IVR/telephony (external gateway vs on-device) → `PHASE_4_DESIGN.md` — `arch`
- [ ] 4.2 Evaluate whether IVR warrants a new `/ivr` specialist — `pm`
