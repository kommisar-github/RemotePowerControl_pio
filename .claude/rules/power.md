---
description: Power control / RSwitch — relay pulses + optocoupler power detection. Edit RSwitch only; fire callbacks, never publish.
globs: src/RSwitch.cpp, src/RSwitch.h
alwaysApply: false
---

# /power — RSwitch rules (condensed)

Full domain knowledge: `.claude/skills/power/SKILL.md`. This is the quick guardrail.

## Owns
- `src/RSwitch.cpp`, `src/RSwitch.h`; design docs `POWER_STATE.md`, `HARDWARE.md`.

## NEVER touches
- `src/MqttClient.*` / topic schema (`/mqtt`) — power only *fires the callback* (C2), never publishes.
- `src/WiFiConnect.*` / `OTA.*` / `Test.*` (`/net`); `src/WebSServer.*` (`/web`); `Application.h` pin **numbers** (`/firmware` owns the map — you own electrical meaning).

## Key pitfalls
- **Detect polarity (the #1 trap):** `INPUT_PULLUP`; optocoupler ⇒ **LOW = power ON, HIGH = OFF**. Inverting flips every reported state.
- `POWER_STATUS` enum is **wire-significant** (`UNDEFINED=-1, POWER_OFF=0, POWER_ON=1, SLEEP=2, POWERING_OFF=10, POWERING_ON=11`) — never renumber (C1/C2 + HA + `MQTT_API.md`).
- **SLEEP** = >5 short detect-pulses in the precision window; `long_pulse` reset alongside.
- Timing constants: `PULSE_PRECESION_MILLS=20`, `LONG_PULSE_DURATION_DETECT_SEC=3`, `VALIDATE_LONG_PULSE_SEC=6`, `VALIDATE_SWITCH_STATUS_PERIOD_MINS=5`.
- Relay is momentary & **blocks loop**: `short_push`=800 ms, `long_push`=10 s (force-off, status=3) — the only grandfathered non-blocking-loop exception.
- Wake-from-SLEEP must spoof `stat_prev=HIGH` + back-date `verify_time_prev` or the sleep-wake bug returns.
- `setSwitch(true)` no-ops if already ON; guard logic prevents double-pulsing — don't "simplify" (behavioral contract C3).
- Edit `detect_power_status_old()` (active), not `_new()` (dormant), unless deliberately switching.
- Index: MQTT 1-based (`payload[2]-'1'`), web 0-based — keep distinct.

## Invariants
- Boot-safe-GPIO: switch pins high-impedance through reset (reference `/firmware`'s pin map). Non-blocking-loop: no new blocking (relay pulses grandfathered, canonical owner `/review`).
