# /web — Agent Guidelines
**Last Updated:** 2026-06-06

## Abstract

**TL;DR:** Durable, project-specific guidelines for the `/web` agent (HTTP UI). This file is the agent's **only** sanctioned write target for notes that should survive across sessions. The agent appends here **only when PM or the user explicitly asks** (review-gated consolidation) — never as a side effect of doing work. Project-specific web-UI conventions that should outlive a single phase land here on request.

**Load when:** the `/web` agent starts a session, or when PM is auditing roster consistency via `/pm audit`.

**Key facts:**
- Owner: `/web` (Primary), `/pm` (Secondary) — per DOC_OWNERSHIP_MATRIX.md
- Write trigger: explicit PM/user request only, routed through the `/review` consolidation gate. Cite the request verbatim in the commit message.
- All other auto-memory writes by specialists are forbidden (see SKILL.md `## Memory Policy`).

**Owner:** `/web` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `.claude/skills/web/SKILL.md`, `DOC_OWNERSHIP_MATRIX.md`, `ARCHITECTURE.md`

---

*Provenance: entries below derived from `doc/audit/2026-06-06-audit.md` (canonical /review verdict, Wave 2) + `doc/audit/2026-06-06-wave1-raw-findings.md`. `/review` APPROVED this delta (Wave 3, 2026-06-06). Web C-W1 intentionally MEDIUM/P2 per /review downgrade (cosmetic only).*

---

## Conventions

- **C-W1 PROGMEM placeholder substitution must replace the FULL `{token}`, not a bare substring (C1, MEDIUM/P2 — DOWNGRADED).** `WebSServer.cpp:42` `page.replace("device1","DEVICE1")` matches the inner substring, turning `{device1}`→`{DEVICE1}` (braces retained) and never touching `{device2/3}`. Rule: always replace the complete delimited token (`"{device1}"`). NOTE per /review: the 2 s JS `/getstatus` poll self-heals `{stat1..3}` and controls use hardcoded indices, so real impact is **cosmetic device-header labels only** — do not overstate as critical. See ARCHITECTURE.md → Cross-cutting failure patterns ("silently-ignored-configuration").

- **C-W2 /getstatus JsonDocument must fit all nodes or it silently truncates (M1).** `DynamicJsonDocument(256)` (`WebSServer.cpp:101`) is borderline for the root+array+3×3-field payload; pool overflow → empty/truncated doc → client JS `JSON.parse` crash. Size the document to the ArduinoJson-assistant capacity (≈384 B) and keep `BUF` adequate.

- **C-W3 Header-scope const PROGMEM must be `static` (or extern+def in .cpp) (M2, latent linker hazard).** `strings1.h:4-7` declares `TAG_STAT/TAG_VER/PAGE_BUTTONS_OLD/PAGE_BUTTONS` as non-static `const char* PROGMEM` at file scope; a second include → multiple-definition linker error. Mark `static` or move defs to `WebSServer.cpp` with `extern` decls.

- **C-W4 /getstatus should send `application/json`, not `text/plain` (L1).** `WebSServer.cpp:126` uses `text/plain`; JS works via `JSON.parse` but header consumers (curl -i, HA REST sensors) break. Use `application/json`.

## Decisions

- **D-W1 Web index is 0-based (confirmed 2026-06-06).** `setswitch?switch=0..2` is 0-based and correct; distinct from MQTT's 1-based payload (C1). ESP8266WebServer/WebServer `#ifdef` branches verified at parity. (Do not unify the two index bases.)

- **D-W2 PAGE_BUTTONS_OLD is dead, scheduled for removal (M3).** Unreferenced; its JS uses `,` instead of `&` between query params (`/setswitch?switch=e,status=x`) → unparseable; ~2 KB flash. Remove with the dead-code cleanup (P2).

## Open Questions

- **Q-W1 Force-power checkbox state vs the 2 s poll (L3).** `/getstatus` poll sets `sw*f.checked = devs[].on`, overwriting the user's force-off feedback during the 10 s `long_push`. Should force checkboxes be (a) not synced from `/getstatus`, or (b) shown disabled/indeterminate during `POWERING_OFF`? (b) needs a `POWERING_OFF` signal — coordinate with /power (C5). Defer.
