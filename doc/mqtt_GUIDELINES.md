# /mqtt — Agent Guidelines
**Last Updated:** 2026-06-05

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

## Conventions

(none yet — populate only when PM or the user asks)

## Decisions

(none yet — append dated entries when PM or the user asks)

## Open Questions

(none yet)
