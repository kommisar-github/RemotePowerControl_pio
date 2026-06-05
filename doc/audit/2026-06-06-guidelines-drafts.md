# Wave 3 GUIDELINES Drafts — handoff to /review (audit gate)

**Date:** 2026-06-06
**Status:** DRAFTS — NOT committed. `/review` audits each delta; PM commits approved ones to `doc/<agent>_GUIDELINES.md` (serialized).
**Provenance:** `doc/audit/2026-06-06-audit.md` (canonical) + `doc/audit/2026-06-06-wave1-raw-findings.md`.

Authorship note:
- power / mqtt / net / firmware drafts = authored by the respective specialist (returned via task-router).
- **web draft = authored by PM** (the `/web` terminal returned empty twice; content fully determined by the audit). Flagged for extra scrutiny by `/review`.

All target GUIDELINES files exist with placeholder-only bodies (no existing content to conflict with).

---

# 1. /power → doc/power_GUIDELINES.md  (specialist-authored)

## Conventions
- **C-P1 longPress() force-off dead-condition trap (BUG-01, CRITICAL/P0).** The 2nd branch of `longPress()` MUST use `isPowerOn()`/`!isPowerOff()`, never a repeat of `isPowerOff()`. Duplicate condition makes `POWERING_OFF` unreachable → non-OFF states fall to `else` → `set_power_status(UNDEFINED)`; HA sees UNDEFINED on force-off while relay still fires. Verify the two branch conditions are mutually exclusive before editing. (C2; `RSwitch.cpp:308`)
- **C-P2 setSwitch(false) double-pulse guard — `isPowerOn()` only (BUG-03, C3).** OFF guard must be `isPowerOn()` alone, NOT `isPowerOn() || isPoweringOff()`; the latter lets a 2nd `set=0` call `short_push()` while the relay is already cycling, possibly toggling the device back on. Mirror the ON-direction guard (excludes POWERING_ON). (`RSwitch.cpp:347`)
- **C-P3 Stamp on_time/off_time in set_power_status() (BUG-02).** Set `on_time=millis()` on transition TO POWER_ON and `off_time=millis()` on transition TO POWER_OFF. Both init 0 and are otherwise never set; unset → `getOnDurationSec()/getOffDurationSec()` return seconds-since-boot, corrupting info JSON telemetry. (C2; `RSwitch.cpp:45-55`, `RSwitch.h:50-51`)
- **C-P4 Suppress periodic 5-min validation during SLEEP (BUG-07).** The periodic `use_stat=true` branch in `detect_power_status_old()` must not fire when `power_status==SLEEP`; mid-cycle GPIO read can spuriously set POWER_ON/OFF. Add `&& power_status != SLEEP`. (`RSwitch.cpp:269`)
- **C-P5 Always edit detect_power_status_old(); _new() is dormant+divergent (BUG-10).** `_new()` is dead code (unreferenced; corroborated by /arch) and uses a different debounce model (`state_change_time` vs `verify_time_prev`). Promotion requires a deliberate reviewed switch with behavioral comparison. (`RSwitch.cpp:133-221`)

## Decisions
- **D-1 Active detect impl = detect_power_status_old() (2026-06-06).** `detect_power_status()` delegates to `_old()`. Must not be silently swapped; promotion routes through /review.
- **D-2 longPress() sequencing intent: relay before callback (2026-06-06).** Intent is `long_push()` then `set_power_status()` so callback fires after relay completes. Current code inverts this (BUG-04); benign while `long_push()` blocks 10 s, but MUST be corrected when relay pulses go non-blocking (backlog).

## Open Questions
- **OQ-1 Wake-from-SLEEP back-date (BUG-06).** Back-date is 3 s (`LONG_PULSE_DURATION_DETECT_SEC`) but confirm window is 6 s (`VALIDATE_LONG_PULSE_SEC`). 6 s (conservative) or 3 s? Depends on real sleep-pulsing cadence; validate on hardware. (`RSwitch.cpp:336`)
- **OQ-2 Duration-getter sentinel during transitional states (BUG-11).** Getters return 0 in POWERING_ON/OFF/SLEEP. Is 0 the intended "n/a" sentinel or should it be distinct (e.g. ULONG_MAX)? Clarify C5 with /web; moot until C-P3 lands.

