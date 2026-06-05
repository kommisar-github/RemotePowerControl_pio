# Documentation Ownership Matrix

**Purpose:** Maps every design / implementation / reference document in the remotepowercontrol project to the specialist agent(s) responsible for it. Agents read their assigned docs when dispatched; PM uses this matrix to pick the right delegate.

**Last Updated:** 2026-06-05
**Maintained By:** `/pm`

## Abstract

**TL;DR:** Index of every design / implementation / reference doc in the remotepowercontrol project, mapping each to the specialist agent(s) that own or read it. PM's routing table for dispatch; agents' filter for which docs are relevant to their domain.

**Load when:** doc ownership, who owns, which agent reads, doc assignment, new doc added, doc primary owner, where to add a new doc, Abstract standard, `DOC_OWNERSHIP_MATRIX`

**Key facts:**
- Every doc in the matrix has an `## Abstract` block (see Section 3) for cheap skimming by subagent forks.
- New docs MUST be added to this matrix in the same commit — un-matrixed docs go stale within weeks.
- PM reads ROADMAP + NEXT_STEPS + MEMORY + this matrix on every dispatch.
- Primary agent updates the doc; secondary agents read but don't own.

**Owner:** `/pm` (self-referential)
**Related:** `ROADMAP.md`, `NEXT_STEPS.md`, `MEMORY.md`

---

## How to use this matrix

- **Primary** — the agent who *owns* the doc: updates it after implementation, enforces its conventions, is the first delegate for related work.
- **Secondary** — agents who must *read* the doc when they touch its subject, but don't own updates.
- **Type** — helps filter by purpose:
  - `roadmap` — phase progress + next steps
  - `design` — architecture + acceptance criteria for a feature
  - `reference` — stable facts (protocol, hardware, config)
  - `analysis` — investigation output, may become stale
  - `plan` — in-flight implementation plan
  - `memory` — project state / history
- **Every agent reads its own roadmap + NEXT_STEPS first** on any task in its domain.
- **`/pm` reads all roadmap + NEXT_STEPS + MEMORY docs** on any dispatch, in addition to whatever the delegated specialist loads.

---

## 1. `doc/` — cross-cutting design + architecture (single-repo shared)

| Document | Type | Primary | Secondary | Notes (what triggers an update) |
|---|---|---|---|---|
| `doc/plans/ROADMAP.md` | roadmap | `/pm` | all | Phase overview + status badges. Update on every phase gate (open/close). |
| `doc/NEXT_STEPS.md` | roadmap | `/pm` | all | Current action items. Update after each implement/verify cycle. |
| `doc/MEMORY.md` | memory | `/pm` | all | Cross-phase decisions, hardware facts, the "detect-pin disconnected" bug findings. Update when a phase closes or a non-obvious lesson is learned. |
| `doc/design/DOC_OWNERSHIP_MATRIX.md` | reference | `/pm` | all | **This file.** Update in the same commit as any doc add/rename/owner-change. |
| `doc/BOOTSTRAP_PLAN.md` (+ `BOOTSTRAP_PLAN/`) | plan | `/pm` | all | This plan. Archived post-bootstrap; §7 (contracts) + §8 (waves) stay live unless migrated to `ARCHITECTURE.md`. |
| `doc/design/ARCHITECTURE.md` | design | `/arch` | all specialists | Module map, `setup()` init order, cooperative-loop contract, dual-MCU rule. Update when a module/boundary/init-order changes. |
| `doc/design/POWER_STATE.md` | design | `/power` | `/mqtt`, `/web`, `/review` | RSwitch state machine + pulse-detection algorithm + timing constants. Update when state logic or a timing constant changes. |
| `doc/design/HARDWARE.md` | reference | `/power` | `/firmware`, `/review` | Relay/optocoupler/button wiring + per-MCU pin behavior + boot-safety. Update only on hardware/pin-map change. |
| `doc/design/MQTT_API.md` | reference | `/mqtt` | `/power`, `/firmware`, `/review` | Topic grammar, payload byte-layout, retained/LWT rules, `info` JSON schema, HA mapping. Update when a topic/payload/field changes. |
| `doc/design/BUILD.md` | reference | `/firmware` | `/net`, `/scm`, `/review` | PlatformIO env matrix, build flags/secrets, flashing + OTA procedure, lib-dep pins. Update on env/flag/dependency/board change. |
| `doc/design/PHASE_<N>_DESIGN.md` | design | `/arch` | phase-relevant specialists | Created at each phase start from `/arch` output; mark `SUPERSEDED` (not deleted) when the phase closes. |
| `doc/<agent>_GUIDELINES.md` | reference | `/<agent>` | `/pm` | Per-agent durable guidelines (review-gated write target). One per agent; PM is Secondary. |

