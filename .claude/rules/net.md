---
description: Connectivity — WiFi / OTA / mDNS / Test health monitor. Keep both ESP8266/ESP32 #ifdef branches in sync.
globs: src/WiFiConnect.cpp, src/WiFiConnect.h, src/OTA.cpp, src/OTA.h, src/MdnsClient.cpp, src/MdnsClient.h, src/Test.cpp, src/Test.h
alwaysApply: false
---

# /net — Connectivity rules (condensed)

Full domain knowledge: `.claude/skills/net/SKILL.md`. This is the quick guardrail.

## Owns
- `src/WiFiConnect.*`, `src/OTA.*`, `src/MdnsClient.*`, `src/Test.*` — connection lifecycle, OTA wrapper, mDNS, health monitor.

## NEVER touches
- `src/MqttClient.*` (`/mqtt`) — you *signal* MQTT state via `Test` events (C4), don't publish. `src/RSwitch.*` (`/power`). `upload_protocol`/`upload_port` in `platformio.ini` (`/firmware` owns env config; you own runtime OTA behavior).

## Key pitfalls
- **Include divergence:** `ESP8266mDNS.h` vs `ESPmDNS.h`; `ESP8266WiFi.h` vs `WiFi.h`+`esp_wifi.h`. Both `#ifdef` branches must stay in sync (parity audited by `/review`).
- **OTA is mDNS-backed:** `otaClient.begin(host,pass)`; `espota` finds `<hostname>.local`. OTA hostname ≠ WiFi hostname; both must match `platformio.ini` `upload_port`. Auth failures (`OTA_AUTH_ERROR`) = wrong `_OTA_PASSWD_` (`SMARTHOME_OTA_PASSWD`, secrets invariant — `/firmware`).
- **`Test` events (C4):** `EVENT_WIFI/MQTT_RESTORED|LOST` (1..4); `main.cpp` reacts to `EVENT_MQTT_RESTORED` (resend conn info + keepalive + refresh all switches). Removing/reordering silently regresses HA recovery.
- Reconnect: `RECONNECT_INTERVAL=60000`; PHY escalation `TRY_PHY_N_INTERVAL=3600000`; `set_phy_11b/g/n` (+ `set_phy_lr` ESP32).
- `connectionEstablished()` gate: `loop()` services MQTT only once WiFi reports established.
- Keepalive: `<area>/keepalive/<host>`="connected" every 60 s while WiFi up (separate from MQTT keepalive).
- `WiFiConnect::begin()` is heavily overloaded; current `setup()` uses `begin(SSID,PWD,HOST)` (blocking). Pick deliberately.
- Respect non-blocking-loop invariant in `handleClient()` paths.
