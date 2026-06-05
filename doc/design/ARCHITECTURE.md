# Architecture — RemotePowerControl

## Abstract

**TL;DR:** Module map and runtime contract for RemotePowerControl firmware — how RSwitch, MqttClient, WiFiConnect/OTA/mDNS/Test, WebSServer, and the main.cpp shell compose under a single cooperative loop() on ESP8266 and ESP32.

**Load when:** architecture, module map, component boundaries, setup init order, loop, handleClient, cooperative scheduling, non-blocking, dual-MCU, ESP8266, ESP32, ifdef, callback wiring, integration, data flow, who calls what, system design

**Key facts:**
- Every module exposes `handleClient()` polled once per `loop()`; no long blocking allowed (non-blocking-loop invariant) — relay pulses are the grandfathered exception.
- `setup()` init order is load-bearing: test → Serial → time/SNTP → WiFi → MQTT(+topics+callback) → RSwitch[3] → OTA → web → serial-debug. Re-ordering breaks callback wiring.
- MQTT is only serviced once `wifiConnect.connectionEstablished()` is true.
- Dual-MCU divergence lives behind `#if defined(ESP8266)/ESP32`, never a forked source file.
- Cross-module state flows by C function-pointer callbacks (RSwitch→main→MQTT), not direct calls.
- Connection-health (WiFi/MQTT lost/restored) is detected by `Test` polling, **not** by MqttClient's own `connect_event` (which is wired in the class but unused by `main.cpp`).

**Owner:** `/arch` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `POWER_STATE.md`, `MQTT_API.md`, `BUILD.md`, `HARDWARE.md`

