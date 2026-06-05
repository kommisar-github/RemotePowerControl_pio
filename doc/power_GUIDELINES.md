# /power — Agent Guidelines
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Durable, project-specific guidelines for the `/power` agent (Relay control + power-state detection). This file is the agent's **only** sanctioned write target for notes that should survive across sessions. The agent appends here **only when PM or the user explicitly asks** (review-gated consolidation) — never as a side effect of doing work. Project-specific RSwitch timing/electrical conventions that should outlive a single phase land here on request.

**Load when:** the `/power` agent starts a session, or when PM is auditing roster consistency via `/pm audit`.

**Key facts:**
- Owner: `/power` (Primary), `/pm` (Secondary) — per DOC_OWNERSHIP_MATRIX.md
- Write trigger: explicit PM/user request only, routed through the `/review` consolidation gate. Cite the request verbatim in the commit message.
- All other auto-memory writes by specialists are forbidden (see SKILL.md `## Memory Policy`).

**Owner:** `/power` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `.claude/skills/power/SKILL.md`, `DOC_OWNERSHIP_MATRIX.md`, `POWER_STATE.md`

---

> **Provenance:** Entries below sourced from `doc/audit/2026-06-06-audit.md` (canonical audit) and confirmed by `/review` APPROVE verdict at the Wave 3 consolidation gate (2026-06-06). See also `doc/audit/2026-06-06-guidelines-drafts.md` §1. For cross-cutting patterns (silently-ignored-config), see ARCHITECTURE.md → Cross-cutting failure patterns.

---

## Conventions

- **C-P1 `longPress()` force-off dead-condition trap (BUG-01, CRITICAL/P0).** The 2nd branch of `longPress()` MUST use `isPowerOn()`/`!isPowerOff()`, never a repeat of `isPowerOff()`. Duplicate condition makes `POWERING_OFF` unreachable → non-OFF states fall to `else` → `set_power_status(UNDEFINED)`; HA sees UNDEFINED on force-off while relay still fires. Verify the two branch conditions are mutually exclusive before editing. (C2; `RSwitch.cpp:308`)

- **C-P2 `setSwitch(false)` double-pulse guard — `isPowerOn()` only (BUG-03, C3).** OFF guard must be `isPowerOn()` alone, NOT `isPowerOn() || isPoweringOff()`; the latter lets a 2nd `set=0` call `short_push()` while the relay is already cycling, possibly toggling the device back on. Mirror the ON-direction guard (excludes POWERING_ON). (`RSwitch.cpp:347`)

- **C-P3 Stamp `on_time`/`off_time` in `set_power_status()` (BUG-02).** Set `on_time=millis()` on transition TO `POWER_ON` and `off_time=millis()` on transition TO `POWER_OFF`. Both init 0 and are otherwise never set; unset → `getOnDurationSec()`/`getOffDurationSec()` return seconds-since-boot, corrupting `info` JSON telemetry. (C2; `RSwitch.cpp:45-55`, `RSwitch.h:50-51`)

- **C-P4 Suppress periodic 5-min validation during SLEEP (BUG-07).** The periodic `use_stat=true` branch in `detect_power_status_old()` must not fire when `power_status==SLEEP`; mid-cycle GPIO read can spuriously set `POWER_ON`/`POWER_OFF`. Add `&& power_status != SLEEP`. (`RSwitch.cpp:269`)

- **C-P5 Always edit `detect_power_status_old()`; `_new()` is dormant+divergent (BUG-10).** `_new()` is dead code (unreferenced; corroborated by `/arch`) and uses a different debounce model (`state_change_time` vs `verify_time_prev`). Promotion requires a deliberate reviewed switch with behavioral comparison. (`RSwitch.cpp:133-221`)

## Decisions

- **D-1 Active detect impl = `detect_power_status_old()` (confirmed 2026-06-06).** `detect_power_status()` delegates to `_old()`. Must not be silently swapped; promotion routes through `/review`.

- **D-2 `longPress()` sequencing intent: relay before callback (clarified 2026-06-06).** Intent is `long_push()` then `set_power_status()` so callback fires after relay completes. Current code inverts this (BUG-04); benign while `long_push()` blocks 10 s, but MUST be corrected when relay pulses go non-blocking (backlog).

## Open Questions

- **OQ-1 Wake-from-SLEEP back-date duration (BUG-06).** Back-date is 3 s (`LONG_PULSE_DURATION_DETECT_SEC`) but confirm window is 6 s (`VALIDATE_LONG_PULSE_SEC`). 6 s (conservative) or 3 s? Depends on real sleep-pulsing cadence; validate on hardware. (`RSwitch.cpp:336`)

- **OQ-2 Duration-getter sentinel during transitional states (BUG-11).** Getters return 0 in `POWERING_ON`/`POWERING_OFF`/`SLEEP`. Is 0 the intended "n/a" sentinel or should it be distinct (e.g. `ULONG_MAX`)? Clarify C5 with `/web`; moot until C-P3 lands.
