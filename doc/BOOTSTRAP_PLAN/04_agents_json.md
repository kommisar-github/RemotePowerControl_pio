## Section 4 — `agents.json`

Exact content for `.claude/mcp/task-router/agents.json`. Reserved-name check: `pm, arch, review, scm, power, mqtt, net, web, firmware` — none collides with (or is one edit from) a Claude Code subcommand (`agent, agents, auth, auto-mode, doctor, install, mcp, plugin, plugins, setup-token, update, upgrade`). ✓

```json
{
  "pm":       { "capabilities": ["planning", "delegation", "wave-dispatch", "doc-ownership", "reconciliation"], "role": "coordinator" },
  "arch":     { "capabilities": ["architecture", "component-boundaries", "phase-design", "init-sequencing"], "role": "coordinator" },
  "review":   { "capabilities": ["code-review", "auditing", "risk-assessment", "invariant-enforcement"], "role": "coordinator" },
  "scm":      { "capabilities": ["git", "commits", "branches", "tags", "prs"], "role": "specialist", "model": "claude-haiku-4-5" },
  "power":    { "capabilities": ["relay-control", "power-detection", "state-machine", "gpio", "optocoupler", "pulse-analysis"], "role": "specialist", "model": "claude-sonnet-4-6" },
  "mqtt":     { "capabilities": ["mqtt", "pubsubclient", "home-assistant", "topic-schema", "retained", "lwt", "json-telemetry"], "role": "specialist", "model": "claude-sonnet-4-6" },
  "net":      { "capabilities": ["wifi", "ota", "mdns", "reconnect", "phy-mode", "connection-monitor", "espota"], "role": "specialist", "model": "claude-sonnet-4-6" },
  "web":      { "capabilities": ["http-server", "web-ui", "html", "json-endpoint", "esp8266webserver"], "role": "specialist", "model": "claude-sonnet-4-6" },
  "firmware": { "capabilities": ["platformio", "build-flags", "dual-mcu", "gpio-map", "secrets", "serial-debug", "ota-deploy", "integration"], "role": "specialist", "model": "claude-sonnet-4-6" }
}
```

### Model resolution notes (per `PM.md` → "Model field — decision tree for `agents.json`")

- **pm, arch, review — Branch 1 (coordinator, no `model`, 1M Opus).** No `model` field, intentionally. These do long planning, matrix reads, reconciliation sweeps, and adversarial review across the whole tree — they need the 1M default. Any explicit `model` (even Opus) silently drops them to 200K.
- **scm — Branch 3 (mechanical, Haiku).** Git plumbing, staging, tags, version-string bumps. Canonical Haiku case. `model: "claude-haiku-4-5"`.
- **power — Branch 4 (default specialist, Sonnet).** Decision-heavy *within* its domain (timing logic), but it is competent execution against a known state machine, not the formal-verification / cross-cutting-refactor tier that warrants Opus. Sonnet is honest. `model: "claude-sonnet-4-6"`.
- **mqtt — Branch 4 (default specialist, Sonnet).** Protocol + topic-schema execution against a documented contract. `model: "claude-sonnet-4-6"`.
- **net — Branch 4 (default specialist, Sonnet).** Connectivity execution with vendor idioms; well-bounded. `model: "claude-sonnet-4-6"`.
- **web — Branch 4 (default specialist, Sonnet).** HTML/HTTP UI work; lightest specialist but still Sonnet-tier execution. `model: "claude-sonnet-4-6"`.
- **firmware — Branch 4 (default specialist, Sonnet).** Build-config + integration wiring; mechanical-leaning but the dual-MCU `#ifdef` and init-order reasoning is above Haiku. Sonnet. `model: "claude-sonnet-4-6"`.

**No per-agent Opus overrides.** No workload here genuinely needs the Opus-200K trade for a specialist. No workspace `taskRouter.modelByRole` setting required — every specialist carries an explicit `model`, so the roster is self-describing.