---

## 2. Cross-cutting load rules

**When dispatched for a task in a domain (single-repo shape):**
1. **Primary agent** reads `ROADMAP.md` + `NEXT_STEPS.md` + `MEMORY.md` + all docs where it is Primary. Dedicated-terminal: amortized at startup. Fork: only the matrix + docs cited in `Context docs:`.
2. **Secondary agents** on the same dispatch read only the specific design doc that applies (usually cited by PM).
3. **All agents** read their `.claude/skills/<agent>/SKILL.md` first.

**When PM is doing triage / planning:** PM reads `CLAUDE.md`, `ROADMAP.md`, `NEXT_STEPS.md`, `MEMORY.md`, and this matrix. PM does **not** read `POWER_STATE.md` / `MQTT_API.md` / `BUILD.md` in full unless triaging that specific domain.

**When writing a new doc:** cross-cutting design → `doc/design/`; planning/status → `doc/plans/` or `doc/`; **add a matrix row in the same commit** — un-matrixed docs rot within weeks.

**Worked token-savings example.** A `/power` fork dispatched for *"tune the SLEEP-detection short-pulse threshold so a flickering UPS LED isn't read as sleep"* loads:
- `doc/design/DOC_OWNERSHIP_MATRIX.md` (~2K)
- `doc/design/POWER_STATE.md` (~6K, cited in `Context docs:`)
- `doc/design/HARDWARE.md` detect-pin section (~4K, cited)

Total **≈ 12K tokens**. Without the matrix the same fork would cold-load `ARCHITECTURE.md` + `MQTT_API.md` + `BUILD.md` as well (~55–65K) just to locate the constants. ~5× saving; context freed for the actual timing reasoning.

