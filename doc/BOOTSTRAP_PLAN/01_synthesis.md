## Section 1 — Project synthesis

**What we are building.** RemotePowerControl is ESP8266/ESP32 firmware (PlatformIO, Arduino framework) that remotely **switches and monitors up to 3 power channels** — the canonical use case is driving a PC's front-panel power button via a relay while sensing the machine's real on/off state through an optocoupler wired to a power LED. It exposes the channels over **MQTT** (Home Assistant integration), a small **web UI**, and **mDNS**, and updates itself over the air via **ArduinoOTA**.

- **Core capability — `RSwitch`.** Each channel is one `RSwitch` instance owning a *switch* GPIO (momentary relay pulse) and a *detect* GPIO (optocoupler input). It runs a software state machine over `POWER_OFF / POWER_ON / SLEEP / POWERING_ON / POWERING_OFF / UNDEFINED`, inferring true power state from detect-GPIO pulse patterns (steady level = on/off; rapid flashing = sleep/standby).
- **MQTT control + telemetry.** Subscribes to `<area>/<host>/power/set|get`, `<area>/alive/get`, `<area>/<host>/info/get`. Publishes **retained** per-channel `power/state/<idx>` (`0|1`) and `power/status/<idx>` (human string), plus a JSON `info` document (heap, RSSI, uptime, boot time, connection-loss counters). Uses MQTT **Last Will & Testament** so Home Assistant marks the device offline on ungraceful disconnect.
- **Web UI.** `ESP8266WebServer` / `WebServer` serving a single PROGMEM HTML page (`strings1.h`) with per-channel toggle + "force" buttons and a `/getstatus` JSON poll; `/setswitch?switch=&status=` drives the relay.
- **Connectivity + resilience.** `WiFiConnect` (reconnect, PHY-mode switching), `OTAClient` (ArduinoOTA over `espota`), `MdnsClient`, and a `Test` health monitor that emits WiFi/MQTT lost/restored events and a status LED.
- **Operability.** `SerialDebug` registers serial commands (`switch_info`, `wifi_info`, `mqtt_info`, `wifi_set_11b/g/n`, `restart`, …) and `Log` provides overloaded logging helpers.

**v1 boundary (what "done" means for the current line).** Reliable 3-channel switch + detect; MQTT state/status/LWT/keepalive correct across reconnects; web UI reflecting live state; OTA flashing on both ESP8266 (`d1_mini`) and ESP32 (`wemos_d1_mini32`); the open `README.txt` items resolved (resend all switch state on MQTT reconnect; web "unavailable/grey" when the device is offline; WiFi setup-portal button; persistent setup). The **Telephone/IVR interface** is explicitly post-v1.

**Hard constraints.**
- **RAM is the binding budget.** ESP8266 has ~40–50 KB usable heap; firmware already tracks `getHeapFragmentation()` / `getMaxFreeBlockSize()`. JSON buffers are fixed (`DynamicJsonDocument(1024)` for `info`, `256` for web status). MQTT packet capped at `MQTT_MAX_PACKET_SIZE = 1024`.
- **Boot-safe relay GPIOs (load-bearing).** On ESP8266 the switch GPIOs must be pins that stay high-impedance through reset/boot (the design uses D1/GPIO5 & D2/GPIO4); a wrong pin fires the relay on every reset/flash. Pin maps differ per MCU (`Application.h`).
- **Single-threaded cooperative loop.** Everything is polled from `loop()` via `handleClient()`. There is no RTOS task isolation; a long blocking call starves WiFi/MQTT/OTA servicing.
- **Secrets are build-time only.** WiFi SSID/PWD, MQTT broker, OTA password are injected through PlatformIO `sysenv` build flags (`SMARTHOME_*`) — never hard-coded, never committed.
- **Dual-MCU from one source tree.** ESP8266 vs ESP32 divergence is handled entirely by `#if defined(ESP8266)/ESP32` — not by separate code trees or roadmaps.

**Explicit non-goals (v1).**
- No TLS/authenticated MQTT (plain `:1883`, trusted LAN).
- No persistent filesystem / EEPROM config yet (runtime config is compile-time; persistence is a *planned* phase, not present).
- No more than 3 channels; channel count is a hard-coded `3`.
- No OTA rollback / dual-bank A/B; ArduinoOTA single-image only.
- No multi-platform fork — this is one repo, one ROADMAP.

**Project invariants** (cross-specialist rules every agent must respect — see `PM.md` → *Project invariants*; owners carry full text, others reference):
- **Secrets invariant** — no credential (WiFi/MQTT/OTA) literal in source or git history; all injected via `sysenv` build flags. Owner: `/firmware` (canonical), audited by `/review`.
- **Boot-safe-GPIO invariant** — any GPIO assigned as a relay/switch output must be boot-safe on the target MCU. Owner: `/firmware` (pin map) + `/power` (electrical rationale), audited by `/review`.
- **Non-blocking-loop invariant** — no new unbounded blocking in the `loop()` path; the existing relay pulses (`short_push` 800 ms, `long_push` 10 s `delay()`) are the *known, grandfathered* exceptions and are themselves a backlog item. Owner: `/review` (audit), `/arch` (design).

**Prerequisite-question callouts (forward references to §12):**
- **§6 (matrix) depends on §12 Q-MEMORY** — if the user wants a seeded `MEMORY.md` at bootstrap, the matrix gets a populated `MEMORY.md` row from t0; if not, the row still exists but the file is a stub created on first phase close.
- **§7 / §10 depend on §12 Q-MARKERS** — whether to insert in-file `// === <owner> section ===` banners in `main.cpp` (the shared integration file) during bootstrap. Default per `BOOTSTRAP_PROMPT.md` §7 is *insert*; if declined, ownership lives only in the matrix + §7.
- **§8 depends on §12 Q-HA-DISCOVERY** — whether Phase 1 includes Home Assistant MQTT auto-discovery (adds a wave to `/mqtt`) or stays on manually-configured HA entities.
