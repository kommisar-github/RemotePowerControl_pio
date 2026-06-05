## Section 10 — Bootstrap checklist

**Bootstrap state: FRESH.** No `.claude/skills/`, `.claude/mcp/`, `.claude/SKILLS.md`, or `.claude/rules/` exists (the repo has only `.vscode/`, `src/`, `include/`, `lib/`, `test/`, `platformio.ini`, `README.txt`). Every file-write below is a fresh write — `[FRESH]` throughout. (If a partial `init.sh` run has happened by application time, switch the affected steps to `[APPEND]`/`[MERGE]` per the `BOOTSTRAP_PROMPT.md` §10 merge-rule table; re-emit this section in Incremental mode if so.)

1. **Prerequisites.** Task Router server installed; VS Code extension (`.vsix`) installed; `.mcp.json` at repo root with `?project=remotepowercontrol` (e.g. `http://127.0.0.1:3100/mcp?project=remotepowercontrol`). `init.sh` writes `.mcp.json` if you have the seed clone.

2. **Directory structure** `[FRESH]` — one `mkdir` per dir (portable across cmd/PowerShell/bash; no brace expansion):
   ```
   mkdir -p .claude/skills/pm
   mkdir -p .claude/skills/arch
   mkdir -p .claude/skills/review
   mkdir -p .claude/skills/scm
   mkdir -p .claude/skills/power
   mkdir -p .claude/skills/mqtt
   mkdir -p .claude/skills/net
   mkdir -p .claude/skills/web
   mkdir -p .claude/skills/firmware
   mkdir -p .claude/tasks
   mkdir -p .claude/rules
   mkdir -p doc/design
   mkdir -p doc/plans
   ```
   PowerShell equivalent: `New-Item -ItemType Directory -Force -Path .claude/skills/pm, .claude/skills/arch, ...` (`doc/`, `doc/design`, `doc/plans` already exist from this plan's authoring).

3. **`CLAUDE.md`** `[FRESH]` from `PM_TEMPLATES.md → CLAUDE.md Template` + append `CLAUDE.md Agent Routing Section`. Fill the five global placeholders: `<PROJECT_NAME>`=`RemotePowerControl`, `<DOC_DIR>`=`doc`, `<SHARED_DOC_DIR>`=`doc` (single-repo collapse), `<DATE>`=`2026-06-05`, `<YYYY-MM-DD>`=`2026-06-05`. Splice in the three project invariants (§1) and the agent-routing table for all 9 agents.

4. **`.claude/SKILLS.md`** `[FRESH]` — paste §5 roster table into `PM_TEMPLATES.md → SKILLS.md Template`.

5. **`.claude/rules/INDEX.md`** `[FRESH]` from `PM_TEMPLATES.md → Rules INDEX Template`; add one row per specialist (`power, mqtt, net, web, firmware`) with the globs from §3, plus `project.md` (global). Coordinators need no rule file.

6. **`doc/design/DOC_OWNERSHIP_MATRIX.md`** `[FRESH]` from `PM_TEMPLATES.md → DOC_OWNERSHIP_MATRIX.md Template` (Sections 1–4 only, no appendices). Paste §6.1→Section 1, §6.2→Section 2, keep the template's Section 3 format spec verbatim, §6.4→Section 4.

7. **Abstract blocks into design docs** `[FRESH]` — create the five design-doc files and paste each §6.3 Abstract at the top: `doc/design/ARCHITECTURE.md`, `POWER_STATE.md`, `HARDWARE.md`, `MQTT_API.md`, `BUILD.md`. (Bodies are authored in Phase 0 waves 0.2–0.4; the Abstract goes in now so the matrix references resolve.)

8. **Default skills** `[FRESH]` from `PM_TEMPLATES.md` exact headings:
   - `## PM Skill Template` → `.claude/skills/pm/SKILL.md` (inject roster + §7 contracts + §8 waves + invariants under `## Project Knowledge (injected)`)
   - `## SCM Skill Template` → `.claude/skills/scm/SKILL.md` (note PlatformIO repo + version-bump reference to `/firmware`)
   - `## Architect Skill Template` → `.claude/skills/arch/SKILL.md` (inject §3 arch domain bullets)
   - `## Architecture Review Skill Template` → `.claude/skills/review/SKILL.md` (inject the §3 review risk checklist)

9. **Domain specialist skills** `[FRESH]` — one per specialist (`power, mqtt, net, web, firmware`) from §3 blocks (Owns / Never-touches / SKILL.md domain knowledge / tiered-load). Each MUST include, copied verbatim from `PM_TEMPLATES.md`:
   - `### Startup Sequence Template` → the `**For specialists (canonical template — copy exactly):**` fenced block
   - `### Task Router Auto-Execute Template` → the Ping + Reconciliation handler block
   - honor `### Agent Design Principles` (canonical-rule-ownership) when drafting the body

10. **Per-specialist rule files** `[FRESH]` `.claude/rules/<name>.md` for `power, mqtt, net, web, firmware` — condensed pitfalls + NEVER-touches + globs (YAML frontmatter `globs:`/`alwaysApply:` informational). Content is a condensed version of each §3 SKILL.md; full knowledge stays in the SKILL.md.

11. **`agents.json`** `[FRESH]` — write §4 JSON to `.claude/mcp/task-router/agents.json` verbatim.

12. **VS Code workspace settings** — optional; **skip** `taskRouter.modelByRole` (every specialist has an explicit `model` in `agents.json`).

13. **Static pre-flight verification** (before step 14):
    - Every `agents.json` name has a matching `.claude/skills/<name>/SKILL.md` → expect 9/9.
    - Every specialist (`scm, power, mqtt, net, web, firmware`) has a `.claude/rules/<name>.md`; coordinators (`pm, arch, review`) intentionally omit the rule file — note the exception.
    - Every SKILL.md contains `## Startup Sequence` and `## Task Router Auto-Execute` (grep) — missing either = no auto-register.
    - Every specialist entry has an explicit `"model"`; `pm`/`arch`/`review` have **no** `"model"` key (1M Opus gate).
    - Sorted-diff report: `agents.json` names ↔ `.claude/skills/*/` dirs ↔ `.claude/rules/*.md` stems. Expected asymmetry: 3 coordinators have no rule file (allowed); any *other* asymmetry is a bootstrap error — fix before step 14.

14. **Runtime verify.** Start PM terminal → `/pm` registers and lists 9 agents → `/pm ping` returns healthy for all specialists → start one specialist terminal (e.g. `/power`) → dispatch a subagent-fork task and confirm it reads only the `Context docs:` cited (matrix + `POWER_STATE.md`), not all five design docs.

**Ordering note:** steps 9 and 11 are interchangeable but both must precede step 13 and step 14. Steps 2–12 may be done in any order in a single edit session; do **not** start terminals (step 14) until 2–12 are complete, and run step 13's static diff immediately before step 14.
