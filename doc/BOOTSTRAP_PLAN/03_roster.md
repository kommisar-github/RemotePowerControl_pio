## Section 3 — Agent roster

Nine agents: four defaults (pm / arch / review / scm) + five domain specialists (power / mqtt / net / web / firmware). Every block below is the full shape required by the bootstrap prompt.

**Minimum vs nice-to-have (v1 load-bearing).**

| Agent | Load-bearing for v1? | If dropped |
|---|---|---|
| pm, arch, review, scm | Yes (defaults) | — |
| **power** | **Yes** — owns the core RSwitch state machine; the whole product is power switching/detection. | No product. |
| **mqtt** | **Yes** — the primary control + HA telemetry surface. | No Home Assistant integration. |
| **net** | **Yes** — without WiFi/OTA/reconnect the device is unreachable and un-updatable. | Device is a dead LAN node. |
| **web** | Nice-to-have | Fold into `/mqtt` short-term; web UI is a secondary control surface (~2 files). Accept context bloat in `/mqtt`. |
| **firmware** | **Yes** — owns the build matrix, secrets, pin maps, and `main.cpp` wiring; nothing flashes without it. | No reproducible build / boot-safe pinning. |

**Recommendation: bootstrap all nine.** `web` is the only honest "earn-later" candidate, but its SKILL.md stub costs ~1 entry and the ESP8266-vs-ESP32 `WebServer` API divergence is real knowledge worth isolating before the "unavailable/grey UI" backlog item lands. Bootstrap it.

---

#### pm — Project manager / dispatcher

**Role:** coordinator
**Model:** (coordinator — omit from agents.json for 1M Opus)
**Capabilities:** planning, delegation, wave-dispatch, doc-ownership, reconciliation

**Owns (files):**
- `doc/plans/ROADMAP.md` — phase list + live status (from `README.txt` TODOs)
- `doc/NEXT_STEPS.md` — current action items
- `doc/MEMORY.md` — cross-phase decisions, hardware facts, lessons
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — the matrix
- `doc/BOOTSTRAP_PLAN.md` (+ `doc/BOOTSTRAP_PLAN/`) — this plan (until archived)

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** read all Primary docs (ROADMAP, NEXT_STEPS, MEMORY, matrix, this plan) at startup
- **Subagent fork:** read only matrix + docs cited in `Context docs:`

**Never touches:**
- `src/**` — owned by the domain specialists; PM dispatches, never edits firmware
- `platformio.ini` — `/firmware`
- `.claude/skills/<specialist>/SKILL.md` bodies — proposes changes, specialists own

**SKILL.md domain knowledge** (lightly-customized default):
- The 9-agent roster + the three project invariants (secrets, boot-safe-GPIO, non-blocking-loop) and their canonical owners.
- The §7 interface contracts — evaluate every cross-cutting change (new MQTT topic, new `POWER_STATUS`, new callback) against them before approving.
- `main.cpp` is a **shared integration file** — dispatch payloads touching it must cite the owning section (see §7 contract C6) and, if markers are enabled, the banner range.
- Wave maps are **mostly serial** (solo operator, single shared-scope `loop()` runtime) — do not invent parallel waves.

**Cursor rule globs:** `doc/**`, `.claude/**`

---

#### arch — Architect

**Role:** coordinator
**Model:** (coordinator — omit from agents.json for 1M Opus)
**Capabilities:** architecture, component-boundaries, phase-design, init-sequencing

**Owns (files):**
- `doc/design/ARCHITECTURE.md` — module map, `setup()` init order, the cooperative-loop contract, dual-MCU convention
- `doc/design/PHASE_<N>_DESIGN.md` — per-phase design output (created at phase start)

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `ARCHITECTURE.md` + all design docs (`POWER_STATE.md`, `MQTT_API.md`, `HARDWARE.md`, `BUILD.md`)
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- `src/**` implementation — designs boundaries, specialists implement
- `doc/plans/ROADMAP.md` status — `/pm`
- Per-specialist SKILL.md bodies — proposes, specialists own

