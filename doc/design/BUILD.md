# Build — PlatformIO envs, flags, secrets, deploy

## Abstract

**TL;DR:** Build, flash, and configuration reference for RemotePowerControl — the PlatformIO env matrix (ESP8266/ESP32, OTA/serial), sysenv-injected secrets and identity flags, library pins, and the OTA/serial deploy procedure.

**Load when:** build, PlatformIO, platformio.ini, env, build flags, sysenv, secrets, WIFI_SSID, MQTT_BROKER, OTA password, _HOST_NAME_, _AREA_, espota, upload protocol, flash, serial upload, COM port, ArduinoJson, PubSubClient, lib_deps, board, d1_mini, wemos_d1_mini32, release build, test build, version bump

**Key facts:**
- Envs: RELEASE_OTA_esp8266 (default, espota), RELEASE_COM_esp8266, TESTING_COM_esp8266, RELEASE_COM_esp32dv.
- Secrets are build-time only via sysenv: SMARTHOME_WIFI_SSID/PWD, SMARTHOME_MQTT_BROKER, SMARTHOME_OTA_PASSWD — never inlined (secrets invariant).
- Identity flags _HOST_NAME_ (rempowercntl1) and _AREA_ (home) drive MQTT topics + OTA hostname + mDNS.
- Lib deps pinned: ArduinoJson ~6.21.2 (6.x API), PubSubClient ~2.8 — a 7.x ArduinoJson bump is breaking.
- OTA upload via espota with --auth=${SMARTHOME_OTA_PASSWD}; serial fallback at 200000 baud on COM5/6.

**Owner:** `/firmware` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `HARDWARE.md`, `ARCHITECTURE.md`, `MQTT_API.md`

---

## Env matrix

> Body authored in Phase 0 wave 0.3 (`/firmware`) from the current `platformio.ini`. This stub carries the Abstract so the matrix references resolve at bootstrap.

## Secrets & identity flags (sysenv)

## Library pins

## OTA / serial deploy procedure

## Version-bump protocol
