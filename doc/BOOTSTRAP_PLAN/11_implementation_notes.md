## Section 11 — Implementation notes

**Context token budget.** Per-agent SKILL.md injection sizes (rule of thumb): lightly-customized default ≈ 1K; project-specific specialist with full domain knowledge ≈ 3–4K; PM with roster + 6 interface contracts + invariants ≈ 6–8K. PM fits 1M Opus with enormous headroom.

PM practical session floor (6-agent-class project — 9 agents but a compact doc set of 5 design docs):

| Component | Plan minimum | Practical floor |
|---|---|---|
| Project-specific SKILL.md injections (roster + §7 + invariants) | ~6–8K | ~6–8K |
| Default `/pm` SKILL.md (init.sh baseline) | — | ~4–6K |
| `CLAUDE.md` + rules `INDEX.md` | — | ~3–5K |
| Auto-memory load at startup | — | ~3–8K (small project, lean MEMORY) |
| Matrix + Abstract blocks (5 docs) | — | ~3–5K |
| **Total PM session floor** | ~6–8K | **~20–28K** |

That lands squarely in the calibration band for a 6-agent single-repo project (~20–28K). `1M − 28K ≈ 972K` headroom — comfortable. Specialist floors are lower (~10–16K: no reconciliation, no full matrix pre-load) — `/power` is the heaviest specialist (12 domain bullets + `POWER_STATE.md` + `HARDWARE.md`), still well under 16K.

**Terminal vs fork trade-off, by phase:**

| Phase | Dedicated terminals | One-shot forks suffice |
|---|---|---|
| 0 (bootstrap) | pm | arch, firmware, power, mqtt (doc-authoring is one-shot) |
| 1 (MQTT reliability) | pm, mqtt | power (bug repro is a focused fork), net, review |
| 2 (web UI) | pm, web | power (one C5 change), review |
| 3 (persistence) | pm, firmware, net | arch (design fork), review |
| 4 (IVR design) | pm | arch |

Heavy-domain work (3+ file reads to rebuild context) justifies a terminal; the per-phase load-bearing specialist gets one, everything else forks.

**Scaling-down (don't run 9 terminals).** `arch`, `review`, `scm` rarely need persistent terminals — one-shot forks cover them unless in a heavy design/review cycle. Per phase, the strict minimum is **pm + the one phase-lead specialist** (e.g., Phase 1 = pm + mqtt). Two terminals is the common case; three only when a file-disjoint 2-way wave is actually run concurrently.

**Relationship to existing design docs.** There are no prior design docs — `README.txt` is the only spec, and the source is the ground truth. This bootstrap creates the *first* design docs (Phase 0). Going forward: the design docs (`ARCHITECTURE.md`, `POWER_STATE.md`, `MQTT_API.md`, `HARDWARE.md`, `BUILD.md`) are the source of truth for **what** to build; this plan + the SKILL.md files define **how the agents collaborate to build it**.

**Expected agent-system growth** (`PM.md` agent-evolution triggers, calibrated):
- **Technology boundary — *likely*.** Phase 3 introduces a persistent filesystem (LittleFS/EEPROM) and a captive/setup portal — a new technology surface. Expect a `/config` (or `/storage`) specialist to be proposed when persistence work exceeds a few bullets in `/firmware` + `/net`.
- **New project domain — *possible*.** Phase 4's Telephone/IVR interface is a genuinely new domain (telephony/SIP/voice gateway). If pursued, a `/ivr` specialist is the natural proposal — but it's post-v1 and avoidable by scoping IVR to an external gateway the firmware just publishes to.
- **Coverage gap — *possible*.** Over-the-air config or an MQTT-discovery (HA auto-discovery) feature could span `/mqtt` + `/config` without a clean owner; PM would flag it.
- **Context overload — *unlikely*.** No specialist is near the ~15-file / ~60-rule threshold; the heaviest (`/firmware`, `/net`) own ~4–8 files each.
- **Recurring cross-cutting concern — *unlikely*.** The invariants already capture the cross-cutting rules (secrets, boot-safe GPIO, non-blocking loop) under named owners; no recurring un-owned work pattern is visible.
- **User friction — *unlikely* at bootstrap.** Re-evaluate if the operator repeatedly re-explains, e.g., the optocoupler polarity or the topic grammar — that would signal a doc/abstract gap, not a new agent.

**Expected new-agent count over the project's lifetime: 0–2.** Most likely **`/config`** in Phase 3 (technology boundary); possibly **`/ivr`** in Phase 4 if IVR is built on-device rather than via an external gateway.

**Retraction / merge paths** (roster evolution runs both ways; protocol in `PM_TEMPLATES.md → ## PM Skill Template → ### Agent Retraction / Merge`):
- **`/web` → merge into `/mqtt`** if, through Phase 2, the web UI stays under ~5 SKILL.md bullets of genuine domain knowledge (i.e., it remains thin HTML-templating). Fold the `ESP8266WebServer`/`WebServer` divergence note into `/mqtt` as a control-surface bullet.
- **`/net` consolidation:** if OTA + mDNS shrink to a couple of stable bullets after Phase 1, they could fold under a renamed `/wifi` — but `Test`'s connection-event model (C4) keeps `/net` justified; unlikely to retract.

**Canonical ownership for cross-cutting rules** (one owner carries full text; others link, never restate — prevents drift):
- **Secrets handling** (no credential literals; `sysenv` build flags) — canonical owner **`/firmware`** (`BUILD.md`). `/scm` and `/net` link to it; `/review` audits against it.
- **Version-bump protocol** (`APP_VER` + per-module header-comment dates on release) — canonical owner **`/firmware`**. `/scm` references `/firmware` SKILL.md → "Version-bump protocol".
- **Non-blocking-loop invariant** — canonical owner **`/review`** (audit) with `/arch` (design rationale in `ARCHITECTURE.md`). `/power`'s SKILL.md references it for the grandfathered relay-pulse exception rather than restating the rule.
- **Boot-safe-GPIO invariant** — canonical owner **`/firmware`** (pin map in `HARDWARE.md`); `/power` references it for the electrical rationale.
