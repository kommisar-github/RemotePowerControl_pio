## Section 6 — Document Ownership Matrix

Single-repo shape → matrix **Sections 1–4 only, no appendices** (the dual-MCU split is `#ifdef`, not separate platforms). Paste each `§6.N` below into the matching slot of `doc/design/DOC_OWNERSHIP_MATRIX.md` (built from `PM_TEMPLATES.md → DOC_OWNERSHIP_MATRIX.md Template`). The template's own **Section 3 (Abstract standard format spec)** is copied verbatim from the template — `§6.3` below produces the *Abstract payloads* destined for the individual design-doc files, not for the matrix.

### §6.1 — Matrix Section 1 (cross-cutting design + architecture table)

| Document | Type | Primary | Secondary | Notes (what triggers an update) |
|---|---|---|---|---|
| `doc/plans/ROADMAP.md` | roadmap | `/pm` | all | Phase overview + status badges. Update on every phase gate (open/close). |
| `doc/NEXT_STEPS.md` | roadmap | `/pm` | all | Current action items. Update after each implement/verify cycle. |
| `doc/MEMORY.md` | memory | `/pm` | all | Cross-phase decisions, hardware facts, the "detect-pin disconnected" bug findings. Update when a phase closes or a non-obvious lesson is learned. |
| `doc/design/DOC_OWNERSHIP_MATRIX.md` | reference | `/pm` | all | **This file.** Update in the same commit as any doc add/rename/owner-change. |
| `doc/BOOTSTRAP_PLAN.md` (+ `BOOTSTRAP_PLAN/`) | plan | `/pm` | all | This plan. Archived post-bootstrap; §7 (contracts) + §8 (waves) stay live unless migrated to `ARCHITECTURE.md`. |
| `doc/design/ARCHITECTURE.md` | design | `/arch` | all specialists | Module map, `setup()` init order, cooperative-loop contract, dual-MCU rule. Update when a module/boundary/init-order changes. |
| `doc/design/POWER_STATE.md` | design | `/power` | `/mqtt`, `/web`, `/review` | RSwitch state machine + pulse-detection algorithm + timing constants. Update when state logic or a timing constant changes. |
| `doc/design/HARDWARE.md` | reference | `/power` | `/firmware`, `/review` | Relay/optocoupler/button wiring + per-MCU pin behavior + boot-safety. Update only on hardware/pin-map change. |
| `doc/design/MQTT_API.md` | reference | `/mqtt` | `/power`, `/firmware`, `/review` | Topic grammar, payload byte-layout, retained/LWT rules, `info` JSON schema, HA mapping. Update when a topic/payload/field changes. |
| `doc/design/BUILD.md` | reference | `/firmware` | `/net`, `/scm`, `/review` | PlatformIO env matrix, build flags/secrets, flashing + OTA procedure, lib-dep pins. Update on env/flag/dependency/board change. |
| `doc/design/PHASE_<N>_DESIGN.md` | design | `/arch` | phase-relevant specialists | Created at each phase start from `/arch` output; mark `SUPERSEDED` (not deleted) when the phase closes. |

### §6.2 — Matrix Section 2 (cross-cutting load rules)

**When dispatched for a task in a domain (single-repo shape):**
1. **Primary agent** reads `ROADMAP.md` + `NEXT_STEPS.md` + `MEMORY.md` + all docs where it is Primary. Dedicated-terminal: amortized at startup. Fork: only the matrix + docs cited in `Context docs:`.
2. **Secondary agents** on the same dispatch read only the specific design doc that applies (usually cited by PM).
3. **All agents** read their `.claude/skills/<agent>/SKILL.md` first.

**When PM is doing triage / planning:** PM reads `CLAUDE.md`, `ROADMAP.md`, `NEXT_STEPS.md`, `MEMORY.md`, and this matrix. PM does **not** read `POWER_STATE.md` / `MQTT_API.md` / `BUILD.md` in full unless triaging that specific domain.

**When writing a new doc:** cross-cutting design → `doc/design/`; planning/status → `doc/plans/` or `doc/`; **add a matrix row in the same commit** — un-matrixed docs rot within weeks.

**Worked token-savings example.** A `/power` fork dispatched for *"tune the SLEEP-detection short-pulse threshold so a flickering UPS LED isn't read as sleep"* loads:
- `doc/design/DOC_OWNERSHIP_MATRIX.md` (~2K)
- `doc/design/POWER_STATE.md` (~6K, cited in `Context docs:`)
- `doc/design/HARDWARE.md` detect-pin section (~4K, cited)

