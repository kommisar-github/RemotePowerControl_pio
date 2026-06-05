# /mqtt — Agent Guidelines
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Durable, project-specific guidelines for the `/mqtt` agent (MQTT protocol + Home Assistant integration). This file is the agent's **only** sanctioned write target for notes that should survive across sessions. The agent appends here **only when PM or the user explicitly asks** (review-gated consolidation) — never as a side effect of doing work. Project-specific topic/payload or HA-integration conventions that should outlive a single phase land here on request.

**Load when:** the `/mqtt` agent starts a session, or when PM is auditing roster consistency via `/pm audit`.

**Key facts:**
- Owner: `/mqtt` (Primary), `/pm` (Secondary) — per DOC_OWNERSHIP_MATRIX.md
- Write trigger: explicit PM/user request only, routed through the `/review` consolidation gate. Cite the request verbatim in the commit message.
- All other auto-memory writes by specialists are forbidden (see SKILL.md `## Memory Policy`).

**Owner:** `/mqtt` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `.claude/skills/mqtt/SKILL.md`, `DOC_OWNERSHIP_MATRIX.md`, `MQTT_API.md`

---

*Provenance: entries below derived from `doc/audit/2026-06-06-audit.md` (canonical `/review`-verified audit) + `doc/audit/2026-06-06-wave1-raw-findings.md`; approved by `/review` (verdict: APPROVE, no edits) — 2026-06-06.*

*Note: the "silently-ignored-config" pattern referenced below is a project-wide failure class; see `doc/design/ARCHITECTURE.md` → Cross-cutting failure patterns for the pattern-level policy.*

## Conventions

- **Inbound callback: length-guard before indexing payload (C-1, P0).** `mqtt_received_callback` and any inbound handler MUST `if (length < N) return;` before the first `payload[N-1]` access. PubSubClient doesn't null-terminate or enforce min length; short/crafted msg → stale bytes → garbage `switch_idx` that may pass `(-1<idx<3)` and actuate wrong relay. Fix: `if (length < 3) return;` atop STR_POWER_SET. (C1/C3; `main.cpp:499`)

- **info JSON: both DynamicJsonDocument pool AND MQTT frame must fit 1024 B (M-2+M-3, P1).** (1) ArduinoJson 6 silently drops fields when the pool (`DynamicJsonDocument(1024)`, `main.cpp:87`) is exhausted (~35 fields likely overflow). (2) MQTT frame `5+2+strlen(topic)+strlen(payload)` (~1054 B) exceeds `MQTT_MAX_PACKET_SIZE=1024`; `publish()` returns false but caller ignores it → HA gets no telemetry silently. Fix: raise doc+BUF to 2048 or split connection_info into a 2nd publish; check publish() return. (Cross-corroborated mqtt M-2/M-3 + firmware F-07.)

- **[2026-06-06, v1.0.1] info JSON buffer is a coupled triplet — change all together.** `DynamicJsonDocument(N)` (`main.cpp:87`), `BUFFER_SIZE/BUF[N]` (`main.cpp:88-89`), and `MQTT_MAX_PACKET_SIZE` (`MqttClient.h:4`) are coupled; raising one without the others silently re-introduces failure: pool<fields → ArduinoJson drops fields; BUF<doc → `serializeJson` truncates (`main.cpp:442`); MAXPKT<overhead+payload → `publish()` returns false (`MqttClient.cpp:198`). Treat as a single version-controlled triplet — change all in the same commit. Current value **2048** (raised from 1024 in v1.0.1). Effective max payload = `MQTT_MAX_PACKET_SIZE − (MQTT_MAX_HEADER_SIZE[5] + 2 len-field + strlen(topic))`; overhead is PER-TOPIC — do NOT hardcode. Worst case is the connection/info frame (largest payload on the longest topic `home/rempowercntl1/connection/info` = 34 ch) → overhead 41 → ~2007 B usable (len-field is 2 B for payloads 128..16383, covering the 2048 buffer). NB: `MQTT_MAX_PACKET_SIZE` only takes effect via `setBufferSize(MQTT_MAX_PACKET_SIZE)` at `MqttClient.cpp:39` (PubSubClient defaults to 256); if that call is removed every large frame silently fails — effectively a 4th coupled site. *(v1.0.1 fix phase; /review APPROVE-WITH-EDITS)*

- **begin() enabled-flag overload silent-drop (M-1).** `begin(server,id,bool enabled)` delegates to 4-arg which unconditionally sets `_enabled=true` (`MqttClient.cpp:95`), overriding `false`. Until fixed always pass `true`; don't use the boolean overload to defer init. Instance of systemic "silently-ignored-config". Fix: remove the unconditional `_enabled=true`.

## Decisions

- **STR_POWER_GET handler intentionally no-op (pending) (L-4).** `power/get` subscribed but body commented (`main.cpp:512-513`); retained `state/`+`status/` cover steady state. Implement-vs-deprecate is an open decision (see Open Questions Q1).

- **Legacy "inTopic" subscription dead, scheduled for removal (L-2).** `resubscribe()` subscribes `"inTopic"` each reconnect (`MqttClient.cpp:235`); no handler matches. Grouped with dead-code cleanup (P2).

## Open Questions

- **Q1 Should power/get trigger on-demand refresh?** (a) call `refresh_all_switches_state()` for HA polling, or (b) remove subscription. Retained topics give eventual consistency; (a) only if HA needs synchronous confirmation. Defer to fix phase.

- **Q2 Should LWT/announcement "connected" be retained?** Both announcement and LWT currently non-retained; HA restart won't see "connected" until next keepalive (60 s). Documented intentional (MQTT_API.md) but may degrade HA availability UX.