**SKILL.md domain knowledge** (lightly-customized default):
- The module decomposition: `RSwitch` (power) ↔ `MqttClient` (mqtt) ↔ `WiFiConnect`/`OTAClient`/`MdnsClient`/`Test` (net) ↔ `WebSServer` (web) ↔ `main.cpp`/`Application.h`/`SerialDebug`/`Log` (firmware).
- The **init-order contract** in `setup()`: `test.begin()` → `Serial` → time/SNTP → `wifiConnect.begin()` → `mqttClient.begin()`+topics+callback → `RSwitch` array → `otaClient.begin()` → `webSServer.begin()` → serial-debug handlers. Re-ordering breaks callback wiring.
- The **cooperative-loop contract**: every module exposes `handleClient()` polled once per `loop()`; nothing may block long enough to starve OTA/MQTT servicing (non-blocking-loop invariant).
- Dual-MCU rule: new platform divergence goes behind `#if defined(ESP8266)/ESP32`, never a forked file.

**Cursor rule globs:** `doc/design/**`

---

#### review — Architecture & code review

**Role:** coordinator
**Model:** (coordinator — omit from agents.json for 1M Opus)
**Capabilities:** code-review, auditing, risk-assessment, invariant-enforcement

**Owns (files):**
- (no source files) — audits diffs across the whole tree; owns the review checklist inside its own SKILL.md

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** the doc under review + `ARCHITECTURE.md`
- **Subagent fork:** matrix + the changed files + cited `Context docs:`

**Never touches:**
- Any `src/**` file as an author — review proposes, the owning specialist edits
- `agents.json` / roster — `/pm`

**SKILL.md domain knowledge** (project-specific risk checklist — invariant owner for non-blocking-loop):
- **Heap/RAM audit:** flag new `String` churn in hot paths, unbounded buffers, and JSON docs sized past `MQTT_MAX_PACKET_SIZE`/web `BUFFER_SIZE`; check `getHeapFragmentation` impact on ESP8266.
- **Non-blocking-loop invariant (canonical owner):** reject new `delay()`/busy-wait in the `loop()` path; the grandfathered relay `short_push`/`long_push` blocks are the only allowed exceptions and are themselves backlog.
- **Boot-safe-GPIO audit:** any new relay/switch output pin must be verified boot-safe on the target MCU before merge.
- **Secrets audit:** no credential literal in source; confirm new config flows through `sysenv` build flags.
- **Retained-topic correctness:** state/status topics publish `retained=true`; transient `get/set` must not be retained — a wrong flag leaves stale state in the broker.
- **ESP8266/ESP32 parity:** every change compiles and behaves on both targets; check `#ifdef` branches are both updated.

**Cursor rule globs:** `src/**`, `doc/design/**`

---

#### scm — Source control

**Role:** specialist
**Model:** claude-haiku-4-5
**Capabilities:** git, commits, branches, tags, PRs

**Owns (files):**
- `.gitignore` — repo ignore rules

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** none beyond the matrix; works from the dispatch payload describing the change
- **Subagent fork:** matrix + dispatch payload

**Never touches:**
- Any `src/**` content as an author — commits what specialists produce
- `platformio.ini`, secrets — never stages a file containing a credential literal (secrets invariant)

**SKILL.md domain knowledge:** *(none — git is git.)* Single project note: this is a PlatformIO repo (`.pio/` is git-ignored); the release **version-bump protocol** (`APP_VER` + per-module header comment dates) is **owned by `/firmware`** — `/scm` references `/firmware` SKILL.md → "Version-bump protocol", does not restate it.

**Cursor rule globs:** *(none — manual invoke)*

---

#### power — Relay control + power-state detection (RSwitch)

**Role:** specialist
**Model:** claude-sonnet-4-6
**Capabilities:** relay-control, power-detection, state-machine, gpio, optocoupler, pulse-analysis

