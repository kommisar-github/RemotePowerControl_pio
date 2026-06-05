# Wave 1 Raw Findings — Legacy Code Audit (handoff to /review)

**Date:** 2026-06-06
**Status:** Raw specialist output — NOT the canonical audit. `/review` consolidates this + `doc/design/ARCHITECTURE.md` into `doc/audit/2026-06-06-audit.md`.
**Scope:** analysis-only; no source was modified. Each specialist audited its own domain.

Severity legend: CRITICAL / HIGH / MEDIUM / LOW / INFO.

---

## /power — RSwitch (`src/RSwitch.cpp`, `src/RSwitch.h`) — health: DEGRADED (11)

- **CRITICAL BUG-01** `RSwitch.cpp:308` — Duplicate condition in `longPress()`; 2nd `else if (isPowerOff())` identical to 1st (line 303) → dead branch. Force-off while ON falls to `else` → `POWERING_OFF`→`UNDEFINED`. HA sees UNDEFINED on force-off; relay still fires (line 317). Impact C2. Fix: 2nd cond should be `isPowerOn()` / `!isPowerOff()`.
- **HIGH BUG-02** `RSwitch.h:50-51`, `RSwitch.cpp:95-111` — `on_time`/`off_time` init 0, never updated. `getOnDurationSec()/getOffDurationSec()` return time-since-boot. Impact C2 (info telemetry). Fix: set on transition in `set_power_status()`.
- **HIGH BUG-03** `RSwitch.cpp:347` — `setSwitch(false)` guard `isPowerOn()||isPoweringOff()` double-pulses relay when already POWERING_OFF (short_push fires, set_power_status no-ops). Impact C3. Fix: guard `isPowerOn()` only.
- **MED BUG-04** `RSwitch.cpp:317` — `long_push()` after `set_power_status()` → callback fires before relay activates. Logically inverted (benign due to 10s block). Fix: relay first.
- **MED BUG-05** `RSwitch.cpp:332-337` — Wake-from-SLEEP spoof clears stat_changed/short_pulse but not `long_pulse` → possible spurious validation. Fix: add `long_pulse=0`.
- **MED BUG-06** `RSwitch.cpp:336` — Wake back-dates `verify_time_prev` by 3s but confirm window is 6s (VALIDATE_LONG_PULSE_SEC) → confirm window may misfire. Fix: back-date by 6s.
- **MED BUG-07** `RSwitch.cpp:269` — 5-min periodic validation fires during SLEEP; mid-pulse GPIO read can spuriously set POWER_ON/OFF. Known floating-input edge. Fix: skip periodic validation when status==SLEEP.
- **LOW BUG-08** `RSwitch.cpp:404` — `stat_changed`(int) printed `%u`. Debug only.
- **LOW BUG-09** `RSwitch.cpp:332-337` — wake spoof doesn't reset `stat_current` (dormant path). 
- **LOW BUG-10** `RSwitch.cpp:133-221` — dormant `detect_power_status_new()` diverges silently from active `_old()` (state_change_time vs verify_time_prev). Maintenance risk.
- **LOW BUG-11** `RSwitch.cpp:95-111` — duration getters return 0 in transitional states, indistinguishable from near-zero. Worsened by BUG-02.

Health: one critical dead-branch, one silent telemetry bug, one C3 double-pulse; rest timing edge-cases/cosmetic.

---

## /mqtt — MqttClient + main.cpp MQTT section — health: mostly sound, edge-case gaps (10)

