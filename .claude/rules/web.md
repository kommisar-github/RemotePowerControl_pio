---
description: Web UI — embedded HTTP server + PROGMEM buttons page. Consume the public RSwitch API only (C5).
globs: src/WebSServer.cpp, src/WebSServer.h, src/strings1.h
alwaysApply: false
---

# /web — Web UI rules (condensed)

Full domain knowledge: `.claude/skills/web/SKILL.md`. This is the quick guardrail.

## Owns
- `src/WebSServer.*`, `src/strings1.h` — HTTP server + route handlers + PROGMEM HTML/CSS/JS page.

## NEVER touches
- `src/RSwitch.*` internals (`/power`) — call the public API only (C5: `setSwitch/longPress/getPowerStatus/decodePowerStat`). `src/MqttClient.*` (`/mqtt`). Connectivity files (`/net`).

## Key pitfalls
- **Server type diverges:** `ESP8266WebServer` vs `WebServer`, both port 80 behind `#ifdef` — keep branches in sync (parity audited by `/review`).
- **Web index is 0-based** (`switch=0..2`), unlike MQTT's 1-based payload (C1) — don't unify carelessly.
- Routes: `GET /`→`handleRootFrame`, `GET /setswitch?switch=&status=`→`handleSetSwitch`, `GET /getstatus`→`handleGetStatus`.
- `/getstatus` JSON: `DynamicJsonDocument(256)`, `{"devs":[{"id","stat","on"}]}`; `stat=decodePowerStat()`, `on=(POWER_ON||POWERING_ON)`. 256-byte buffer bounds field count — watch heap.
- PROGMEM `PAGE_BUTTONS` in `strings1.h`; substitution via `String::replace()`; JS polls `/getstatus` every 2 s.
- **Offline/Unavailable backlog:** new `decodePowerStat` return values or RSwitch methods are a **C5 change owned by `/power`** — request via PM, don't reach into RSwitch.
