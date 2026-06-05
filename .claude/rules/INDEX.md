# INDEX.md - Project Rule Roster
**Last Updated:** 2026-06-05
**Location:** `.claude/rules/*.md` (files) + this index

## Purpose

This index lists every rule file in the project, what domain it covers, which glob patterns the rule applies to, and which Claude Code skill it mirrors. PM consults this index when dispatching domain work and includes the relevant rule path in the dispatch payload's `Context docs:`.

## Quick Reference

| Rule | Globs | Domain | Matching Skill |
|------|-------|--------|----------------|
| `project.md` | `alwaysApply: true` | Global planning-only default, phase isolation | (global) |
| `power.md` | `src/RSwitch.cpp`, `src/RSwitch.h` | RSwitch state machine, relay pulses, optocoupler power detection, GPIO meaning | `.claude/skills/power/SKILL.md` |
| `mqtt.md` | `src/MqttClient.cpp`, `src/MqttClient.h` | MQTT topic grammar, set-payload layout, retained/LWT, info JSON, HA | `.claude/skills/mqtt/SKILL.md` |
| `net.md` | `src/WiFiConnect.*`, `src/OTA.*`, `src/MdnsClient.*`, `src/Test.*` | WiFi/OTA/mDNS, reconnect/PHY, Test connection-health events | `.claude/skills/net/SKILL.md` |
| `web.md` | `src/WebSServer.*`, `src/strings1.h` | Embedded HTTP server, PROGMEM page, /getstatus JSON | `.claude/skills/web/SKILL.md` |
| `firmware.md` | `platformio.ini`, `src/Application.h`, `src/main.cpp`, `src/SerialDebug.*`, `src/Log.*` | PlatformIO envs, build flags/secrets, pin maps, init wiring, serial debug | `.claude/skills/firmware/SKILL.md` |

**Coordinators omit rule files by design:** `/pm`, `/arch`, `/review` have a `.claude/skills/<name>/SKILL.md` but **no** `.claude/rules/<name>.md` (they are manual-invoke / cross-cutting, not glob-scoped to a source subtree). Only `project.md` (global) + the five specialist rules above exist. `/scm` is also manual-invoke and intentionally has no rule file.

(Add rows as PM proposes new specialist agents. Each new `.claude/skills/<name>/SKILL.md` should get a paired `.claude/rules/<name>.md` with a row here.)

## Convention

- **Specialist rules** (per-agent): `<name>.md`, `alwaysApply: false`, globs document which files the rule applies to (informational).
- **Global rules**: `alwaysApply: true` (e.g., `project.md`).
- **Rule content** is a condensed version of the agent's SKILL.md — key pitfalls, NEVER-touches, naming conventions. Full domain knowledge lives in the SKILL.md under `.claude/skills/<name>/`.
- **Sync rule**: whenever `agents.json` or `.claude/SKILLS.md` gain an agent, add a matching `.claude/rules/<name>.md` and a row here.

## See Also

- `.claude/SKILLS.md` — Claude Code skill roster (the agent-system roster counterpart to this file)
- `.claude/skills/<name>/SKILL.md` — full agent definitions
- `CLAUDE.md` — top-level project rules, agent routing, execution modes