> **Provenance:** This document was reconstructed from the actual `src/` code on
> 2026-06-06 (ARCH-RECONSTRUCT task). Where the code diverges from `CLAUDE.md` /
> the §7 interface-contract notes, the divergence is flagged inline and collected
> in [§9 Observations for /review](#9-observations-for-review). The C1–C6 contracts
> below are documented against the **real signatures in code**, superseding the
> bootstrap stubs.

---

## 1. System overview

RemotePowerControl is single-binary Arduino-framework firmware for a Wemos D1
mini (ESP8266) or D1 mini ESP32 that controls and monitors **three independent
power channels**. Each channel is a momentary relay (simulating a front-panel
power button press) plus an optocoupler that reads the host's power LED, so the
firmware infers true ON/OFF/SLEEP state rather than assuming the relay's last
command took effect.

The device exposes that state and accepts commands over four surfaces:

```
                         ┌──────────────────────────────────────────────┐
                         │                  ESP8266 / ESP32              │
   MQTT broker  ◄──1883──┤  MqttClient ──┐                              │
   (Home Assistant)      │               │                              │
                         │  WebSServer ──┤   main.cpp shell (callbacks) │
   Browser      ◄──:80───┤   (HTTP UI)   ├─► RSwitch[0]  ─► relay0 / detect0
                         │               ├─► RSwitch[1]  ─► relay1 / detect1
   espota / IDE ◄─mDNS───┤  OTAClient    ├─► RSwitch[2]  ─► relay2 / detect2
   (<host>.local)        │               │                              │
                         │  WiFiConnect ─┘   Test (health) · SerialDebug │
                         └──────────────────────────────────────────────┘
```

- **MQTT** (`MqttClient` + PubSubClient, plain TCP `:1883`, no TLS/auth — trusted
  LAN) is the primary integration surface for Home Assistant.
- **HTTP UI** (`WebSServer`, port 80) serves a single PROGMEM buttons page that
  polls `/getstatus` every 2 s.
- **mDNS + ArduinoOTA** (`OTAClient`, `MdnsClient`) advertise `<host>.local` and
  accept over-the-air sketch updates.
- **Serial debug** (`SerialDebug`, `Log`) is the local diagnostic console.

### 1.1 Component / module boundaries

| Module | Files | Owner | Responsibility | Singleton |
|---|---|---|---|---|
| **RSwitch** | `RSwitch.{h,cpp}` | `/power` | Per-channel state machine: momentary relay pulse + optocoupler power-state detection. One instance per channel (3 total, heap-allocated). | `new RSwitch[3]` via `rSwitch**` |
| **MqttClient** | `MqttClient.{h,cpp}` | `/mqtt` | PubSubClient wrapper: connect/reconnect, LWT, subscribe/publish, inbound dispatch to a user callback. | `mqttClient` |
| **WiFiConnect** | `WiFiConnect.{h,cpp}` | `/net` | WiFi lifecycle: connect (blocking or non-blocking), reconnect with PHY escalation, hostname, mDNS responder, rich getters. | `wifiConnect` |
| **OTAClient** | `OTA.{h,cpp}` | `/net` | ArduinoOTA wrapper; OTA hostname/password, progress/error logging. | `otaClient` |
| **MdnsClient** | `MdnsClient.{h,cpp}` | `/net` | mDNS service discovery helper (used only by `debug_info`). | `mdnsClient` |
| **Test** | `Test.{h,cpp}` | `/net` | Connection-health monitor: drives status LED, fires WiFi/MQTT lost/restored events, keepalive timer, lost/restored counters + timestamps. | `test` |
| **WebSServer** | `WebSServer.{h,cpp}`, `strings1.h` | `/web` | Embedded HTTP server: `/`, `/setswitch`, `/getstatus`; PROGMEM HTML/CSS/JS page. | `webSServer` |
| **SerialDebug** | `SerialDebug.{h,cpp}` | `/firmware` | Serial command console: `addInfoCommandHandler`, `addCommandHandler`. | `serialDebug` |
| **Log** | `Log.{h,cpp}` | `/firmware` | `LOG(...)` / `LOG_ERROR(...)` logging macros. | (macros) |
| **main.cpp shell** | `main.cpp` | `/firmware` (+ `/mqtt`, `/power` sections — see [C6](#c6--maincpp-shared-file-ownership)) | Globals, `setup()`, `loop()`, `send_info()`, SNTP, callback wiring, restart. | — |
| **Application.h** | `Application.h` | `/firmware` | Per-MCU pin map (`#define`s), board comment table. | — |

**Boundary rule:** modules never call each other directly across domains. State
flows through **C function-pointer callbacks wired in `main.cpp`** and through one
shared-pointer handoff (`RSwitch**` → `WebSServer::begin`). `Test` is the one
module that reaches into peers (`wifiConnect.connected()`, `mqttClient.connected()`)
because it is by design a cross-cutting health probe.

---

## 2. Init sequence — `setup()`

The order below is **load-bearing**: each step either registers a callback that a
later step's object depends on, or must run before the object it configures is
first serviced in `loop()`. Source: `main.cpp::setup()` (lines ~241–309).

```
 1. test.begin(test_change_event_callback, TEST_LED_PULLUP)   // LED pinMode + health-event cb
 2. test.setKeepAliveEventCallback(keepAlive_event_callback, 60000)
 3. Serial.begin(115200); delay(2000); print banner (APP_NAME/APP_VER)
 4. Time / SNTP:
       ESP8266: settimeofday_cb(settime_cb); configTime(MYTZ, "pool.ntp.org")
       ESP32  : sntp_set_time_sync_notification_cb(settime_cb);
                configTime(gmtOffset=7200, dst=3600, "pool.ntp.org")
 5. wifiConnect.begin(SSID, PWD, HOST)                        // BLOCKING until WL_CONNECTED
 6. mqttClient.begin(mqtt_server, clientId, true)             // enabled=true
       mqttClient.setAnnouncementTopic(announcementTopic)
       mqttClient.setLastWillTopic(lastWillTopic)             // == announcementTopic
       mqttClient.setTopics(topics, TOPIC_COUNT=4)
       mqttClient.setCallback(mqtt_received_callback)
 7. rSwitch = new RSwitch*[3]; rSwitch[i] = new RSwitch(SWITCHi_GPIO, DETECTi_GPIO)
       pinMode(BUTTON_GPIO, INPUT)
       for i in 0..2: rSwitch[i]->setCallback(power_status_change_callback)
 8. otaClient.begin(HOST, OTA_PASSWD)
 9. webSServer.begin(APP_VER, rSwitch)                        // hands the RSwitch** array to /web
10. serialDebug.add*Handler(...) x N  (main/switch/wifi/ota/mqtt/test info + commands)
11. print initial BUTTON_GPIO state
```

**Why the order matters:**
- `test.begin` first installs the health-event callback so no early WiFi/MQTT
  transition is missed.
- SNTP is configured before WiFi so the `settimeofday` callback can back-compute
  `boottime` the moment the first time sync arrives.
- WiFi is **blocking** here (`begin(SSID,PWD,HOST)` loops until `WL_CONNECTED`,
  with `delay(500)` ticks and 60 s reconnect attempts). Nothing after it runs
  until WiFi is up. The non-blocking variant (`begin_nb`) exists but is not used.
- MQTT topics + inbound callback must be registered before the first
  `mqttClient.handleClient()` in `loop()`, or the device connects but is deaf.
- RSwitch instances must exist and have their change-callback set before any
  `power_status_change_callback` can fire (detection runs in `loop()`).
- `webSServer.begin` receives the **same `rSwitch**` pointer** main owns; the web
  UI calls the RSwitch public API directly ([C5](#c5--rswitch-public-api-consumed-by-the-web-ui)).

> **Note (init vs. boot blocking):** the blocking WiFi connect and the
> `delay(2000)` after `Serial.begin` run in `setup()`, not `loop()`, so they do
> not violate the non-blocking-**loop** invariant. They do extend boot time.

---

## 3. Service model — `loop()`

Every module is cooperatively scheduled: `loop()` calls each module's
`handleClient()` exactly once per iteration; no handler may block long enough to
starve the others (notably OTA and MQTT servicing). Source: `main.cpp::loop()`
(lines ~316–337).

```
loop():
  wifiConnect.handleClient()                 // reconnect / PHY escalation / mDNS update
  if (wifiConnect.connectionEstablished())   // GATE: WiFi up AND initial setup done
      mqttClient.handleClient()              // reconnect (WiFi-gated) + _client->loop()
  otaClient.handleClient()                   // ArduinoOTA.handle()
  for i in 0..2: rSwitch[i]->handleClient()  // detect_power_status() per channel
  test.handleClient()                        // health probe + LED + events + keepalive
  serialDebug.handleClient()                 // serial command parsing
  webSServer.handleClient()                  // HTTP request servicing
  every ~1000 ms: Serial.print BUTTON_GPIO state   // debug heartbeat only
```

**Key contracts:**
- **MQTT gate:** `mqttClient.handleClient()` runs only once
  `wifiConnect.connectionEstablished()` is true (WiFi associated *and* initial
  setup complete). MqttClient's own `reconnect()` independently bails if
  `WiFi.status() != WL_CONNECTED`, so there are two layers of WiFi-gating.
- **Non-blocking-loop invariant:** the only sanctioned blocking inside a `loop()`
  path is the RSwitch relay pulse — `short_push()` = `delay(800 ms)`,
  `long_push()` = `delay(10 000 ms)`. These fire only on an actuation command and
  are the **grandfathered** exception (canonical audit owner `/review`). No new
  blocking may be added.
- **Ordering note:** `wifiConnect.handleClient()` runs before the MQTT gate so a
  just-restored link is observed in the same iteration.

---

## 4. Data-flow diagrams

### 4.1 MQTT command → relay actuation (C1 → C3)

```
HA / client publishes  <area>/<host>/power/set   payload "1 2"  (action ' ' idx)
        │
        ▼
PubSubClient → MqttClient::callback() → custom_callback
        │
        ▼
main.cpp::mqtt_received_callback(topic, payload, len)        [/mqtt section]
   topic == STR_POWER_SET ?
   idx  = payload[2] - '1'        // 1-based on the wire → 0-based index
   action = payload[0]:
        '1' → rSwitch[idx]->setSwitch(true)
        '0' → rSwitch[idx]->setSwitch(false)
        '3' → rSwitch[idx]->longPress()           // force / long-press
        │
        ▼
RSwitch::setSwitch()/longPress()  → short_push()/long_push()  (relay pulse, blocks)
   → set_power_status(POWERING_ON | POWERING_OFF | …)
```

`power/get` and `alive/get`/`info/get` are also inbound: `info/get` →
`send_info(STR_INFO, true)`, `alive/get` → `send_info(STR_ALIVE, false)`.
`power/get`'s handler body is currently empty (commented `send_status()`).

### 4.2 Power-status change → MQTT publish (C2)

```
RSwitch::handleClient() → detect_power_status() → detect_power_status_old()
   reads detect_gpio (INPUT_PULLUP):  LOW = power ON, HIGH = power OFF
   pulse analysis → set_power_status(new)
        │  (fires only on actual change)
        ▼
RSwitch::set_power_status() → power_status_change_callback(this, new, old)
        │
        ▼
main.cpp::power_status_change_callback()      [/power section]
   resolve src → index i (0..2)
        ▼
main.cpp::power_status_change(i, src, new, old)
   publish  <area>/<host>/power/status/<i+1>  = decodePowerStat(new)   retained
   publish  <area>/<host>/power/state/<i+1>   = "0"|"1"                retained
        POWER_OFF → "0"   POWER_ON → "1"   SLEEP → "0"
        POWERING_OFF / POWERING_ON → state publish SUPPRESSED (commented out)
        UNDEFINED  → no publish
```

> The transient `POWERING_*` states publish only the human `status/` string, not
> the boolean `state/`. Consumers that key off `state/` see the channel hold its
> last stable 0/1 until detection confirms the new steady state.

### 4.3 Web command / status (C5)

```
Browser GET /setswitch?switch=<0..2>&status=<0|1|3>
        ▼
WebSServer::handleSetSwitch()        // index is 0-BASED here (cf. MQTT 1-based)
   status 0|1 → rSwitch[id]->setSwitch(status)
   status 3   → rSwitch[id]->longPress()

Browser GET /getstatus   (polled every 2 s by PAGE_BUTTONS JS)
        ▼
WebSServer::handleGetStatus()  → DynamicJsonDocument(256)
   {"devs":[{"id":i,"stat":decodePowerStat(s),"on":(POWER_ON||POWERING_ON)} …]}
   null channel → {"stat":"DISABLED","on":false}
```

### 4.4 Connection-health events (C4)

```
Test::handleClient() → test_connection()   (every TESTCONNECTION_INTERVAL = 1 s)
   probes wifiConnect.connected() and mqttClient.connected()
   LED: OFF = no WiFi · blinking = WiFi up/MQTT down · ON = both up
   transitions fire change_event_callback(event):
        EVENT_WIFI_RESTORED(1)  EVENT_MQTT_RESTORED(2)
        EVENT_WIFI_LOST(3)      EVENT_MQTT_LOST(4)
   keepalive timer (60 s) → keepalive_event_callback()
        │
        ▼
main.cpp::test_change_event_callback(event)       [/firmware shell wiring]
   event == EVENT_MQTT_RESTORED:
        send_info(STR_CONNECTION_INFO, true)      // resend connection telemetry
        keepAlive_event_callback()                // publish <area>/keepalive/<host>="connected"
        refresh_all_switches_state()              // re-publish all 3 retained state/status
main.cpp::keepAlive_event_callback()
   if test.wifiConnected(): mqttClient.publish(STR_KEEPALIVE, "connected")
```

> **HA recovery path lives here.** `refresh_all_switches_state()` re-fires
> `power_status_change()` for all three channels on every MQTT reconnect, so
> retained topics are rebuilt after a broker restart. Only `EVENT_MQTT_RESTORED`
> is handled; `WIFI_RESTORED`/`*_LOST` are fired by `Test` but have no action in
> `main.cpp` (counters/timestamps are still recorded inside `Test`).

---

## 5. Interface contracts (C1–C6) vs. real code

Documented against the actual signatures in `src/`. These supersede the
`doc/BOOTSTRAP_PLAN/07_interface_contracts.md` stubs.

### C1 — MQTT topic + payload grammar
**Producer** `/mqtt` → **consumers** `/power`, `/web`, `/firmware`, Home Assistant.
Defined as `#define`s in the `main.cpp` MQTT section; canonical in `MQTT_API.md`.

```
Identity:  _AREA_ = "home"   _HOST_NAME_ = "rempowercntl1"
Inbound (subscribed, topics[] = 4):
  home/alive/get                       payload ignored      → send_info(alive)
  home/rempowercntl1/info/get          payload ignored      → send_info(info, conn=true)
  home/rempowercntl1/power/get         payload <idx>        → (no-op currently)
  home/rempowercntl1/power/set         payload "<act><sep><idx>"
        action = payload[0] ∈ {'0','1','3'} ; index = payload[2]-'1' ∈ {0,1,2}
  (also legacy "inTopic" is subscribed on every (re)connect)
Outbound (published):
  home/rempowercntl1/power/state/<n>   retained, "0"|"1"           (n = idx+1, 1-based)
  home/rempowercntl1/power/status/<n>  retained, decodePowerStat() human string
  home/rempowercntl1/info | home/alive payload = info JSON (DynamicJsonDocument 1024)
  home/rempowercntl1/connection/info   info JSON incl. connection counters
  home/keepalive/rempowercntl1         "connected"   (every 60 s while WiFi up)
  home/announcement/rempowercntl1      "connected" on connect / "disconnected" LWT
```
**Retained discipline:** `state/` + `status/` are retained; `get`/`set`/`keepalive`/
`announcement` are not. A stray retained `set` would re-fire the relay on every
reconnect. **Break risk:** renaming a topic or shifting payload byte positions
silently desyncs HA + web JS — gated as a C1 change touching `MQTT_API.md`.

### C2 — Power-status-change callback
**Producer** `/power` → **consumer** `/mqtt` (via `main.cpp` wiring).

```c
void (*power_status_change_callback)(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status);
// registered: rSwitch[i]->setCallback(power_status_change_callback);
// fired only on actual transition, from RSwitch::set_power_status()
// main resolves src→index (0..2) and publishes state/<n> + status/<n>
```
`POWER_STATUS` is **wire-significant** and must never be renumbered:
`UNDEFINED=-1, POWER_OFF=0, POWER_ON=1, SLEEP=2, POWERING_OFF=10, POWERING_ON=11`.
**Break risk:** adding a `POWER_STATUS` value without updating both the publish
`switch` in `power_status_change()` and C1's status mapping drops that state from
MQTT.

### C3 — MQTT-command → RSwitch actuation
**Producer** `/mqtt` → **consumer** `/power`. `mqtt_received_callback` decodes C1
and calls the RSwitch public API:

```c
action '1' → rSwitch[idx]->setSwitch(true)
action '0' → rSwitch[idx]->setSwitch(false)
action '3' → rSwitch[idx]->longPress()        // force / long-press
```
Behavioral, not just signature: `setSwitch(true)` **no-ops if already ON** (guards
on `isPowerOff() || isSleep() || isUndefined()`), and the SLEEP→ON path spoofs
`stat_prev=HIGH` + back-dates `verify_time_prev` to avoid the sleep-wake mis-read.
Don't "simplify" the guards. **Break risk:** changing these semantics changes what
an HA/web command does.

### C4 — Connection-health events
**Producer** `/net` (`Test`) → **consumers** `/mqtt`, `/power` (via `main.cpp`).

```c
void (*change_event_callback)(uint8_t event);  // EVENT_WIFI/MQTT_RESTORED|LOST (1..4)
void (*keepalive_event_callback)();            // publishes home/keepalive/rempowercntl1
// main handles ONLY EVENT_MQTT_RESTORED → send_info + keepalive + refresh_all_switches_state
```
**Break risk:** removing/reordering the `EVENT_MQTT_RESTORED` handling silently
regresses HA state recovery after a broker restart (the retained-resend backlog
item lives here).

### C5 — RSwitch public API consumed by the web UI
**Producer** `/power` → **consumer** `/web`. `WebSServer::begin(PGM_P version,
RSwitch **rSwitch)` receives the shared array and calls the public surface:

```c
rSwitch[i]->setSwitch(bool);                  // /setswitch?status=0|1
rSwitch[i]->longPress();                      // /setswitch?status=3
POWER_STATUS rSwitch[i]->getPowerStatus();    // /getstatus
char*        rSwitch[i]->decodePowerStat(s);  // /getstatus "stat" string ("DISABLED" if null)
```
Web index is **0-based** (`switch=0..2`); MQTT (C1) is **1-based**. Keep distinct.
**Break risk:** changing the public method set or `decodePowerStat` return strings
changes the JSON the buttons page parses. `RSwitch**` lifetime is owned by
`main.cpp`/`firmware`.

### C6 — `main.cpp` shared-file ownership
`main.cpp` houses three concerns behind in-file banners (present in the code):

| Section | Owner | Contents (verified in code) |
|---|---|---|
| Includes, globals, `setup()`, `loop()`, `send_info()`, SNTP `settime_cb`, debug wiring, `restart`, `test_change_event_callback`, `keepAlive_event_callback` | `/firmware` | integration shell + init order |
| Topic `#define`s, `topics[]`, `mqtt_received_callback`, `send_info` field set | `/mqtt` | C1/C3 producer code |
| `power_status_change()`, `power_status_change_callback()`, `refresh_all_switches_state()` | `/power` | C2/C4 consumer code |

Banners are live (`// === /mqtt section … ===`, `// === /power section … ===`).
Edit only inside your owned region; cross-boundary changes route through `/pm`.
`Log` and `SerialDebug` are stable `/firmware` utility APIs, not contracts.

---

## 6. Dual-MCU (ESP8266 / ESP32) divergence map

All divergence is behind `#if defined(ESP8266)/ESP32` in shared files — never a
forked tree. Both branches must stay in sync (parity audited by `/review`).

| Concern | ESP8266 | ESP32 | File |
|---|---|---|---|
| **Pin map** (SWITCH1/2/3) | D1, D8, D2 | 22, 5, 21 | `Application.h` |
| **Pin map** (DETECT1/2/3) | D7, D6, D5 | 23, 19, 18 | `Application.h` |
| **Status LED / BUTTON** | D4 / D0 | 16 / 26 | `Application.h` |
| **SNTP** | `settimeofday_cb` + `configTime(TZ_Asia_Jerusalem, …)` | `sntp_set_time_sync_notification_cb` + `configTime(7200,3600,…)` | `main.cpp` |
| **WiFi include** | `ESP8266WiFi.h` | `WiFi.h` + `esp_wifi.h` | `WiFiConnect.cpp`, `MqttClient.cpp`, `Test.cpp` |
| **mDNS include** | `ESP8266mDNS.h` | `ESPmDNS.h` | `WiFiConnect.cpp`, `OTA.cpp` |
| **Web server type** | `ESP8266WebServer` (:80) | `WebServer` (:80) | `WebSServer.{h,cpp}` |
| **PHY mode get/set** | `WiFi.getPhyMode()` / `setPhyMode()` | `esp_wifi_get/set_protocol()` bitmaps | `WiFiConnect.cpp` |
| **PHY LR (long range)** | not supported | `WIFI_PROTOCOL_LR` (+ `wifi_set_lr` debug cmd) | `WiFiConnect.cpp`, `main.cpp` |
| **Hostname timing** | set **after** `WiFi.begin()` (in `initial_connection_setup`) | set **before** `WiFi.begin()` | `WiFiConnect.cpp` |
| **WiFi sleep disable** | `setSleepMode(WIFI_NONE_SLEEP)` | `setSleep(WIFI_PS_NONE)` | `WiFiConnect.cpp` |
| **Heap diagnostics** | `getHeapFragmentation`, `getMaxFreeBlockSize`, `getCoreVersion`, `getFlashChipRealSize` | (omitted) | `main.cpp` `send_info`/`main_debug_info` |
| **WiFi event handlers** | `onStationMode*` lambdas registered | none | `WiFiConnect.cpp` |
| **`mc` field** | `"ESP8266"` | `"ESP32"` | `main.cpp` `send_info` |

PHY escalation policy (both MCUs): start 11N; `reconnect()` steps 11N→11G after
3+1 failed attempts, 11G→11N after 2+1; an hourly timer
(`TRY_PHY_N_INTERVAL = 3 600 000 ms`) tries to return to 11N. `RECONNECT_INTERVAL
= 60 000 ms`.

---

## 7. Where the three invariants are enforced

### 7.1 Secrets invariant
No credential literal appears in `src/`. WiFi/MQTT/OTA secrets flow:
`sysenv.SMARTHOME_*` → `platformio.ini` `build_flags` → `-D _WIFI_SSID_`,
`_WIFI_PWD_`, `_MQTT_BROKER_`, `_OTA_PASSWD_` → consumed in `main.cpp`
(`SSID`, `PWD`, `OTA_PASSWD`, `mqtt_server`). `build_flags_home` /
`build_flags_office` select the network. Identity (`_HOST_NAME_`, `_AREA_`) is
per-env, not secret. Canonical owner `/firmware` (`BUILD.md`); audited `/review`.

### 7.2 Boot-safe-GPIO invariant
RSwitch pins are configured `OUTPUT` and driven LOW except during a pulse, so a
relay must not fire through reset. Enforcement is **by pin choice**, in
`Application.h`. The source comment (`main.cpp` header + `Application.h`) documents
D1/GPIO5 and D2/GPIO4 as high-impedance through reset — but the map also assigns
**SWITCH2 = D8 (GPIO15)** on ESP8266 and **SWITCH2 = GPIO5** on ESP32, both of
which are boot **strapping** pins. See [§9](#9-observations-for-review). Electrical
rationale owned by `/power` (`HARDWARE.md`) + pin numbers by `/firmware`.

### 7.3 Non-blocking-loop invariant
Audited by `/review` (canonical). The only blocking on a `loop()` path is the
grandfathered relay pulse: `RSwitch::short_push()` `delay(800)` and
`long_push()` `delay(10000)`. All other `handleClient()` bodies are time-sliced
(millis-gated). The blocking `wifiConnect.begin()` and `delay(2000)` are in
`setup()`, outside the invariant's scope.

---

## 8. Cross-cutting notes

- **`info` JSON size:** `send_info()` builds a `DynamicJsonDocument(1024)` and
  serializes into a 1024-byte `BUF`, against `MQTT_MAX_PACKET_SIZE = 1024`. The
  `connection_info=true` variant adds ~10 fields and approaches the cap; an
  oversized `publish()` silently returns `false` (the code logs a WARN with the
  computed length). Field adds are a C1 concern.
- **Shared `BUF` aliasing:** `power_status_change()` reuses the global 1024-byte
  `BUF` for *both* the status string and the topic, placing the topic at
  `BUF + strlen(BUF) + 1`. Correct but fragile — a longer status string narrows
  the topic space.
- **`Test` couples to peers by design:** it is the only module that reads
  `wifiConnect`/`mqttClient` state directly, because it is the health probe.
- **MQTT connect detection:** `MqttClient` exposes `setConnectEvent()` but
  `main.cpp` does **not** use it; restored/lost detection is entirely `Test`'s
  polling job. This is intentional but means MqttClient's own event path is dead
  code in this build.
- **BUTTON_GPIO:** read in `setup()` and every ~1 s in `loop()` for a Serial
  print only — no actuation is wired to the physical button yet.

---

## 9. Observations for /review

Reconstruction surfaced the following. These are **architecture-level flags**, not
a substitute for the domain specialists' deep audits — confirm severity with the
owning specialist before acting.

1. **`RSwitch::longPress()` dead branch (likely bug, `/power`).**
   `RSwitch.cpp` ~302–316 tests `if (isPowerOff()) {POWERING_ON} else if
   (isPowerOff()) {POWERING_OFF} else {UNDEFINED}`. The second condition is
   identical to the first and is unreachable, so a long-press while powered ON
   sets `UNDEFINED`, never `POWERING_OFF`. The second branch was almost certainly
   meant to be `isPowerOn()`. Affects C3 ('3'/force) semantics.

2. **`OTA.cpp` malformed `#ifdef` (latent, `/net`).**
   Lines 13/16/67 use `#ifdef ESP8266 && !defined(NO_GLOBAL_INSTANCES) && …`.
   `#ifdef` takes a single identifier; the `&&` tail is stray tokens (ignored,
   possibly warned). It happens to work because the guard reduces to plain
   `#ifdef ESP8266`/`ESP32`, but the intent (skip when mDNS globals are disabled)
   is silently not honored. Should be `#if defined(ESP8266) && !defined(...)`.

3. **Boot-strapping pins used as relay outputs (boot-safe-GPIO, `/power` + `/firmware`).**
   ESP8266 `SWITCH2 = D8/GPIO15` and ESP32 `SWITCH2 = GPIO5` are strapping pins.
   The code's own header comment recommends only D1/D2 (GPIO5/GPIO4) as
   boot-safe. GPIO15 must be LOW at boot; an active-high relay board holding it
   high would block boot. Worth an explicit boot-safety confirmation in
   `HARDWARE.md` (the map is "known" in `CLAUDE.md`, but the rationale for D8/GPIO5
   being acceptable is undocumented).

4. **`power/get` is a no-op (`/mqtt`).** Inbound `STR_POWER_GET` is matched but its
   handler body is commented out (`send_status()`), so a `power/get` request
   returns nothing. If HA relies on it for on-demand refresh, it is silently
   ignored (the retained `state/`/`status/` topics cover steady state).

5. **`POWERING_*` states never clear `state/` (`/mqtt` + `/power`, by design?).**
   `power_status_change()` suppresses the `state/` publish for `POWERING_ON/OFF`
   (commented out), so a consumer keyed on `state/` sees stale 0/1 during the
   transition window. Confirm this is intended vs. an optimistic-update gap.

6. **Two dormant detection algorithms (`/power`).** `detect_power_status()`
   delegates to `detect_power_status_old()` (active); `detect_power_status_new()`
   exists but is unreferenced. Per `/power` rules, edit the `_old()` path. Flagged
   so `/review` knows `_new()` is intentional dead code, not an orphan.

7. **`send_status()` forward declaration is dead (`/firmware`).** Declared at
   `main.cpp` ~91 but only defined inside a commented block; referenced only in
   commented code. Harmless, removable.

---

## 10. Doc set produced / matrix status

- **Refreshed:** `doc/design/ARCHITECTURE.md` (this file) — was an Abstract-only
  stub; body reconstructed from `src/`.
- **No new docs added**, so `DOC_OWNERSHIP_MATRIX.md` needs no new row
  (`ARCHITECTURE.md` is already listed, Primary `/arch`).
- Specialist-owned design docs (`POWER_STATE.md`, `HARDWARE.md`, `MQTT_API.md`,
  `BUILD.md`) were intentionally **not** edited here — they belong to the
  concurrent `/power`, `/mqtt`, `/firmware` domain audits. Cross-references from
  this doc point at them as canonical for their domains.

---

## 11. Cross-cutting failure patterns

This section is the single canonical home for project-wide failure *classes* that
recur across domains. Per-domain `GUIDELINES.md` files keep a short local pointer
("see `ARCHITECTURE.md` → Cross-cutting failure patterns") and the per-instance
fix; the pattern, policy, and prevention live here so they are stated once.

> **Source:** `/review` canonical audit `doc/audit/2026-06-06-audit.md` §4
> (theme **VALIDATED**) + the cross-cutting recommendation block in
> `doc/audit/2026-06-06-guidelines-drafts.md`. Severities/IDs trace to that audit.

### 11.1 Pattern — "Silently-ignored configuration"

**Definition.** Configuration or expressed intent — a preprocessor condition, a
function argument, a template placeholder, a documented hardware constraint — is
**silently dropped** rather than honored or rejected. There is **no compile error
and no warning**; the discarded intent only surfaces as wrong behavior at runtime
(or never, masking a latent defect). The common shape: a construct *looks* like it
carries the configuration, but the language/API quietly ignores the part that
matters, so the code reads as correct to a human and to the compiler.

This is the project's dominant systemic theme — four structurally-different bugs
across four domains share this single root cause, which is why it gets a
pattern-level policy rather than four independent point fixes.

### 11.2 Confirmed instances

| # | Domain | Construct | What is silently dropped | Ref |
|---|---|---|---|---|
| 1 | `/net` · `/firmware` | `#ifdef ESP8266 && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)` | `#ifdef` consumes **one** identifier; the `&& …` tail is stray tokens. Guard reduces to plain `#ifdef ESP8266`/`ESP32` — the `NO_GLOBAL_MDNS` opt-out intent is never honored. | `OTA.cpp:13,16,67`; sibling in `MdnsClient.cpp`; net C1 / firmware C7 |
| 2 | `/mqtt` | `begin(server, id, boolean enabled)` overload | Delegates to the 4-arg `begin()` which unconditionally sets `_enabled = true`, so a caller passing `enabled=false` is overridden. The explicit flag is discarded. | `MqttClient.cpp:95`; mqtt M-1 |
| 3 | `/net` | `begin(ssid, pwd, hostname, boolean mdns_enabled)` overload | Forwards a hard-coded `false` for `mdns_enabled` instead of the caller's argument — the parameter is accepted then thrown away. | `WiFiConnect.cpp` (`begin(…,mdns_enabled)`); net M2 |
| 4 | `/web` | `page.replace("device1", "DEVICE1")` | Substring match on `{device1}` rewrites only the inner text (→ `{DEVICE1}`, braces kept) and never touches `{device2}`/`{device3}`. The full delimited-token substitution intent is silently no-op. | `WebSServer.cpp:42`; web C-W1 |
| — | `/power` *(logic-level analogue)* | `if (isPowerOff()) … else if (isPowerOff()) …` | Same class one layer up: a **duplicated condition** makes the `POWERING_OFF` branch unreachable, so force-off intent silently falls through to `UNDEFINED`. Not a config drop, but the same "expressed intent quietly discarded, no diagnostic" failure mode. Also BUG-10 (`detect_power_status_new()` dormant + divergent). | BUG-01 `RSwitch.cpp:308`; BUG-10 `RSwitch.cpp:133-221` |

A related variant at the *documentation* layer: hardware intent that is stated but
contradicted by the code — the boot-safe comment (`Application.h:29`, "use D1/D2")
versus the strapping-pin assignments `POWER_SWITCH2_GPIO = 5` (ESP32, F-01) and
`D8`/GPIO15 (ESP8266, F-02). The intent is documented but silently not enforced by
the pin map.

### 11.3 Policy

**Configuration must be honored or explicitly rejected — never silently
discarded.** Concretely:

- A conditional-compilation guard must evaluate **every** condition it appears to
  test, or not pretend to test it.
- A function parameter must be **used**, or removed from the signature. An
  overload that accepts a flag and ignores it is a defect, not a convenience.
- A substitution/templating step must act on the **complete** token it claims to
  replace, or fail loudly.
- Documented hardware/runtime constraints must be enforced by the code (pin map,
  asserts, build flags), not left as comments the implementation can drift from.

When configuration genuinely cannot be honored, the code must **reject it
loudly** (compile error, early `return`, logged error) rather than proceed on a
silently-substituted default.

### 11.4 Prevention — recommended CI / build guards

*Design rationale only — `/pm` owns converting these into backlog action items.*

1. **Prefer `#if defined(X) && …` over `#ifdef X && …`.** `#if defined()` composes
   with `&&`/`!defined()`; `#ifdef` does not. Sweep `src/` for the `#ifdef X &&`
   antipattern and convert all sites.
2. **Enable compiler warnings that catch the silent drops.** Add `-Wundef`
   (flags the macro-name typos that this pattern hides behind) and consider
   `-Werror` (or targeted `-Werror=undef`) in `platformio.ini` `build_flags`, so a
   malformed guard fails the build instead of compiling to the wrong branch. Pair
   with `-Wall -Wextra` to surface unused-parameter cases (instance #2/#3).
3. **Audit every multi-arg `begin()` overload for full argument pass-through.**
   Each overload must forward all of its parameters to the canonical overload (or
   not declare a parameter it will not use). This is a one-time review of
   `MqttClient`, `WiFiConnect`, and any future module exposing layered `begin()`s.
4. **Use whole-token substitution for the web page** (`"{device1}"`, not
   `"device1"`) — or move to a templating helper that errors on an unsubstituted
   placeholder.