**Owns (files):**
- `src/RSwitch.cpp`, `src/RSwitch.h` — the channel state machine + detect/switch GPIO logic
- `doc/design/POWER_STATE.md` — state-machine + pulse-detection design (Primary)
- `doc/design/HARDWARE.md` — relay/optocoupler wiring + per-MCU pin behavior (Primary; shared with `/firmware` on the pin-number table)

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `POWER_STATE.md` + `HARDWARE.md` + the §7 contracts (C2, C3, C5)
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- `src/MqttClient.*`, topic schema — `/mqtt`; power only *fires the callback*, it does not publish
- `src/WiFiConnect.*`, `src/OTA.*`, `src/Test.*` — `/net`
- `src/WebSServer.*` — `/web` (web *calls* RSwitch; the API surface is power's contract C5)
- `Application.h` pin **numbers** — `/firmware` owns the map; power owns the *electrical meaning* of each pin

**SKILL.md domain knowledge** (platform-quirk-dense — 12 bullets):
- `POWER_STATUS` enum values are **wire-significant**: `UNDEFINED=-1, POWER_OFF=0, POWER_ON=1, SLEEP=2, POWERING_OFF=10, POWERING_ON=11` — published verbatim in the `info` JSON; never renumber without updating HA + `MQTT_API.md`.
- **Detect-GPIO polarity (the #1 trap):** input is `INPUT_PULLUP`; the optocoupler pulls it **LOW when power is ON** (PLED+ high), **HIGH when OFF**. Inverting this flips every reported state.
- **Sleep detection:** >5 short detect-pulses (3 LED flashes ≈ 6 edges) within the precision window ⇒ `SLEEP`. `short_pulse > 5` is the threshold; `long_pulse` is always set once before pulsing starts, so it's reset alongside.
- **Pulse timing constants:** `PULSE_PRECESION_MILLS=20` (debounce), `LONG_PULSE_DURATION_DETECT_SEC=3` (short-vs-long edge), `VALIDATE_LONG_PULSE_SEC=6` (long-pulse confirm), `VALIDATE_SWITCH_STATUS_PERIOD_MINS=5` (periodic re-read). These are tuned to a specific PC's power-LED behavior.
- **Relay is momentary, not latching:** `short_push()` = `digitalWrite(HIGH); delay(short_delay=800); LOW`. `long_push()` = `delay(long_delay=10000)` (forced power-off / `status=3`). Both **block the loop** — known non-blocking-loop-invariant exception, slated for refactor to non-blocking.
- **Wake-from-SLEEP requires state reset:** when `setSwitch(ON)` from `SLEEP`, it spoofs `stat_prev=HIGH` and back-dates `verify_time_prev` so the validator doesn't mis-read the residual sleep pulsing as a state change. Skipping this re-introduces the sleep-wake bug.
- **`setSwitch(true)` is a no-op if already ON/POWERING_ON;** `setSwitch(false)` only acts from `POWER_ON`/`POWERING_OFF`. Guard logic prevents double-pulsing — don't "simplify" it away.
- **Two detect implementations exist:** `detect_power_status_old()` (active, called via `detect_power_status()`) and `detect_power_status_new()` (state-machine variant, dormant). Edit the *old* one unless deliberately switching; they diverge on how `verify_time_prev`/`state_change_time` are tracked.
- **State changes flow out via callback only** (contract C2): `set_power_status()` fires `power_status_change_callback(this, new, old)` *only on actual change* — `main.cpp` maps it to MQTT publishes. RSwitch never touches MQTT directly.
- **Three channels, fixed:** `rSwitch[0..2]`; switch index in payloads is `'1'..'3'` (1-based on the wire, 0-based in the array — `payload[2]-'1'`).
- **Boot-safe outputs:** switch GPIOs must be high-impedance during reset (ESP8266 design: D1/GPIO5, D2/GPIO4) so the relay doesn't pulse on flash/reset — coordinate pin choice with `/firmware`.
- **Known pitfall (open `README.txt`):** when the PC is ON and the detect pin is physically disconnected, status sticks ON on one channel but not another — a floating-input edge case in the validation timer; reproduce before "fixing."

**Cursor rule globs:** `src/RSwitch.cpp`, `src/RSwitch.h`

---

#### mqtt — MQTT protocol + Home Assistant integration

**Role:** specialist
**Model:** claude-sonnet-4-6
**Capabilities:** mqtt, pubsubclient, home-assistant, topic-schema, retained, lwt, json-telemetry

**Owns (files):**
- `src/MqttClient.cpp`, `src/MqttClient.h` — the PubSubClient wrapper (connect/LWT/publish/subscribe)
- `main.cpp` **MQTT section** — the topic `#define`s (`STR_POWER_*`, `STR_ALIVE*`, `STR_INFO*`), `topics[]`, `mqtt_received_callback`, `send_info()` (shared file; see §7 C6)
- `doc/design/MQTT_API.md` — topic grammar + payload contract + HA mapping (Primary)

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `MQTT_API.md` + contracts C1, C3, C4
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- `src/RSwitch.*` internals — `/power`; mqtt only calls `setSwitch/longPress/getPowerStatus/decodePowerStat`
- `src/WiFiConnect.*`/`Test.*` — `/net`; mqtt *consumes* `Test` reconnect events, doesn't own them
- `platformio.ini` / build flags — `/firmware`

**SKILL.md domain knowledge** (platform-quirk-dense — 11 bullets):
- **Topic grammar:** `STR_POWER_STATE = <area>/<host>/power/state/` + `<idx>` (retained, payload `0|1` — on/off only); `STR_POWER_STATUS = .../power/status/` + `<idx>` (retained, human string from `decodePowerStat`); `STR_POWER_SET = .../power/set` (not retained, payload `<0|1|3><idx>`); `STR_POWER_GET`, `STR_ALIVE(/get)`, `STR_INFO(/get)`, `STR_CONNECTION_INFO`, `STR_KEEPALIVE`. `<area>`/`<host>` come from `_AREA_`/`_HOST_NAME_` build flags.
- **Set-payload byte layout:** `payload[0]` = action (`'0'` off, `'1'` on, `'3'` long-press/force), `payload[2]` = 1-based switch char → `payload[2]-'1'` index. Position-sensitive parsing — a reformatted payload silently misroutes.
- **Retained discipline:** `state/` and `status/` publish with `retained=true` so HA restores last state on restart; `get`/`set`/`keepalive`/`announcement` are **non-retained**. A stray retained `set` would re-fire the relay on every broker reconnect.
- **LWT:** `lastWillTopic == announcementTopic == <area>/announcement/<host>`; connect message `"connected"`, will message `"disconnected"` (`WILLQOS=0`). HA marks the device offline via the will. Both share the same topic by design.
- **PubSubClient buffer:** `MQTT_MAX_PACKET_SIZE=1024` set via `setBufferSize`; total frame = `5 (header) + 2 + strlen(topic) + strlen(payload)`. The `info` JSON can approach this — oversized publishes silently fail (`publish()` returns false; code logs the overflow math).
- **Reconnect is WiFi-gated and non-blocking:** `reconnect()` bails if `WiFi.status() != WL_CONNECTED`; retries every `_reconnectInterval` (default 10 s) from `handleClient()`. On success it `publish_announcement()` + `resubscribe()` + fires the connect event.
- **Resubscribe-on-reconnect is mandatory:** subscriptions are lost on disconnect; `resubscribe()` re-subscribes all `topics[]` (plus a legacy `"inTopic"`). Forgetting it = device connected but deaf.
- **`info` JSON** (`DynamicJsonDocument(1024)`): `app/ver/host/powerStatus1..3/ts/now/boottime/boot/MAC/IP/RSSI/phymode/chan/BSSID/freeHeap/...` + (`connection_info`) loss/restore timestamps and counters. The `mc` field reports `ESP8266`/`ESP32`. Adding fields risks the 1024-byte cap.
- **State resend on reconnect (open backlog):** `README.txt` wants *all* switch states re-published on MQTT restore. The hook exists — `EVENT_MQTT_RESTORED` → `refresh_all_switches_state()` (contract C4) — verify it publishes all 3 retained.
- **`mqtt_received_callback` routing** lives in `main.cpp` (mqtt section): matches `STR_POWER_SET/GET`, `STR_ALIVE_GET`, `STR_INFO_GET`. This is the consumer end of contract C3 (→ `/power`).
- **Plain TCP `:1883`, no TLS/auth** in v1 (trusted LAN). Don't add auth without coordinating the broker side.

**Cursor rule globs:** `src/MqttClient.cpp`, `src/MqttClient.h`

---

#### net — WiFi, OTA, mDNS, connection health

**Role:** specialist
**Model:** claude-sonnet-4-6
**Capabilities:** wifi, ota, mdns, reconnect, phy-mode, connection-monitor, espota

**Owns (files):**
- `src/WiFiConnect.cpp`, `src/WiFiConnect.h` — connection lifecycle, reconnect, PHY switching, getters
- `src/OTA.cpp`, `src/OTA.h` — ArduinoOTA wrapper
- `src/MdnsClient.cpp`, `src/MdnsClient.h` — mDNS service discovery
- `src/Test.cpp`, `src/Test.h` — WiFi/MQTT health monitor + status LED + keepalive

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `ARCHITECTURE.md` (connectivity section) + contract C4
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- `src/MqttClient.*` — `/mqtt`; `net` *signals* MQTT state via `Test` events but doesn't publish
- `src/RSwitch.*` — `/power`
- `upload_protocol`/`upload_port` lines in `platformio.ini` — `/firmware` owns the env config; `net` owns the *runtime* OTA behavior

**SKILL.md domain knowledge** (platform-quirk-dense — 10 bullets):
- **mDNS include divergence:** `ESP8266mDNS.h` (ESP8266) vs `ESPmDNS.h` (ESP32); WiFi headers `ESP8266WiFi.h` vs `WiFi.h`+`esp_wifi.h`. Every connectivity file guards these with `#ifdef` — both branches must stay in sync.
- **ArduinoOTA is mDNS-backed:** `otaClient.begin(host, pass)` sets hostname+password and `ArduinoOTA.begin()`; the `espota` uploader finds the device as `<hostname>.local`. OTA hostname (`ArduinoOTA.getHostname()`) is separate from the WiFi hostname — both must match the `platformio.ini` `upload_port`.
- **OTA error codes:** `OTA_AUTH_ERROR` (wrong `--auth` / `_OTA_PASSWD_`), `OTA_BEGIN/CONNECT/RECEIVE/END_ERROR` — auth failures are the common one; the password is the `SMARTHOME_OTA_PASSWD` build flag.
- **Reconnect policy:** `RECONNECT_INTERVAL=60000`; PHY-mode retry escalation `TRY_PHY_N_INTERVAL=3600000` (60 min). `try_reconnect()` + `set_phy_11b/g/n` (+ `set_phy_lr` ESP32-only) let a flaky AP be coerced to a more robust PHY mode.
- **`Test` connection model (contract C4):** emits `EVENT_WIFI_RESTORED/LOST`, `EVENT_MQTT_RESTORED/LOST`; `main.cpp`'s `test_change_event_callback` reacts to `EVENT_MQTT_RESTORED` by re-sending connection info, keepalive, and refreshing all switch states.
- **Status LED:** `TEST_LED_PULLUP` vs `TEST_LED_PULLDOWN` sets active level; `test.begin(cb, TEST_LED_PULLUP)` in `setup()`. `TEST_WIFI_LED` pin differs per MCU (`Application.h`).
- **Keepalive:** `setKeepAliveEventCallback(cb, 60000)` publishes `<area>/keepalive/<host>` = `"connected"` every 60 s while WiFi up — separate from MQTT keepalive.
- **`connectionEstablished()` gate:** `loop()` only services MQTT once WiFi reports established (ensures hostname fully configured); don't call MQTT before this.
- **Bad-DHCP detection:** `Test` tracks `bad_dhcp_ip_assigned` and lost/restore timestamps surfaced in the `info` JSON — useful when an AP hands out a bogus IP.
- **`WiFiConnect::begin()` has many overloads** (blocking vs `begin_nb`, hostname vs generated, mDNS on/off, custom callback). Current `setup()` uses `begin(SSID, PWD, HOST)` (blocking, explicit hostname). Pick the overload deliberately.

**Cursor rule globs:** `src/WiFiConnect.cpp`, `src/WiFiConnect.h`, `src/OTA.cpp`, `src/OTA.h`, `src/MdnsClient.cpp`, `src/MdnsClient.h`, `src/Test.cpp`, `src/Test.h`

---

#### web — HTTP UI

**Role:** specialist
**Model:** claude-sonnet-4-6
**Capabilities:** http-server, web-ui, html, json-endpoint, esp8266webserver

**Owns (files):**
- `src/WebSServer.cpp`, `src/WebSServer.h` — the embedded HTTP server + route handlers
- `src/strings1.h` — PROGMEM HTML/CSS/JS page

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `ARCHITECTURE.md` (web-route section) + contract C5
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- `src/RSwitch.*` internals — `/power`; web calls the public RSwitch API only (contract C5)
- `src/MqttClient.*` — `/mqtt`
- Connectivity files — `/net`

**SKILL.md domain knowledge** (standard-stack, honest — 6 bullets):
- **Server type diverges by MCU:** `ESP8266WebServer` (`ESP8266WebServer.h`) vs `WebServer` (`WebServer.h`); both constructed on port 80 behind `#ifdef`. API is otherwise compatible.
- **Routes:** `GET /` → `handleRootFrame` (serves `PAGE_BUTTONS`), `GET /setswitch?switch=<0-2>&status=<0|1|3>` → `handleSetSwitch`, `GET /getstatus` → `handleGetStatus` (JSON). Registered as C++ lambdas capturing `this`.
- **Switch index is 0-based on the web wire** (`switchId[0]-'0'`, range 0–2) — *unlike* MQTT's 1-based payload. Don't unify them carelessly; the JS sends `switch=0..2`.
- **PROGMEM page:** `PAGE_BUTTONS` is a single `const char* PROGMEM` string in `strings1.h`; `String page; page += FPSTR(PAGE_BUTTONS); page.replace("device1","…")` — placeholder substitution via `replace()`. The JS polls `/getstatus` every 2 s and toggles checkboxes from `resp.devs[i].on`.
- **`/getstatus` JSON:** `DynamicJsonDocument(256)`, `{"devs":[{"id","stat","on"},…]}` where `stat = decodePowerStat(status)`, `on = (POWER_ON || POWERING_ON)`. The 256-byte buffer bounds field count.
- **Open backlog (`README.txt`):** when the device is off / channel disabled, the UI must render **Unavailable/grey** instead of a live toggle — a `strings1.h` + `handleGetStatus` change; `decodePowerStat` already returns `"DISABLED"` for null channels.

**Cursor rule globs:** `src/WebSServer.cpp`, `src/WebSServer.h`, `src/strings1.h`

---

#### firmware — Build, platform, pin maps, integration shell

**Role:** specialist
**Model:** claude-sonnet-4-6
**Capabilities:** platformio, build-flags, dual-mcu, gpio-map, secrets, serial-debug, ota-deploy, integration

**Owns (files):**
- `platformio.ini` — env matrix, build flags, OTA/serial upload config, lib deps
- `src/Application.h` — per-MCU GPIO pin map + boot-safe pin choices
- `src/main.cpp` **shell** — `setup()` init order, `loop()`, `send_info` plumbing, debug-command wiring (shared file; MQTT section is `/mqtt`'s, power callbacks are `/power`'s — §7 C6)
- `src/SerialDebug.cpp`, `src/SerialDebug.h` — serial command dispatcher
- `src/Log.cpp`, `src/Log.h` — logging helpers
- `doc/design/BUILD.md` — env matrix, secrets, flashing/OTA procedure (Primary)

**Context (tiered doc loading):**
- `doc/design/DOC_OWNERSHIP_MATRIX.md` — always read first
- **Dedicated terminal:** `BUILD.md` + `HARDWARE.md` (pin table, shared with `/power`) + `ARCHITECTURE.md` (init order)
- **Subagent fork:** matrix + docs cited in `Context docs:`

**Never touches:**
- Module internals — `RSwitch`/`MqttClient`/`WiFiConnect`/`WebSServer` bodies belong to their specialists; firmware owns the *wiring* in `main.cpp`, not the logic
- MQTT topic `#define`s in `main.cpp` — `/mqtt` (C6)
- The power-callback bodies in `main.cpp` — `/power` (C6)

**SKILL.md domain knowledge** (platform-quirk-dense — 11 bullets):
- **Env matrix:** `RELEASE_OTA_esp8266` (default, `espota` upload to `${hostname}`), `RELEASE_COM_esp8266` / `TESTING_COM_esp8266` (serial, COM5/COM6), `RELEASE_COM_esp32dv` (`espressif32@^6.4.0`, `wemos_d1_mini32`). Boards: `d1_mini` (8266) / `wemos_d1_mini32` (32).
- **Secrets via `sysenv` (secrets invariant — canonical owner):** `SMARTHOME_WIFI_SSID[_OFFICE]`, `SMARTHOME_WIFI_PWD[_OFFICE]`, `SMARTHOME_MQTT_BROKER[_OFFICE]`, `SMARTHOME_OTA_PASSWD` → `-D _WIFI_SSID_`, `_WIFI_PWD_`, `_MQTT_BROKER_`, `_OTA_PASSWD_`. `build_flags_home` vs `build_flags_office` toggle the network. Never inline a credential.
- **Identity flags:** `_HOST_NAME_` (`_HOST_NAME_=rempowercntl1`), `_AREA_` (`home`) — drive MQTT topics + OTA hostname + mDNS. `_SN_RELEASE_BUILD_` / `_SN_TEST_BUILD_` select build profile.
- **Pin maps (`Application.h`) are per-MCU and boot-safety-critical:** ESP8266 `POWER_SWITCH1=D1, SWITCH2=D8, SWITCH3=D2; DETECT1=D7, DETECT2=D6, DETECT3=D5; BUTTON=D0; LED=D4`. ESP32 `SWITCH1=22, SWITCH2=5, SWITCH3=21; DETECT1=23, DETECT2=19, DETECT3=18; BUTTON=26; LED=16`. Switch pins chosen high-impedance-on-boot (coordinate with `/power`).
- **`setup()` init order is load-bearing** (see `/arch`): `test.begin` first (LED + keepalive), then Serial, time/SNTP, WiFi, MQTT (+topics/callback), RSwitch array, OTA, web, serial-debug handlers. The order wires the callbacks — don't shuffle.
- **SNTP differs per MCU:** ESP8266 `settimeofday_cb` + `configTime(MYTZ, "pool.ntp.org")` (TZ string); ESP32 `sntp_set_time_sync_notification_cb` + `configTime(gmtOffset, daylightOffset, …)` (numeric offsets). `boottime` derived once on first sync.
- **Serial debug registry:** `addInfoCommandHandler(fn)` (chained into `i`/info dump) and `addCommandHandler("name", fn)` (e.g. `wifi_reconnect`, `wifi_set_11b/g/n`, `switch_info`, `restart`). 115200 baud, `CMD_BUFFER_SIZE=30`.
- **Lib deps (pinned):** `bblanchon/ArduinoJson@~6.21.2`, `knolleary/PubSubClient@~2.8`. ArduinoJson 6.x API (`DynamicJsonDocument`, `createNestedArray`) — a 7.x bump is a breaking migration.
- **`framework = arduino`** for both platforms; `espressif8266` and `espressif32@^6.4.0` cores. Core-version-specific WiFi/mDNS behavior noted in `/net`.
- **OTA deploy folds in the `/devops` role** (no separate agent): `espota` over WiFi with `--auth=${SMARTHOME_OTA_PASSWD}`; serial fallback at 200000 baud on COM5/6. Version-bump protocol (`APP_VER` in `main.cpp`, per-module header comment dates) is owned here.
- **`monitor_speed = 115200`** across envs; `DEBUG_ESP_PORT=Serial` + `DEBUG_ESP_WIFI` enabled on COM/test builds for core-level WiFi diagnostics.

**Cursor rule globs:** `platformio.ini`, `src/Application.h`, `src/main.cpp`, `src/SerialDebug.cpp`, `src/SerialDebug.h`, `src/Log.cpp`, `src/Log.h`

---

### Common specialist candidates — include/decline decisions

Per `BOOTSTRAP_PROMPT.md` §3, each recurring candidate gets an explicit decision:

- **`/devops`** — **Declined.** Deploy = `espota` OTA + serial flash, fully described by PlatformIO envs. This is firmware-deploy knowledge (build flags, `upload_protocol`, OTA auth) folded into `/firmware`'s SKILL.md, not a CI/containerized pipeline. No CI, no cloud, no IaC.
- **`/db`** — **Declined.** No persistent storage exists; runtime config is compile-time. The *planned* "persistent setup" backlog item is EEPROM/LittleFS — when it lands it is a handful of bullets in `/firmware` (flash partition) + `/net` (WiFi portal), not a schema/migration domain. Re-evaluate only if a real on-device datastore appears.
- **`/security`** — **Declined.** Security here is two invariants — secrets-never-committed (owned `/firmware`) and the trusted-LAN posture (no TLS/auth in v1) — audited by `/review`. It's a cross-cutting invariant, not a specialist domain. Promote only if TLS/authenticated MQTT or an exposed-to-WAN posture is adopted.
- **`/perf`** — **Declined.** Real resource pressure exists (ESP8266 heap, the blocking relay pulses), but it's a recurring **`/review` checklist** concern, not a standing knowledge hub. The hot-path knowledge already lives with its owning specialist (`/power` for the blocking pulses, `/mqtt` for buffer sizing). Promote only if a hard latency SLO appears.
- **`/release`** — **Declined.** No registry/OTA-channel publishing cadence; releases are a manual `APP_VER` bump + flash. The version-bump protocol is owned by `/firmware`; tag-and-push is `/scm`. Folds cleanly into the two.
- **`/hardware`** *(domain-specific candidate)* — **Declined as an agent; promoted to a doc.** The relay/optocoupler/button electrical reference is real and load-bearing, but there is no *code* to own — it's reference knowledge. Captured in `doc/design/HARDWARE.md` (Primary `/power`, shared pin table with `/firmware`) and in `/power`'s SKILL.md, not a separate agent.