(Power-domain note: systemic "silently-ignored-config" surfaces as BUG-01 dead `else if` and BUG-10 dormant divergence; encoded in C-P1/C-P5. Pattern-level policy belongs to a cross-cutting doc owned by /arch or /pm.)

---

# 2. /mqtt → doc/mqtt_GUIDELINES.md  (specialist-authored)

## Conventions
- **Inbound callback: length-guard before indexing payload (C-1, P0).** `mqtt_received_callback` and any inbound handler MUST `if (length < N) return;` before the first `payload[N-1]` access. PubSubClient doesn't null-terminate or enforce min length; short/crafted msg → stale bytes → garbage `switch_idx` that may pass `(-1<idx<3)` and actuate wrong relay. Fix: `if (length < 3) return;` atop STR_POWER_SET. (C1/C3; `main.cpp:499`)
- **info JSON: both DynamicJsonDocument pool AND MQTT frame must fit 1024 B (M-2+M-3, P1).** (1) ArduinoJson 6 silently drops fields when the pool (`DynamicJsonDocument(1024)`, `main.cpp:87`) is exhausted (~35 fields likely overflow). (2) MQTT frame `5+2+strlen(topic)+strlen(payload)` (~1054 B) exceeds `MQTT_MAX_PACKET_SIZE=1024`; `publish()` returns false but caller ignores it → HA gets no telemetry silently. Fix: raise doc+BUF to 2048 or split connection_info into a 2nd publish; check publish() return. (Cross-corroborated mqtt M-2/M-3 + firmware F-07.)
- **begin() enabled-flag overload silent-drop (M-1).** `begin(server,id,bool enabled)` delegates to 4-arg which unconditionally sets `_enabled=true` (`MqttClient.cpp:95`), overriding `false`. Until fixed always pass `true`; don't use the boolean overload to defer init. Instance of systemic "silently-ignored-config". Fix: remove the unconditional `_enabled=true`.

## Decisions
- **STR_POWER_GET handler intentionally no-op (pending) (L-4).** `power/get` subscribed but body commented (`main.cpp:512-513`); retained `state/`+`status/` cover steady state. Implement-vs-deprecate is an open decision (OQ).
- **Legacy "inTopic" subscription dead, scheduled for removal (L-2).** `resubscribe()` subscribes `"inTopic"` each reconnect (`MqttClient.cpp:235`); no handler matches. Grouped with dead-code cleanup (P2).

## Open Questions
- **Q1 Should power/get trigger on-demand refresh?** (a) call `refresh_all_switches_state()` for HA polling, or (b) remove subscription. Retained topics give eventual consistency; (a) only if HA needs synchronous confirmation. Defer to fix phase.
- **Q2 Should LWT/announcement "connected" be retained?** Both announcement and LWT currently non-retained; HA restart won't see "connected" until next keepalive (60 s). Documented intentional (MQTT_API.md) but may degrade HA availability UX.

---

# 3. /net → doc/net_GUIDELINES.md  (specialist-authored)

## Conventions
- **C1 Use `#if defined(X) && cond`, never `#ifdef X && cond` (H1, P1).** `#ifdef` takes one identifier; trailing `&&`/`!defined(...)` are silently ignored (warning only). `OTA.cpp:13/16/67` + `MdnsClient.cpp` drop their `NO_GLOBAL_*` guards. WiFiConnect.cpp already correct. Lead instance of systemic "silently-ignored-config".
- **C2 WiFi-lost path MUST also reset mqtt_conn_timestamp (H2, C4, P1).** When `EVENT_WIFI_LOST` fires, the MQTT branch is unreachable while WiFi down; stale `mqtt_conn_timestamp>0` → `EVENT_MQTT_LOST` fires after `EVENT_WIFI_RESTORED` (wrong order) and `EVENT_MQTT_RESTORED` may be suppressed entirely. That event drives `refresh_all_switches_state()`+re-announce → HA stale on miss. Rule: clear `mqtt_conn_timestamp` atomically in the WiFi-lost branch (fire `EVENT_MQTT_LOST` if it was >0). (`Test.cpp:~190`)
- **C3 All begin() overload args must be forwarded, never silently dropped (M2 + mqtt M-1).** `WiFiConnect::begin(ssid,pwd,hostname,mdns_enabled)` passes hardcoded `false` for `mdns_enabled`. Every overload must forward all params to the canonical overload, or not accept the param. Constant-where-a-param-belongs is not acceptable delegation.
- **C4 No begin()/pinMode()/digitalWrite() in global ctors (L8).** Global object ctors run at static-init before `setup()`/GPIO init; hardware calls may no-op or produce undefined state. Ctors init plain data only; hardware init belongs in `setup()`. (`Test.cpp:45-48`)

