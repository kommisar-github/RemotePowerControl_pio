# SKILLS.md - Agent Skill Definitions and Usage Guide
**Last Updated:** 2026-06-05
**Locations:** .claude/skills/<name>/SKILL.md + .claude/rules/

## Overview
| System | Location | Trigger | Best For |
|--------|----------|---------|----------|
| Claude Code Skills | .claude/skills/<name>/SKILL.md | Manual (/name) | CLI |
| Project Rules      | .claude/rules/*.md             | Read by Task Router agents via the `Read` tool | Domain guardrails referenced from skills + matrix |

## Quick Reference

Nine agents: four defaults (pm / arch / review / scm) + five domain specialists (power / mqtt / net / web / firmware). Coordinators (pm/arch/review) carry no `model` in `agents.json` so they keep the 1M Opus window.

| Skill | CLI | Cursor Globs | Domain | Model |
|-------|-----|--------------|--------|-------|
| Project Manager | `/pm` | `doc/**`, `.claude/**` | Planning, delegation, wave dispatch, doc ownership, reconciliation | (1M Opus — no model) |
| Architect | `/arch` | `doc/design/**` | Architecture, component boundaries, init sequencing, dual-MCU convention | (1M Opus — no model) |
| Architecture Review | `/review` | `src/**`, `doc/design/**` | Code review, RAM/heap audit, invariant enforcement, ESP8266/ESP32 parity | (1M Opus — no model) |
| Source Control | `/scm` | *(manual invoke)* | Git, commits, branches, tags, PRs | claude-haiku-4-5 |
| Power Control | `/power` | `src/RSwitch.cpp`, `src/RSwitch.h` | RSwitch state machine, relay pulses, optocoupler power detection, GPIO | claude-sonnet-4-6 |
| MQTT / Home Assistant | `/mqtt` | `src/MqttClient.cpp`, `src/MqttClient.h` | MQTT topics, PubSubClient, retained/LWT, JSON telemetry, HA integration | claude-sonnet-4-6 |
| Connectivity | `/net` | `src/WiFiConnect.*`, `src/OTA.*`, `src/MdnsClient.*`, `src/Test.*` | WiFi, OTA (espota), mDNS, reconnect/PHY, connection-health monitor | claude-sonnet-4-6 |
| Web UI | `/web` | `src/WebSServer.*`, `src/strings1.h` | Embedded HTTP server, HTML/CSS/JS page, `/getstatus` JSON endpoint | claude-sonnet-4-6 |
| Firmware / Build | `/firmware` | `platformio.ini`, `src/Application.h`, `src/main.cpp`, `src/SerialDebug.*`, `src/Log.*` | PlatformIO env matrix, build flags/secrets, pin maps, init wiring, serial debug | claude-sonnet-4-6 |

## How to Use

### Mode 1-2 (Direct / Agent Fork)
- /pm - Plan, triage, track progress, propose new agents
- /scm - Commit, push, create branches, PRs
- /arch - Design new phases, propose architecture
- /review - Audit and challenge architect designs
- /power - RSwitch state machine, relay pulses, optocoupler power detection
- /mqtt - MQTT topics/payloads, retained/LWT, info JSON, Home Assistant
- /net - WiFi/OTA/mDNS, reconnect/PHY, Test connection-health events
- /web - Embedded HTTP server, buttons page, /getstatus JSON
- /firmware - PlatformIO envs, build flags/secrets, pin maps, main.cpp wiring
- /<agent> <description> - Invoke with task context

### Mode 3 (Terminal Task)
- /pm task <agent> <description> - PM writes task file for specialist
- "read your task" - Specialist reads and executes in their terminal
- "read <agent> result" - PM reads result in their terminal

### Mode 4 (MCP Task Router)
- Launch: Ctrl+Shift+P → "Tasks: Run Task" → agent name (or "all agents")
- Skills auto-register via $TASK_ROUTER_AGENT env var check (no manual step needed)
- Manual fallback: /<agent> mcp register
- /pm <request> - PM auto-dispatches via MCP if agent is online
- Hook auto-polls inbox (specialists) and results (PM) on every prompt
- /pm serve results - Explicitly review completed results anytime

## Adding New Agents
The PM agent (/pm) proposes new agents when it detects coverage gaps.
You can also request: /pm propose an agent for <domain>.