- **CRITICAL C-1** `main.cpp:499` — `mqtt_received_callback` reads `payload[2]`/`payload[0]` with NO length guard → OOB read on payload <3 bytes; garbage switch_idx may pass `-1<idx<3`, wrong relay or crash. Impact C3. Fix: `if (length<3) return;`.
- **MED M-1** `MqttClient.cpp:95` — `begin(server,id,enabled)` delegates to 4-arg which forces `_enabled=true`; `begin(...,false)` silently ignored. Broken API contract (not currently triggered).
- **MED M-2** `main.cpp:350-443` — `send_info(connection_info=true)` frame (~1054B) > MQTT_MAX_PACKET_SIZE 1024 → publish returns false, caller ignores. HA gets no telemetry silently. Impact C1.
- **MED M-3** `main.cpp:87` — `DynamicJsonDocument(1024)` pool likely exhausted by full info field set; ArduinoJson silently drops trailing fields (wifi_lost/mqtt_lost/timestamps). Impact C1. Fix: 2048 or split.
- **LOW L-1** `MqttClient.cpp:217-224` — `publish_announcement()` bypasses `MqttClient::publish()` wrapper (no size log / guard).
- **LOW L-2** `MqttClient.cpp:235` — `resubscribe()` subscribes dead legacy `"inTopic"` each reconnect.
- **LOW L-3** `MqttClient.cpp:230-235` — subscribe() return values ignored; silent deafness on failure.
- **LOW L-4** `main.cpp:512-513` — `STR_POWER_GET` handler no-op (commented). HA can't poll on demand.
- **INFO I-1** `MqttClient.cpp:143-169` — blocking `_client->connect()` (1s WiFiClient timeout, 10s interval, WiFi-gated). For /review non-blocking-loop awareness.
- **INFO I-2** `main.cpp:64-65` — announcement/lastWill duplicate literals (correct per LWT contract; consider single #define).
- **INFO I-3** LWT/announcement non-retained by design; HA restart sees connected only on next keepalive.

Health: 1 critical (OOB), 2 silent telemetry failures; retained/LWT/resubscribe correct.

---

## /net — WiFiConnect/OTA/MdnsClient/Test — health: functional, latent drop/restore defects (13)

- **HIGH H1** `OTA.cpp:13-18`, `MdnsClient.cpp:11-16` — `#ifdef ESP8266 && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)`; `#ifdef` takes one identifier, rest silently ignored. Guard intent non-functional. WiFiConnect.cpp uses correct `#if defined()`. Fix: use `#if defined()`.
- **HIGH H2** `Test.cpp:188-196` — WiFi-lost resets `wifi_conn_timestamp=0` but NOT `mqtt_conn_timestamp`. On restore, stale mqtt_conn_timestamp>0 → EVENT_MQTT_LOST after EVENT_WIFI_RESTORED (wrong order); EVENT_MQTT_RESTORED can be silently skipped. Impact C4 (drives refresh_all_switches_state). HA stale after reconnect. Fix: reset mqtt_conn_timestamp in WiFi-lost branch.
- **MED M1** `WiFiConnect.cpp:183` — ESP32 `WiFi.setHostname(hostname)` called unconditionally outside `hostname!=NULL` guard → NULL to WiFi stack via begin(ssid,pwd). ESP8266 path has no such call (parity bug).
- **MED M2** `WiFiConnect.cpp:81-87` — `begin(ssid,pwd,hostname,mdns_enabled)` delegates dropping mdns_enabled → silently disabled. Not active today.
- **MED M3** `Test.cpp:134-186` — event order WIFI_LOST→WIFI_RESTORED→MQTT_LOST→MQTT_RESTORED instead of WIFI_LOST→MQTT_LOST→WIFI_RESTORED→MQTT_RESTORED. Fixed by H2.
- **LOW L1** `WiFiConnect.cpp:178,268` — `this->hostname != WiFi.getHostname()` pointer compare (always true). Fix: strcmp w/ NULL guard.
- **LOW L2** `WiFiConnect.cpp:364` — `period<0` on unsigned diff dead branch.
- **LOW L3** `WiFiConnect.cpp:196-200` — setSleepMode called every 500ms in connect wait loop; move before loop.
- **LOW L4** `OTA.cpp:67-77` — mDNS log asymmetry ESP8266(conditional)/ESP32(unconditional).
- **LOW L5** `MdnsClient.cpp:34-36` — ESP32 debug_info branch empty (parity gap, TODO).
- **LOW L6** `MdnsClient.cpp:42-51` — `findAllServices()` public but fully commented out (dead API).
- **LOW L7** `Test.cpp:181` — hardcoded network ID `192.168.66.0` in bad-DHCP detection (deployment-specific).
- **LOW L8** `Test.cpp:45-48` — `Test()` ctor calls `begin()`→pinMode/digitalWrite at static-init before Arduino GPIO ready; redundant with setup() begin(). Fix: remove from ctor.

Health: 2 HIGH (broken guards; C4 reconnect-state), parity edges, static-init risk.

---

## /web — WebSServer + strings1.h — health: PARTIALLY BROKEN (8)

- **CRITICAL C1** `WebSServer.cpp:42-43` — `page.replace("device1","DEVICE1")` matches substring → `{device1}`→`{DEVICE1}` (braces kept); `{device2/3}`,`{stat1/2/3}` NEVER replaced; `version` never substituted. Cards render literal placeholders. Fix: replace full `{device1}` tokens; source names from /firmware-/power.
- **MED M1** `WebSServer.cpp:101` — `DynamicJsonDocument(256)` borderline; pool overflow → empty/truncated doc → JS JSON.parse crash. Fix: 384.
- **MED M2** `strings1.h:4-7` — TAG_STAT/TAG_VER/PAGE_BUTTONS_OLD/PAGE_BUTTONS non-static const char* PROGMEM at file scope in header → latent multiple-definition linker hazard. Fix: static or extern+def in .cpp.
- **MED M3** `strings1.h:6` — `PAGE_BUTTONS_OLD` dead; JS uses `,` not `&` between query params (`/setswitch?switch=e,status=x`) → unparseable. ~2KB flash waste. Fix: remove.
- **LOW L1** `WebSServer.cpp:126` — `/getstatus` sends `text/plain` not `application/json`. Fix MIME.
- **LOW L2** `strings1.h:4-5` — TAG_STAT/TAG_VER dead symbols.
- **LOW L3** `strings1.h:7` (JS) — force checkbox state overwritten by 2s getStatus poll → erases force-off feedback during 10s long_push. Coordinate /power on POWERING_OFF signal.
- **LOW L4** `WebSServer.cpp:18,39-45` — `version` stored but never rendered. Fix with C1.

ESP8266/ESP32 parity: clean. setswitch 0-based index: correct.

---

## /firmware — platformio.ini, main.cpp shell, Application.h, SerialDebug, Log — health: MODERATE RISK (14)

- **CRITICAL F-01** `Application.h:45` — ESP32 `POWER_SWITCH2_GPIO=5` (GPIO5) strapping pin pulled HIGH at boot → relay fires on reset/flash if active-high. BOOT-SAFE-GPIO VIOLATED. Fix: reassign to non-strapping pin (GPIO13/14/27); polarity w/ /power.
- **HIGH F-02** `Application.h:33` — ESP8266 `POWER_SWITCH2_GPIO=D8`(GPIO15) strapping pin pulled LOW at boot; comment misleading. Safe only if active-high. Confirm polarity w/ /power.
- **HIGH F-03** `Log.h:21` vs `Log.cpp:78` — header declares `LOG_DDEBUG`, impl defines `LOG_DEBUG`. Mismatch: LOG_DDEBUG undefined (linker error if called); LOG_DEBUG not in header. Reconcile.
- **HIGH F-04** `Log.h:18` vs `Log.cpp:68` — `LOG_ERROR(char*,const char*)` decl vs `LOG_ERROR(const char*,const char*)` def. Unresolved overload. Add const to header.
- **MED F-05** `main.cpp:50-51` — `boottime`,`timeset_cnt` modified in SNTP sync callback but not `volatile` → stale reads in send_info/main_debug_info. Fix: volatile.
- **MED F-06** `main.cpp:257-260` — ESP32 SNTP hardcoded numeric offsets vs ESP8266 POSIX TZ string → DST/parity gap. Fix: ESP32 setenv TZ POSIX string.
- **MED F-07** `main.cpp:442` — send_info(connection_info=true) into DynamicJsonDocument(1024)→BUF[1024]; serializeJson silently truncates on overflow, no check. (Corroborates mqtt M-2/M-3.)
- **MED F-08** `main.cpp:91` — `send_status()` fwd-decl, no impl (call sites 509/513 commented). Dead decl.
- **LOW F-09** `platformio.ini:88,114,141` — build_flags mix tabs/spaces vs RELEASE_OTA_esp8266 spaces. Normalize.
- **LOW F-10** `main.cpp:314` — `unsigned long ll = millis()` at global scope (~0); suppresses button print ~1s post-boot.
- **LOW F-11** `main.cpp:333-336` — button state printed every 1s in ALL profiles incl release. Wrap in test ifdef.
- **LOW F-12** `main.cpp:369-375` — `ctime()` static-buffer pointer stored as `&t[4]`; safe under ArduinoJson 6 (copies) but fragile. Copy to local.
- **LOW F-13** `Application.h:13` — `#include "log.h"` lowercase; file is `Log.h`. Breaks Linux CI.
- **LOW F-14** `SerialDebug.cpp:3-5` — version history date typo (v0.1 dated after v0.2).

Invariants: Secrets CLEAN; Boot-safe GPIO VIOLATED (F-01) + SUSPECT (F-02); Non-blocking-loop CLEAN; init order CLEAN; lib pins CLEAN.

---

## /arch — Architecture reconstruction (`doc/design/ARCHITECTURE.md` refreshed, full body)

Produced: 11-module boundary table; load-bearing setup() init order + rationale; loop() service model + MQTT gate; 4 data-flow diagrams (MQTT cmd→relay C1/C3, power-change→publish C2, web C5, conn-health C4); C1–C6 contracts vs REAL code signatures (supersedes bootstrap stubs); dual-MCU divergence map (13 rows); invariant-enforcement map. No new docs → no matrix row needed. Specialist docs (POWER_STATE/HARDWARE/MQTT_API/BUILD) intentionally untouched.

Architecture observations for /review (sec 9 of ARCHITECTURE.md):
1. [power] longPress() dead branch (corroborates BUG-01).
2. [net] malformed `#ifdef ESP8266 && ...` OTA.cpp 13/16/67 (corroborates H1).
3. [power+firmware] SWITCH2 strapping pins D8/GPIO15 + GPIO5; needs boot-safety rationale in HARDWARE.md (corroborates F-01/F-02).
4. [mqtt] power/get matched but handler commented (no-op) (corroborates L-4).
5. [mqtt+power] POWERING_ON/OFF suppress state/ publish (commented) → state/ holds stale 0/1 during transition. Confirm intended.
6. [power] detect_power_status_new() unreferenced dead code; _old() active (corroborates BUG-10).
7. [firmware] send_status() fwd-decl dead (corroborates F-08).
Confirmed no drift: detect polarity LOW=ON/HIGH=OFF; POWER_STATUS enum numbering; init order; retained discipline; SLEEP-wake stat_prev spoof. NOTE: MqttClient::setConnectEvent() exists but main.cpp doesn't use it — connection detection is entirely Test polling (intentional; MqttClient connect_event path dead in this build).

---

## Cross-corroboration index (high confidence — 2+ independent agents)
- longPress() dead branch: power BUG-01 + arch #1
- broken `#ifdef && ...` guards: net H1 + arch #2
- boot-safe strapping pins: firmware F-01/F-02 + arch #3
- info JSON > 1024B silent failure: mqtt M-2/M-3 + firmware F-07
- no-op power/get: mqtt L-4 + arch #4
- dead code (send_status, detect_power_status_new): firmware F-08 + power BUG-10 + arch #6/#7

## Systemic theme
"Silently-ignored configuration" failure class: broken `#ifdef` guards (net), overloads dropping args (mqtt begin(enabled), net begin(mdns_enabled)), web substring replace(). Recommend /review treat as a pattern, not just point fixes.
