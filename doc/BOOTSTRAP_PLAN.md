# RemotePowerControl — Task Router Bootstrap Plan (Index)

**Project:** RemotePowerControl (ESP8266 / ESP32 firmware — MQTT remote power switch controller)
**Project shape:** Single-repo (dual-MCU hybrid handled via `#ifdef`, one shared `ROADMAP.md`)
**Bootstrap state:** Fresh bootstrap — no `.claude/skills/`, `.claude/mcp/`, `.claude/SKILLS.md`, or `.claude/rules/` exists yet.
**Output mode:** Multi-file split under `doc/BOOTSTRAP_PLAN/` — 9-agent roster + 6 interface contracts trips two of the multi-file proxies (≥8 agents, ≥5 contracts); split for Read-limit harnesses (est. ~38–44K tokens monolithic).
**Date:** 2026-06-05
**Owner:** `/pm` (this plan has a matrix row; archive per `PM.md` → *Bootstrap plan — lifecycle and archive protocol* once live artifacts exist)

---

This index enumerates the plan sections. Each per-section file under `doc/BOOTSTRAP_PLAN/` is self-contained and readable in a single Read call. The user pastes each section's fills into the corresponding live artifact during the bootstrap checklist (§10).

| # | File | One-line summary |
|---|------|------------------|
| 1 | [`01_synthesis.md`](BOOTSTRAP_PLAN/01_synthesis.md) | What we build, v1 boundary, hard constraints (RAM, boot-safe GPIO, dual-MCU), non-goals, project invariants, §12 gating callouts. |
| 2 | [`02_design_principles.md`](BOOTSTRAP_PLAN/02_design_principles.md) | Why knowledge-domain specialists for this firmware, matrix context economy, what each default agent gets injected. |
| 3 | [`03_roster.md`](BOOTSTRAP_PLAN/03_roster.md) | Full per-agent blocks for all 9 agents (pm, arch, review, scm, power, mqtt, net, web, firmware) + candidate include/decline decisions. |
| 4 | [`04_agents_json.md`](BOOTSTRAP_PLAN/04_agents_json.md) | Exact `.claude/mcp/task-router/agents.json` JSON + per-agent model-resolution notes. |
| 5 | [`05_skills_md.md`](BOOTSTRAP_PLAN/05_skills_md.md) | `.claude/SKILLS.md` roster table (one row per agent). |
| 6 | [`06_matrix.md`](BOOTSTRAP_PLAN/06_matrix.md) | Document Ownership Matrix fills: §6.1 doc table, §6.2 load rules, §6.3 Abstract payloads, §6.4 quick-reference. |
| 7 | [`07_interface_contracts.md`](BOOTSTRAP_PLAN/07_interface_contracts.md) | The 6 data/function/file boundaries that cross specialist lines (callback signatures, MQTT payloads, `main.cpp` section ownership). |
| 8 | [`08_wave_mapping.md`](BOOTSTRAP_PLAN/08_wave_mapping.md) | Roadmap (from `README.txt` TODOs) → wave dispatch maps. Mostly serial (solo operator). |
| 9 | [`09_interaction_map.md`](BOOTSTRAP_PLAN/09_interaction_map.md) | ASCII topology — PM fan-out plus the standing cross-specialist callback arrows. |
| 10 | [`10_checklist.md`](BOOTSTRAP_PLAN/10_checklist.md) | Numbered fresh-bootstrap execution checklist with `[FRESH]` annotations + static pre-flight + runtime verify. |
| 11 | [`11_implementation_notes.md`](BOOTSTRAP_PLAN/11_implementation_notes.md) | Token budget table, terminal-vs-fork per phase, scaling-down, expected agent-system growth, canonical rule ownership. |
| 12 | [`12_open_gaps.md`](BOOTSTRAP_PLAN/12_open_gaps.md) | `Q:` decisions only the user can make + pre-handoff self-check result. |

**Pre-handoff checks: 7/7 pass** (detail in `12_open_gaps.md`).