**Ownership transfer (when a doc changes Primary).** Use the protocol **verbatim** (PM proposes → user approves → matrix row + the doc's `## Abstract` `Owner:` updated in the **same commit** as the motivating code change → log a `## History` entry). Never silently change the Primary column. Likely future transfer for this project: if the "persistent setup / WiFi portal" work makes config a first-class concern, parts of `BUILD.md` may split into a `CONFIG.md` owned jointly by `/firmware` + `/net` — flag, don't pre-split.

---

## 3. Abstract standard (for all docs in the matrix)

Every doc listed in Section 1 (and Appendix A / Appendix B, if used) must have an `## Abstract` block placed immediately after the title/metadata header and before the first content section. This lets agents skim all abstracts cheaply (~50 tokens each) to decide which docs to read in full — critical for subagent fork context efficiency.

### Format

```markdown
## Abstract

**TL;DR:** <1–2 sentence description of what the doc covers and its purpose.>

**Load when:** <comma-separated keywords/phrases that would appear in task payloads.>

**Key facts:**
- <one-liner non-obvious fact that would save time if known upfront>
- <another key fact, max 5 bullets>
- <prefer facts that were bug-causing when forgotten>

**Owner:** `/<agent>` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `OTHER_DOC.md`, `ANOTHER.md`
```

### Rules

- `TL;DR` — 1–2 sentences on the doc's purpose (not its structure).
- `Load when` — comma-separated matchable keywords. The most important field for heuristic matching. **Count keywords as noun phrases, not individual tokens** — `rate limit middleware` is one keyword (one comma-bounded phrase), not three. Commas are the phrase boundary; spaces inside a phrase are not. A list of 20 noun phrases reads as 20 keywords regardless of how many individual words the phrases contain.
- `Key facts` — 2–5 bullets of non-obvious load-bearing info. Prefer facts that would have prevented past bugs.
- `Owner` — must match the Primary column. Use `Owner: (none — read lazily per task)` if no Primary is assigned.
- `Related` — 2–4 most-commonly-co-loaded docs. Not exhaustive; do not include the doc itself.

### Placement

Immediately after the title/metadata block, before the first content section. If there is no metadata block, place it immediately after the `# Title`.

### Special cases

- **Live-state docs** (`ROADMAP.md`, `NEXT_STEPS.md`, `MEMORY.md`) — Abstract describes the **purpose and structure** of the file, not its current contents.
- **Hardware reference docs** — lead with the hardware model name; `Key facts` MUST include VID/PID for USB devices and I2C address for I2C devices.
- **Design docs** (`DESIGN_*.md`) — `Key facts` captures the 2–3 load-bearing decisions the design makes, not the process of making them.
- **`DOC_OWNERSHIP_MATRIX.md`** itself — has a self-referential Abstract; `Owner: /pm`.

### When adding a new doc

1. Write the doc.
2. Add the Abstract block immediately after the title/metadata.
3. Add an entry to this matrix in the appropriate section.
4. Commit the new doc + matrix update in the same commit.

---

## 4. Quick-reference — "which agent for which topic"

PM uses this table to pick `Context docs:` for every `dispatch_task` payload. One row per topic domain; cover every capability claimed by every specialist in the project.

| If the task is about… | Delegate | Context docs |
|---|---|---|
| Planning, phase gating, delegation, doc updates | `/pm` | `ROADMAP.md` + `NEXT_STEPS.md` + `MEMORY.md` + this matrix |
| Architecture, module boundaries, init order, dual-MCU convention | `/arch` | `ARCHITECTURE.md` + relevant `PHASE_<N>_DESIGN.md` |
| Auditing a change / design / plan; RAM, invariants, ESP8266-ESP32 parity | `/review` | the diff/doc under review + `ARCHITECTURE.md` |
| Git, commits, branches, tags, PRs | `/scm` | (dispatch payload describes the change) |
| Relay control, power-state detection, RSwitch state machine, pulse timing, SLEEP | `/power` | `POWER_STATE.md` + `HARDWARE.md` |
| Hardware wiring, GPIO behavior, boot-safe pins, optocoupler | `/power` (+ `/firmware` for pin numbers) | `HARDWARE.md` |
| MQTT topics, payloads, retained/LWT, keepalive, `info` JSON, Home Assistant | `/mqtt` | `MQTT_API.md` |
| WiFi connect/reconnect, PHY mode, OTA flashing behavior, mDNS, connection health | `/net` | `ARCHITECTURE.md` (connectivity) + `BUILD.md` (OTA) |
| Web UI, HTTP routes, `/getstatus` JSON, the buttons page | `/web` | `ARCHITECTURE.md` (web routes) |
| Build envs, build flags, secrets, pin map, serial debug, `main.cpp` wiring, OTA deploy | `/firmware` | `BUILD.md` + `HARDWARE.md` (pin table) + `ARCHITECTURE.md` (init order) |
| `main.cpp` change spanning topic + power callback + init | `/pm` coordinates → owning section specialist | `ARCHITECTURE.md` + §7 contract C6 (shared-file ownership) |

---

*Single-repo shape: Sections 1–4 only, no appendices. The ESP8266/ESP32 dual-target is handled by `#ifdef`, not separate platform roadmaps — Appendix A/B intentionally omitted.*

