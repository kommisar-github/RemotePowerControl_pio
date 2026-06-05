---
description: MQTT / Home Assistant — PubSubClient wrapper + topic/payload contract. Retained discipline is load-bearing.
globs: src/MqttClient.cpp, src/MqttClient.h
alwaysApply: false
---

# /mqtt — MQTT rules (condensed)

Full domain knowledge: `.claude/skills/mqtt/SKILL.md`. This is the quick guardrail.

## Owns
- `src/MqttClient.cpp`, `src/MqttClient.h`; the `main.cpp` **MQTT section** (topic `#define`s, `topics[]`, `mqtt_received_callback`, `send_info` fields — inside the C6 banner); `MQTT_API.md`.

## NEVER touches
- `src/RSwitch.*` internals (`/power`) — only call `setSwitch/longPress/getPowerStatus/decodePowerStat` (C3/C5).
- `src/WiFiConnect.*` / `Test.*` (`/net`) — you *consume* `Test` events (C4). `platformio.ini` (`/firmware`). The `/firmware` shell & `/power` callbacks in `main.cpp` (C6).

## Key pitfalls
- **Retained discipline:** `state/` + `status/` publish `retained=true`; `get`/`set`/`keepalive`/`announcement` non-retained. A stray retained `set` re-fires the relay on every reconnect.
- **Set-payload byte layout (C1):** `payload[0]` action (`'0'` off / `'1'` on / `'3'` force), `payload[2]` 1-based switch → index `payload[2]-'1'`. Position-sensitive — a reformat silently misroutes.
- **Resubscribe on reconnect is mandatory** (`resubscribe()` all `topics[]` + legacy `"inTopic"`) — else connected but deaf.
- **LWT:** `announcementTopic == lastWillTopic == <area>/announcement/<host>`; connect `"connected"`, will `"disconnected"` (WILLQOS=0).
- Buffer `MQTT_MAX_PACKET_SIZE=1024`; `info` JSON (`DynamicJsonDocument(1024)`) nears the cap — oversized `publish()` silently returns false.
- Reconnect is WiFi-gated + non-blocking (bails if `WiFi.status()!=WL_CONNECTED`, retries from `handleClient()`).
- Topic/payload-position change = C1 (touch `MQTT_API.md` + HA + web JS). New `POWER_STATUS` mapping = C2 (coordinate `/power`). Resend-all-state on reconnect = C4 (`EVENT_MQTT_RESTORED` → `refresh_all_switches_state()`).
- v1 is plain TCP `:1883`, no TLS/auth (trusted LAN).
