# Power State — RSwitch state machine & pulse detection

## Abstract

**TL;DR:** The RSwitch power state machine and optocoupler pulse-detection algorithm — how the firmware infers a controlled device's real on/off/sleep state from a detect GPIO and drives a momentary relay to change it.

**Load when:** RSwitch, power state, state machine, POWER_ON, POWER_OFF, SLEEP, POWERING_ON, POWERING_OFF, detect gpio, optocoupler, pulse detection, short pulse, long pulse, sleep detection, relay, short_push, long_push, longPress, setSwitch, pulse timing, validation period, power LED, debounce

**Key facts:**
- Detect input is INPUT_PULLUP: LOW = power ON, HIGH = power OFF (inverting this flips every reported state).
- >5 short detect-pulses ⇒ SLEEP; long-pulse (>3 s edge, confirmed >6 s) ⇒ re-validate; 5-min periodic re-read.
- Relay is momentary: short_push = 800 ms HIGH pulse, long_push = 10 s (force-off, status=3) — both currently block loop().
- POWER_STATUS enum values are wire-significant (published in info JSON); never renumber.
- Wake-from-SLEEP spoofs stat_prev=HIGH + back-dates verify_time_prev to avoid mis-reading residual sleep pulsing.

**Owner:** `/power` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `HARDWARE.md`, `MQTT_API.md`, `ARCHITECTURE.md`

---

## State machine

> Body authored in Phase 0 wave 0.4 (`/power`) from the current `src/RSwitch.*` code. This stub carries the Abstract so the matrix references resolve at bootstrap.

## Pulse-detection algorithm

## Timing constants

## Relay actuation (short_push / long_push)
