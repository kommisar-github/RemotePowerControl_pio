# /firmware — Agent Guidelines
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Durable, project-specific guidelines for the `/firmware` agent (Build, platform, pin maps, integration shell). This file is the agent's **only** sanctioned write target for notes that should survive across sessions. The agent appends here **only when PM or the user explicitly asks** (review-gated consolidation) — never as a side effect of doing work. Project-specific build/secrets/pin/version-bump conventions that should outlive a single phase land here on request.

**Load when:** the `/firmware` agent starts a session, or when PM is auditing roster consistency via `/pm audit`.

**Key facts:**
- Owner: `/firmware` (Primary), `/pm` (Secondary) — per DOC_OWNERSHIP_MATRIX.md
- Write trigger: explicit PM/user request only, routed through the `/review` consolidation gate. Cite the request verbatim in the commit message.
- All other auto-memory writes by specialists are forbidden (see SKILL.md `## Memory Policy`).
- Canonical owner of the secrets invariant, the boot-safe pin map, and the version-bump protocol.

**Owner:** `/firmware` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `.claude/skills/firmware/SKILL.md`, `DOC_OWNERSHIP_MATRIX.md`, `BUILD.md`, `HARDWARE.md`

---

_Populated 2026-06-06. Source: `doc/audit/2026-06-06-audit.md` (Wave 2 canonical) + `doc/audit/2026-06-06-wave1-raw-findings.md`. Verdict: `/review` APPROVE-WITH-EDITS (edits 4a strapping-pin list + 4b include-case scope applied before write). Systemic "silently-ignored-configuration" theme — see `doc/design/ARCHITECTURE.md` → Cross-cutting failure patterns._

---

## Conventions

- **C1 Cross-MCU strapping-pin rule for relay-output pins (F-01, CRITICAL/invariant).** A shared GPIO number is NOT equivalent across MCUs: GPIO5 is boot-safe D1 on ESP8266 but a strapping pin (HIGH at boot) on ESP32. Relay drive is active-HIGH (`RSwitch.cpp:114,122`, ctor sets OUTPUT no initial LOW) → strapping-HIGH ESP32 pin = momentary host power-button press on every reset/flash. Rule: on any `Application.h` pin change, validate every switch output against strapping lists for BOTH MCUs (ESP8266: GPIO0/2/15; ESP32: GPIO0/2/5/12/15). ESP8266 passing ≠ ESP32 certified.

- **C2 Secrets invariant — verified CLEAN baseline (2026-06-06).** All creds flow `sysenv.SMARTHOME_*` → `platformio.ini -D` → `main.cpp`, no source literals, no fallback `#define`. Future literal/fallback in `src/` violates the invariant. Git-history scan recommended, not yet done.

- **C3 Log macro name + const-qualifier must match Log.h/Log.cpp.** Mismatch passes the compiler (undefined overload) until called → link error, no compile warning. Confirmed: `LOG_DDEBUG`(h)/`LOG_DEBUG`(impl); `LOG_ERROR(char*,...)`(h)/`LOG_ERROR(const char*,...)`(impl). (F-03/F-04, P1)

- **C4 `#include` filename case must match real filename.** (file is `Log.h`; all 9 src includes use lowercase `"log.h"` — `Application.h:13`, `Log.cpp:8`, `MdnsClient.cpp:19`, `MqttClient.cpp:28`, `OTA.cpp:21`, `SerialDebug.cpp:9`, `Test.cpp:39`, `WebSServer.cpp:3`, `WiFiConnect.cpp:32`. Builds on Windows, breaks case-sensitive/Linux CI. Fix = rename file or all 9 includes. (F-13))

- **C5 Vars written in SNTP/timer callbacks must be `volatile`.** `boottime`, `timeset_cnt` (`main.cpp:50-51`) set in SNTP callback, read on main loop; without `volatile` reads may be stale. (F-05)

- **C6 ESP32 SNTP must use POSIX TZ string, not numeric offsets.** ESP8266 uses `TZ_Asia_Jerusalem`; ESP32 uses numeric `gmtOffset/daylightOffset` which don't self-adjust DST reliably. Migrate ESP32 to `setenv("TZ","IST-2IDT,M3.4.4/26,M10.5.0")`+`tzset()` after `configTime()`. (F-06)

- **C7 Use `#if defined(X)` not `#ifdef X` in compound guards.** (mirrors net C1; confirmed instances: `OTA.cpp:13/16/67`). `#ifdef` takes one identifier; trailing `&&`/`!defined(...)` are silently ignored. Instance of systemic "silently-ignored-config".

## Decisions

- **D1 Relay drive is active-HIGH (confirmed 2026-06-06).** `RSwitch.cpp:114,122` drive HIGH; ctor sets OUTPUT without initial LOW. Load-bearing for boot-safe-GPIO analysis: any switch GPIO high during the boot window briefly actuates the relay.

- **D2 ESP8266 POWER_SWITCH2=D8/GPIO15: boot-SAFE under active-HIGH; comment misleading (F-02, downgraded MED).** GPIO15 has external pulldown, held LOW at boot → doesn't fire. `Application.h:29` comment omits D8; correct the comment, confirm polarity with /power before relay-board changes.

- **D3 ESP32 POWER_SWITCH2=GPIO5: boot-safe FAIL, must migrate (F-01, P0).** GPIO5 internal pull-up, HIGH at boot before setup() → momentary press on every ESP32 reset/flash. Candidate replacements GPIO13/14/27 (non-strapping; verify traces + polarity with /power).

## Open Questions

- **Q1 ESP32 SWITCH2 replacement pin (blocks F-01).** GPIO13/14/27 candidates; selection depends on wemos_d1_mini32 trace availability, no JTAG/SPI/PSRAM conflict, and /power confirming reset state is HIGH-Z or LOW (not HIGH).

- **Q2 git-history secret scan.** Source clean; history not scanned. Recommend a one-time scan before/after public exposure. (NOTE: repo already pushed public at v1.0.0.)
