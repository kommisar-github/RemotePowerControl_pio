## Section 7 — Interface contracts between specialists

This firmware is small in line count but **callback-rich**: cross-module state flows through C function pointers wired in `main.cpp`, plus one shared-pointer API (`RSwitch**`) and one shared file. These are the boundaries PM enforces — when a specialist proposes a change, PM checks it against the relevant contract before approving. Six real contracts; idiomatic form is C/C++ signatures (matching the codebase).

The dependency that makes these load-bearing: `main.cpp` wires producers to consumers, so a change to any signature below silently breaks the other side at the wiring point unless both specialists coordinate.

---

**C1 — MQTT topic + payload grammar** · producer `/mqtt` → consumers `/power`, `/web`, `/firmware`, Home Assistant
The on-the-wire contract. Defined as `#define`s in the `main.cpp` MQTT section; documented in `MQTT_API.md`.
```c
// Inbound (subscribed):
//   <area>/<host>/power/set   payload: "<action><sep><idx>"  action ∈ {'0','1','3'}, idx char ∈ {'1','2','3'}  (index = payload[2]-'1')
//   <area>/<host>/power/get   payload: <idx>
//   <area>/alive/get , <area>/<host>/info/get   payload: (ignored)
// Outbound (published):
//   <area>/<host>/power/state/<idx>   retained, payload "0"|"1"
//   <area>/<host>/power/status/<idx>  retained, payload <human string from decodePowerStat()>
//   <area>/<host>/info | /alive        payload <info JSON, DynamicJsonDocument(1024)>
//   <area>/announcement/<host>         "connected" (connect) / "disconnected" (LWT)
```
**Break risk:** renaming a topic or shifting the payload byte positions silently desyncs HA and the web JS. PM gates any topic/payload change as a C1 change touching `MQTT_API.md`.

---

**C2 — Power-status-change callback** · producer `/power` → consumer `/mqtt` (via `main.cpp` wiring, owned `/firmware`)
RSwitch reports state transitions outward; `main.cpp` maps them to retained MQTT publishes. Fired only on actual change.
```c
typedef void (*power_status_change_callback)(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status);
// registered: rSwitch[i]->setCallback(power_status_change_callback);
// main.cpp resolves src→index (0..2) and publishes state/<idx+1> + status/<idx+1>
```
**Break risk:** changing the signature, or adding a `POWER_STATUS` value without updating the publish `switch` in `main.cpp::power_status_change()`, drops that state from MQTT. New enum values must update C1's status mapping too.

---

**C3 — MQTT-command → RSwitch actuation** · producer `/mqtt` → consumer `/power`
The inbound control path: `mqtt_received_callback` (main.cpp MQTT section) decodes C1's set-payload and calls the public RSwitch API.
```c
// action '1' -> rSwitch[idx]->setSwitch(true)
// action '0' -> rSwitch[idx]->setSwitch(false)
// action '3' -> rSwitch[idx]->longPress()        // force / long-press
```
**Break risk:** `/power` changing `setSwitch`/`longPress` semantics (e.g. making `setSwitch(true)` from SLEEP a no-op) changes what an HA command does. Behavioral contract, not just signature — coordinate when guard logic changes.

---

**C4 — Connection-health events** · producer `/net` (`Test`) → consumers `/mqtt`, `/power` (via `main.cpp`)
`Test` watches WiFi+MQTT and fires lifecycle events; `main.cpp::test_change_event_callback` reacts.
```c
typedef void (*change_event_callback)(uint8_t event);   // EVENT_WIFI/MQTT_RESTORED|LOST (1..4)
// on EVENT_MQTT_RESTORED: send_info(STR_CONNECTION_INFO,true); keepAlive(); refresh_all_switches_state();
typedef void (*keepalive_event_callback)();             // publishes <area>/keepalive/<host>
```
**Break risk:** the open backlog item "resend all switch state on MQTT reconnect" lives exactly here — `refresh_all_switches_state()` must re-publish all 3 retained on `EVENT_MQTT_RESTORED`. Removing or reordering the event handlers silently regresses HA state recovery.

---

**C5 — RSwitch public API consumed by the web UI** · producer `/power` → consumer `/web`
`WebSServer::begin(version, RSwitch **rSwitch)` receives the shared channel array and calls the public surface directly (no MQTT in between).
```c
rSwitch[i]->setSwitch(bool);                 // /setswitch?status=0|1
rSwitch[i]->longPress();                     // /setswitch?status=3
POWER_STATUS rSwitch[i]->getPowerStatus();   // /getstatus
char*        rSwitch[i]->decodePowerStat(s); // /getstatus "stat" string ("DISABLED" if channel null)
```
Note: web switch index is **0-based** (`switch=0..2`); MQTT (C1) is 1-based. Keep them distinct.
**Break risk:** `/power` changing the public method set or `decodePowerStat` strings changes the web JSON the buttons page parses. The shared `RSwitch**` pointer is passed at `webSServer.begin()` — lifetime owned by `main.cpp`/`firmware`.

---

**C6 — `main.cpp` shared-file ownership boundary** · `/firmware` (shell) ∥ `/mqtt` (MQTT section) ∥ `/power` (power callbacks)
`main.cpp` is a single file housing three concerns that cannot practically be split. Ownership by section:

| Section | Owner | Contents |
|---|---|---|
| Includes, globals, `setup()`, `loop()`, `send_info()`, debug-command wiring, time/SNTP, `restart` | `/firmware` | The integration shell + init order |
| Topic `#define`s, `topics[]`, `mqtt_received_callback`, `send_info` field set | `/mqtt` | C1/C3 producer code |
| `power_status_change()`, `power_status_change_callback()`, `refresh_all_switches_state()` | `/power` | C2/C4 consumer code |

**Default: insert in-file section banners** during bootstrap (per `BOOTSTRAP_PROMPT.md` §7 default, flipped to insert in v0.3.18):
```c
// === /mqtt section — topic schema + inbound callback — owner: /mqtt ===
// ... topic #defines, mqtt_received_callback ...
// === end /mqtt section ===
```
**Dispatch-time coordination:** when PM dispatches a `main.cpp` task it cites the owned region in the payload (e.g. `Owns: main.cpp MQTT section`); the specialist re-confirms against the banner before editing and surfaces any mismatch rather than silently editing across the boundary. Line-range drift is caught by `/review` in its normal pass, not by a file lock (the task router does not mediate file access). **Gated on §12 Q-MARKERS** — if the user declines banners, ownership lives in this table + the matrix only.

---

**Why not more contracts.** `Log` (`LOG(...)`) and `SerialDebug` (`addCommandHandler`) are used by every module but are **stable utility APIs owned by `/firmware`**, not cross-specialist negotiation points — a layout convention, documented in `ARCHITECTURE.md`, not split into per-caller contracts. The `info` JSON field set is part of C1 (it's an MQTT payload), not a seventh contract. Padding these into thin contracts would add noise without fixing a real silent-break risk.
