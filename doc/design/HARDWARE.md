# Hardware — wiring, pin maps, boot-safety

## Abstract

**TL;DR:** Hardware reference for RemotePowerControl — relay outputs, optocoupler power-detect inputs, button, status LED, and the per-MCU GPIO pin maps with boot-safety constraints for ESP8266 (d1_mini) and ESP32 (wemos_d1_mini32).

**Load when:** hardware, wiring, GPIO, pin map, relay, optocoupler, PLED, power detect, button, status LED, boot-safe pin, high impedance, D1, D2, ESP8266 pins, ESP32 pins, d1_mini, wemos_d1_mini32, INPUT_PULLUP, channel pinout

**Key facts:**
- Relay/switch output pins MUST be high-impedance during reset/boot or the relay fires on flash — ESP8266 uses D1/GPIO5 & D2/GPIO4 for this reason.
- ESP8266: SWITCH1=D1,SWITCH2=D8,SWITCH3=D2; DETECT1=D7,DETECT2=D6,DETECT3=D5; BUTTON=D0; LED=D4.
- ESP32: SWITCH1=22,SWITCH2=5,SWITCH3=21; DETECT1=23,DETECT2=19,DETECT3=18; BUTTON=26; LED=16.
- Detect input wired through an optocoupler off the controlled device's power LED; pull-up means active-LOW.
- Three channels, fixed; reference-only doc — updated only on hardware/pin change, never during feature work.

**Owner:** `/power` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `POWER_STATE.md`, `BUILD.md`, `ARCHITECTURE.md`

---

## Channel wiring (relay + optocoupler)

> Body authored in Phase 0 wave 0.3 (`/firmware` + `/power` on the pin table). This stub carries the Abstract so the matrix references resolve at bootstrap.

## Per-MCU pin map

## Boot-safety constraints