## Decisions
- **D1 Test event-ordering guarantees under WiFi drop/restore (C4) (2026-06-06).** After H2 fix: drop-together → `EVENT_MQTT_LOST` then `EVENT_WIFI_LOST` (same tick); restore → `EVENT_WIFI_RESTORED` then `EVENT_MQTT_RESTORED`; MQTT-only drop → `EVENT_MQTT_LOST`. Any change to order/suppression is a C4 contract change → surface to /pm.

## Open Questions
- **OQ1 Hardcoded network ID 192.168.66.0 in bad-DHCP detection (L7).** `Test.cpp:181` forces reconnect on exactly `192.168.66.0` (+ `!isIPAddressSet(gatewayIP)`). Options: (a) build-flag `-D BAD_DHCP_NETWORK_ID=...`, (b) remove and rely on gateway check, (c) keep (single-deployment). Recommend (b) unless portability needed. P2.

---

# 4. /firmware → doc/firmware_GUIDELINES.md  (specialist-authored)

## Conventions
- **C1 Cross-MCU strapping-pin rule for relay-output pins (F-01, CRITICAL/invariant).** A shared GPIO number is NOT equivalent across MCUs: GPIO5 is boot-safe D1 on ESP8266 but a strapping pin (HIGH at boot) on ESP32. Relay drive is active-HIGH (`RSwitch.cpp:114,122`, ctor sets OUTPUT no initial LOW) → strapping-HIGH ESP32 pin = momentary host power-button press on every reset/flash. Rule: on any `Application.h` pin change, validate every switch output against strapping lists for BOTH MCUs (ESP8266: GPIO0/2/15; ESP32: GPIO0/2/5/12). ESP8266 passing ≠ ESP32 certified.
- **C2 Secrets invariant — verified CLEAN baseline (2026-06-06).** All creds flow `sysenv.SMARTHOME_*` → `platformio.ini -D` → `main.cpp`, no source literals, no fallback `#define`. Future literal/fallback in `src/` violates the invariant. Git-history scan recommended, not yet done.
- **C3 Log macro name + const-qualifier must match Log.h/Log.cpp.** Mismatch passes the compiler (undefined overload) until called → link error, no compile warning. Confirmed: `LOG_DDEBUG`(h)/`LOG_DEBUG`(impl); `LOG_ERROR(char*,...)`(h)/`LOG_ERROR(const char*,...)`(impl). (F-03/F-04, P1)
- **C4 `#include` filename case must match real filename.** `Application.h:13` `#include "log.h"` but file is `Log.h`; builds on Windows, breaks Linux CI. (F-13)
- **C5 Vars written in SNTP/timer callbacks must be `volatile`.** `boottime`,`timeset_cnt` (`main.cpp:50-51`) set in SNTP callback, read on main loop; without `volatile` reads may be stale. (F-05)
- **C6 ESP32 SNTP must use POSIX TZ string, not numeric offsets.** ESP8266 uses `TZ_Asia_Jerusalem`; ESP32 uses numeric `gmtOffset/daylightOffset` which don't self-adjust DST reliably. Migrate ESP32 to `setenv("TZ","IST-2IDT,M3.4.4/26,M10.5.0")`+`tzset()` after `configTime()`. (F-06)
- **C7 Use `#if defined(X)` not `#ifdef X` in compound guards.** (mirrors net C1; `OTA.cpp:13/16/67`). Instance of systemic "silently-ignored-config".

