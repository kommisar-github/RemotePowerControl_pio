## Section 5 — `SKILLS.md` roster table

Target: `.claude/SKILLS.md` (paste into the template's roster table; keep the template's surrounding Overview / How-to-Use / Adding-New-Agents sections from `PM_TEMPLATES.md → SKILLS.md Template`).

| Skill | CLI | Cursor Globs | Domain | Model |
|-------|-----|--------------|--------|-------|
| Project Manager | `/pm` | `doc/**`, `.claude/**` | Planning, delegation, wave dispatch, doc ownership, reconciliation | (1M Opus — no model) |
| Architect | `/arch` | `doc/design/**` | Architecture, component boundaries, init sequencing, dual-MCU convention | (1M Opus — no model) |
| Architecture Review | `/review` | `src/**`, `doc/design/**` | Code review, RAM/heap audit, invariant enforcement, ESP8266/ESP32 parity | (1M Opus — no model) |
| Source Control | `/scm` | *(manual invoke)* | Git, commits, branches, tags, PRs | claude-haiku-4-5 |
| Power Control | `/power` | `src/RSwitch.cpp`, `src/RSwitch.h` | RSwitch state machine, relay pulses, optocoupler power detection, GPIO | claude-sonnet-4-6 |
| MQTT / Home Assistant | `/mqtt` | `src/MqttClient.cpp`, `src/MqttClient.h` | MQTT topics, PubSubClient, retained/LWT, JSON telemetry, HA integration | claude-sonnet-4-6 |
| Connectivity | `/net` | `src/WiFiConnect.*`, `src/OTA.*`, `src/MdnsClient.*`, `src/Test.*` | WiFi, OTA (espota), mDNS, reconnect/PHY, connection-health monitor | claude-sonnet-4-6 |
| Web UI | `/web` | `src/WebSServer.*`, `src/strings1.h` | Embedded HTTP server, HTML/CSS/JS page, `/getstatus` JSON endpoint | claude-sonnet-4-6 |
| Firmware / Build | `/firmware` | `platformio.ini`, `src/Application.h`, `src/main.cpp`, `src/SerialDebug.*`, `src/Log.*` | PlatformIO env matrix, build flags/secrets, pin maps, init wiring, serial debug | claude-sonnet-4-6 |
