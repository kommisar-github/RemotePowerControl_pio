## Section 8 — Roadmap → wave dispatch mapping

The project has no formal roadmap yet — these phases are derived from `README.txt` (TODO + VERIFY lists) and the v1 boundary in §1. PM seeds `doc/plans/ROADMAP.md` from this mapping at bootstrap. This is **not** a Learning-First project, so there are **no** `(author, /learn reviews)` non-delegable waves.

**Wave parallelism is mostly serial** because the v1 scope has one load-bearing specialist per milestone, a single solo operator (one attention head), and a single shared-scope `loop()` runtime where most changes touch `main.cpp` (a shared file — C6). Per `BOOTSTRAP_PROMPT.md` §8 honest-parallelism rule, that is a truthful map, not a planning shortfall.

---

### Phase 0 — Bootstrap & baseline (this plan)

**Active agents:** pm, arch, firmware
**Parallelism:** Serial

| Wave | Tasks | Agent(s) | Dependencies |
|------|-------|----------|--------------|
| 0.1 | Apply bootstrap (skills, agents.json, matrix, design-doc stubs) | pm | — |
| 0.2 | Author `ARCHITECTURE.md` (module map, init order, loop contract) | arch | 0.1 |
| 0.3 | Author `BUILD.md` + `HARDWARE.md` (env matrix, pin maps, secrets) | firmware | 0.1 |
| 0.4 | Author `POWER_STATE.md` + `MQTT_API.md` from current code | power, mqtt | 0.1 |
| 0.5 | Verify clean build on `RELEASE_OTA_esp8266` and `RELEASE_COM_esp32dv` | firmware | 0.2–0.4 |

*0.3 and 0.4 are file-disjoint (different design docs) and could run 2-way if the operator runs two terminals; default Serial.*

---

### Phase 1 — MQTT state reliability & HA correctness (README VERIFY block)

**Active agents:** pm, mqtt, power, net, review
**Parallelism:** Serial (every wave converges on the MQTT publish path / `main.cpp`)

| Wave | Tasks | Agent(s) | Dependencies |
|------|-------|----------|--------------|
| 1.1 | Verify `EVENT_MQTT_RESTORED` re-publishes **all 3** retained switch states (C4 + the "resend all switch status" TODO) | mqtt | 0.5 |
| 1.2 | Verify LWT marks the device offline in HA on power-off/restart; confirm `announcement`=`lastWill` topic behavior | mqtt | 1.1 |
| 1.3 | Reproduce + diagnose the "detect pin disconnected → status sticks ON on D0 not D5" bug (VERIFY block) | power | 1.1 |
| 1.4 | Fix the floating-detect-input validation-timer edge case from 1.3 | power | 1.3 |
| 1.5 | Audit retained-flag correctness + heap impact across the changes | review | 1.2, 1.4 |

*1.2 (`/mqtt`) and 1.3 (`/power`) are genuinely file-disjoint (MqttClient vs RSwitch) — 2-way if two terminals are run; default Serial.*

---

### Phase 2 — Web UI: unavailable/offline states (README TODO)

**Active agents:** pm, web, power, review
**Parallelism:** Serial

| Wave | Tasks | Agent(s) | Dependencies |
|------|-------|----------|--------------|
| 2.1 | Extend `/getstatus` JSON + `decodePowerStat` so offline/disabled channels report an unavailable marker (C5) | power, web | 1.5 |
| 2.2 | Update `strings1.h` page: render greyed/Unavailable card + disabled toggle when channel offline | web | 2.1 |
| 2.3 | Reflect device-level offline (no recent poll) as all-grey in the UI | web | 2.2 |
| 2.4 | Review: ESP8266/ESP32 `WebServer` parity + PROGMEM size of the new page | review | 2.3 |

*2.1 touches the C5 boundary — `/power` (`decodePowerStat`) then `/web` (consume); serialized because they share the contract.*

---

### Phase 3 — Persistence & WiFi setup portal (README TODO)

**Active agents:** pm, arch, net, firmware, review
**Parallelism:** Serial (new subsystem; design-gated)

| Wave | Tasks | Agent(s) | Dependencies |
|------|-------|----------|--------------|
| 3.1 | `/arch` designs persistence layer (LittleFS/EEPROM) + setup-portal flow → `PHASE_3_DESIGN.md` | arch | 2.4 |
| 3.2 | Implement persistent config store (flash partition, load on boot) | firmware | 3.1 |
| 3.3 | Implement WiFi setup/captive portal + "Setup portal" button trigger | net | 3.2 |
| 3.4 | Wire persisted config into `setup()` (SSID/MQTT/host from store, not just build flags) | firmware | 3.3 |
| 3.5 | Review: secrets-at-rest handling, boot-time fallback, dual-MCU flash layout | review | 3.4 |

*This phase likely fires the **technology-boundary** evolution trigger (filesystem) and possibly a `/config` agent — see §11.*

---

### Phase 4 — Telephone / IVR interface (post-v1, README TODO)

**Active agents:** pm, arch (+ a likely new specialist)
**Parallelism:** Serial — design only at this stage

| Wave | Tasks | Agent(s) | Dependencies |
|------|-------|----------|--------------|
| 4.1 | `/arch` scopes the IVR/telephony integration (external gateway vs on-device) → `PHASE_4_DESIGN.md` | arch | 3.5 |
| 4.2 | PM evaluates whether IVR warrants a new `/ivr` specialist (new-domain trigger) | pm | 4.1 |

*Explicitly post-v1; kept in the map so the roadmap is complete, not to schedule it now.*