## Decisions
- **D1 Relay drive is active-HIGH (confirmed 2026-06-06).** `RSwitch.cpp:114,122` drive HIGH; ctor sets OUTPUT without initial LOW. Load-bearing for boot-safe-GPIO analysis: any switch GPIO high during boot window briefly actuates the relay.
- **D2 ESP8266 POWER_SWITCH2=D8/GPIO15: boot-SAFE under active-HIGH; comment misleading (F-02, downgraded MED).** GPIO15 has external pulldown, held LOW at boot → doesn't fire. `Application.h:29` comment omits D8; correct the comment, confirm polarity with /power before relay-board changes.
- **D3 ESP32 POWER_SWITCH2=GPIO5: boot-safe FAIL, must migrate (F-01, P0).** GPIO5 internal pull-up, HIGH at boot before setup() → momentary press on every ESP32 reset/flash. Candidate replacements GPIO13/14/27 (non-strapping; verify traces + polarity with /power).

## Open Questions
- **Q1 ESP32 SWITCH2 replacement pin (blocks F-01).** GPIO13/14/27 candidates; selection depends on wemos_d1_mini32 trace availability, no JTAG/SPI/PSRAM conflict, and /power confirming reset state is HIGH-Z or LOW (not HIGH).
- **Q2 git-history secret scan.** Source clean; history not scanned. Recommend a one-time scan before/after public exposure. (NOTE: repo already pushed public at v1.0.0.)

---

# 5. /web → doc/web_GUIDELINES.md  (PM-AUTHORED — /web returned empty twice; verify against audit)

## Conventions
- **C-W1 PROGMEM placeholder substitution must replace the FULL `{token}`, not a bare substring (C1, MEDIUM/P2 — DOWNGRADED).** `WebSServer.cpp:42` `page.replace("device1","DEVICE1")` matches the inner substring, turning `{device1}`→`{DEVICE1}` (braces retained) and never touching `{device2/3}`. Rule: always replace the complete delimited token (`"{device1}"`). NOTE per /review: the 2 s JS `/getstatus` poll self-heals `{stat1..3}` and controls use hardcoded indices, so real impact is **cosmetic device-header labels only** — do not overstate as critical.
- **C-W2 /getstatus JsonDocument must fit all nodes or it silently truncates (M1).** `DynamicJsonDocument(256)` (`WebSServer.cpp:101`) is borderline for the root+array+3×3-field payload; pool overflow → empty/truncated doc → client JS `JSON.parse` crash. Size the document to the ArduinoJson-assistant capacity (≈384 B) and keep `BUF` adequate.
- **C-W3 Header-scope const PROGMEM must be `static` (or extern+def in .cpp) (M2, latent linker hazard).** `strings1.h:4-7` declares `TAG_STAT/TAG_VER/PAGE_BUTTONS_OLD/PAGE_BUTTONS` as non-static `const char* PROGMEM` at file scope; a second include → multiple-definition linker error. Mark `static` or move defs to `WebSServer.cpp` with `extern` decls.
- **C-W4 /getstatus should send `application/json`, not `text/plain` (L1).** `WebSServer.cpp:126` uses `text/plain`; JS works via `JSON.parse` but header consumers (curl -i, HA REST sensors) break. Use `application/json`.

## Decisions
- **D-W1 Web index is 0-based (confirmed 2026-06-06).** `setswitch?switch=0..2` is 0-based and correct; distinct from MQTT's 1-based payload (C1). ESP8266WebServer/WebServer `#ifdef` branches verified at parity. (Do not unify the two index bases.)
- **D-W2 PAGE_BUTTONS_OLD is dead, scheduled for removal (M3).** Unreferenced; its JS uses `,` instead of `&` between query params (`/setswitch?switch=e,status=x`) → unparseable; ~2 KB flash. Remove with the dead-code cleanup (P2).

## Open Questions
- **Q-W1 Force-power checkbox state vs the 2 s poll (L3).** `/getstatus` poll sets `sw*f.checked = devs[].on`, overwriting the user's force-off feedback during the 10 s `long_push`. Should force checkboxes be (a) not synced from `/getstatus`, or (b) shown disabled/indeterminate during `POWERING_OFF`? (b) needs a `POWERING_OFF` signal — coordinate with /power (C5). Defer.

---

## Cross-cutting recommendation (for /review verdict, not a per-domain entry)
The systemic **"silently-ignored-configuration"** pattern recurs across domains: malformed `#ifdef && ...` guards (net C1 / firmware C7), `begin()` overloads dropping args (mqtt M-1 / net C3), and the web substring-replace bug (web C-W1). `/review` to decide whether a pattern-level policy belongs in a cross-cutting doc owned by /arch or /pm (per /power's suggestion), rather than duplicated in each GUIDELINES.