Total **≈ 12K tokens**. Without the matrix the same fork would cold-load `ARCHITECTURE.md` + `MQTT_API.md` + `BUILD.md` as well (~55–65K) just to locate the constants. ~5× saving; context freed for the actual timing reasoning.

**Ownership transfer (when a doc changes Primary).** Use the template's protocol **verbatim** (PM proposes → user approves → matrix row + the doc's `## Abstract` `Owner:` updated in the **same commit** as the motivating code change → log a `## History` entry). Never silently change the Primary column. Likely future transfer for this project: if the "persistent setup / WiFi portal" work makes config a first-class concern, parts of `BUILD.md` may split into a `CONFIG.md` owned jointly by `/firmware` + `/net` — flag, don't pre-split.

### §6.3 — Abstract payloads (destination: each design doc's own file, top-of-file)

Five design/reference docs → five `## Abstract` blocks. Paste each at the top of the named file (after the title), **not** into the matrix.

**`doc/design/ARCHITECTURE.md`:**
```markdown
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
```

**`doc/design/POWER_STATE.md`:**
```markdown
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
```

**`doc/design/HARDWARE.md`:**
```markdown
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
```

**`doc/design/MQTT_API.md`:**
```markdown
## Abstract

**TL;DR:** The MQTT contract between RemotePowerControl and Home Assistant — topic grammar, set/get payload byte-layout, retained-vs-not rules, Last Will, keepalive, and the info-JSON telemetry schema.

**Load when:** MQTT, topic, home assistant, HA, PubSubClient, retained, last will, LWT, announcement, keepalive, power/set, power/get, power/state, power/status, info, alive, payload format, switch index, json telemetry, broker, subscribe, resubscribe, reconnect, buffer size

**Key facts:**
- Topics: `<area>/<host>/power/{state,status}/<idx>` (retained), `.../power/{set,get}`, `<area>/alive(/get)`, `<area>/<host>/info(/get)`, `<area>/keepalive/<host>`, `<area>/announcement/<host>` (=LWT topic).
- set payload byte layout: payload[0]=action ('0' off / '1' on / '3' force), payload[2]=1-based switch char; index = payload[2]-'1'.
- state/status publish retained=true; get/set/keepalive/announcement non-retained — a retained set would re-fire the relay on reconnect.
- LWT: connect="connected", will="disconnected" on the announcement topic (WILLQOS=0); HA marks offline via the will.
- PubSubClient buffer MQTT_MAX_PACKET_SIZE=1024; resubscribe() is mandatory on every reconnect or the device goes deaf.

**Owner:** `/mqtt` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `POWER_STATE.md`, `ARCHITECTURE.md`, `BUILD.md`
```

**`doc/design/BUILD.md`:**
```markdown
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
```

### §6.4 — Matrix Section 4 (Quick-reference: which agent for which topic)

| If the task is about… | Delegate | Context docs |
|---|---|---|
| Planning, phase gating, delegation, doc updates | `/pm` | `ROADMAP.md` + `NEXT_STEPS.md` + `MEMORY.md` + this matrix |
| Architecture, module boundaries, init order, dual-MCU convention | `/arch` | `ARCHITECTURE.md` + relevant `PHASE_<N>_DESIGN.md` |
| Auditing a change / design / plan; RAM, invariants, ESP8266-ESP32 parity | `/review` | the diff/doc under review + `ARCHITECTURE.md` |
| Git, commits, branches, tags, PRs | `/scm` | (dispatch payload describes the change) |
| Relay control, power-state detection, RSwitch state machine, pulse timing, SLEEP | `/power` | `POWER_STATE.md` + `HARDWARE.md` |
| Hardware wiring, GPIO behavior, boot-safe pins, optocoupler | `/power` (+ `/firmware` for pin numbers) | `HARDWARE.md` |
| MQTT topics, payloads, retained/LWT, keepalive, `info` JSON, Home Assistant | `/mqtt` | `MQTT_API.md` |
| WiFi connect/reconnect, PHY mode, OTA flashing behavior, mDNS, connection health | `/net` | `ARCHITECTURE.md` (connectivity) + `BUILD.md` (OTA) |
| Web UI, HTTP routes, `/getstatus` JSON, the buttons page | `/web` | `ARCHITECTURE.md` (web routes) |
| Build envs, build flags, secrets, pin map, serial debug, `main.cpp` wiring, OTA deploy | `/firmware` | `BUILD.md` + `HARDWARE.md` (pin table) + `ARCHITECTURE.md` (init order) |
| `main.cpp` change spanning topic + power callback + init | `/pm` coordinates → owning section specialist | `ARCHITECTURE.md` + §7 contract C6 (shared-file ownership) |
