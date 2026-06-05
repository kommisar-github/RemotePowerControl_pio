# /net — Agent Guidelines
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Durable, project-specific guidelines for the `/net` agent (WiFi, OTA, mDNS, connection health). This file is the agent's **only** sanctioned write target for notes that should survive across sessions. The agent appends here **only when PM or the user explicitly asks** (review-gated consolidation) — never as a side effect of doing work. Project-specific connectivity/OTA conventions that should outlive a single phase land here on request.

**Load when:** the `/net` agent starts a session, or when PM is auditing roster consistency via `/pm audit`.

**Key facts:**
- Owner: `/net` (Primary), `/pm` (Secondary) — per DOC_OWNERSHIP_MATRIX.md
- Write trigger: explicit PM/user request only, routed through the `/review` consolidation gate. Cite the request verbatim in the commit message.
- All other auto-memory writes by specialists are forbidden (see SKILL.md `## Memory Policy`).

**Owner:** `/net` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `.claude/skills/net/SKILL.md`, `DOC_OWNERSHIP_MATRIX.md`, `ARCHITECTURE.md`, `BUILD.md`

---

*Provenance: entries below consolidated from `doc/audit/2026-06-06-audit.md` (legacy-code audit, Wave 3). Delta approved by `/review` (APPROVE, no edits) on 2026-06-06. See `doc/audit/2026-06-06-guidelines-drafts.md` §3 for the authoritative source text.*

*Cross-cutting note: C1 and C3 below are domain instances of the project-wide "silently-ignored-configuration" failure class — see `ARCHITECTURE.md` → Cross-cutting failure patterns for the pattern-level policy.*

---

## Conventions

- **C1 Use `#if defined(X) && cond`, never `#ifdef X && cond` (H1, P1).** `#ifdef` takes one identifier; trailing `&&`/`!defined(...)` are silently ignored (warning only). `OTA.cpp:13/16/67` + `MdnsClient.cpp` drop their `NO_GLOBAL_*` guards. WiFiConnect.cpp already correct. Lead instance of systemic "silently-ignored-config".

- **C2 WiFi-lost path MUST also reset mqtt_conn_timestamp (H2, C4, P1).** When `EVENT_WIFI_LOST` fires, the MQTT branch is unreachable while WiFi is down; stale `mqtt_conn_timestamp > 0` → `EVENT_MQTT_LOST` fires after `EVENT_WIFI_RESTORED` (wrong order) and `EVENT_MQTT_RESTORED` may be suppressed entirely. That event drives `refresh_all_switches_state()` + re-announce → HA stale on miss. Rule: clear `mqtt_conn_timestamp` atomically in the WiFi-lost branch (fire `EVENT_MQTT_LOST` if it was > 0). (`Test.cpp:~190`)

- **C3 All begin() overload args must be forwarded, never silently dropped (M2 + mqtt M-1).** `WiFiConnect::begin(ssid,pwd,hostname,mdns_enabled)` passes hardcoded `false` for `mdns_enabled`. Every overload must forward all params to the canonical overload, or not accept the param. Constant-where-a-param-belongs is not acceptable delegation.

- **C5 millis() interval math must use `unsigned long` — never assign a millis() difference to a signed type (loopgate deadlock, P1).** `long period = (unsigned long)t - (unsigned long)stamp` silently sign-flips when the true elapsed time enters [2^31, 2^32) ms (~24.85–49.71 days uptime), making `period` negative and causing `period >= INTERVAL` to permanently return false until millis() rolls over. This froze the gate-reopen poll at `WiFiConnect.cpp:347` for up to ~24.9 days — exactly matching the "self-resolves after DAYS" symptom. Rule: all millis() delta variables must be `unsigned long`; drop any `|| period < 0` dead branches that result. (Provenance: `/review` adjudication in `doc/audit/2026-06-06-bughunt-loopgate.md`.)

- **C4 No begin()/pinMode()/digitalWrite() in global ctors (L8).** Global object ctors run at static-init before `setup()`/GPIO init; hardware calls may no-op or produce undefined state. Ctors init plain data only; hardware init belongs in `setup()`. (`Test.cpp:45-48`)

## Decisions

- **D1 Test event-ordering guarantees under WiFi drop/restore (C4) (2026-06-06).** After H2 fix: drop-together → `EVENT_MQTT_LOST` then `EVENT_WIFI_LOST` (same tick); restore → `EVENT_WIFI_RESTORED` then `EVENT_MQTT_RESTORED`; MQTT-only drop → `EVENT_MQTT_LOST`. Any change to order/suppression is a C4 contract change → surface to /pm.

## Open Questions

- **OQ1 Hardcoded network ID 192.168.66.0 in bad-DHCP detection (L7).** `Test.cpp:181` forces reconnect on exactly `192.168.66.0` (+ `!isIPAddressSet(gatewayIP)`). Options: (a) build-flag `-D BAD_DHCP_NETWORK_ID=...`, (b) remove and rely on gateway check, (c) keep (single-deployment). Recommend (b) unless portability needed. P2.
