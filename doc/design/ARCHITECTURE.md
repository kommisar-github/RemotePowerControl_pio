# Architecture — RemotePowerControl

## Abstract

**TL;DR:** Module map and runtime contract for RemotePowerControl firmware — how RSwitch, MqttClient, WiFiConnect/OTA/mDNS/Test, WebSServer, and the main.cpp shell compose under a single cooperative loop() on ESP8266 and ESP32.

**Load when:** architecture, module map, component boundaries, setup init order, loop, handleClient, cooperative scheduling, non-blocking, dual-MCU, ESP8266, ESP32, ifdef, callback wiring, integration, data flow, who calls what, system design

**Key facts:**
- Every module exposes `handleClient()` polled once per `loop()`; no long blocking allowed (non-blocking-loop invariant) — relay pulses are the grandfathered exception.
- `setup()` init order is load-bearing: test → Serial → time/SNTP → WiFi → MQTT(+topics+callback) → RSwitch[3] → OTA → web → serial-debug. Re-ordering breaks callback wiring.
- MQTT is only serviced once `wifiConnect.connectionEstablished()` is true.
- Dual-MCU divergence lives behind `#if defined(ESP8266)/ESP32`, never a forked source file.
- Cross-module state flows by C function-pointer callbacks (RSwitch→main→MQTT), not direct calls.

**Owner:** `/arch` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `POWER_STATE.md`, `MQTT_API.md`, `BUILD.md`, `HARDWARE.md`

---

## Module map

> Body authored in Phase 0 wave 0.2 (`/arch`). This stub carries the Abstract so the matrix references resolve at bootstrap.

## setup() init order

## Cooperative loop() contract

## Dual-MCU convention
