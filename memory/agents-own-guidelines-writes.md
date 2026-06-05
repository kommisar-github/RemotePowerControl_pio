---
name: agents-own-guidelines-writes
description: In the consolidation gate, the owning agent writes its own GUIDELINES file — PM runs the gate and commits, but does not author the content.
metadata:
  type: feedback
---

When consolidating audit/knowledge into `doc/<agent>_GUIDELINES.md`, the **owning
specialist agent writes its own file**. PM's role is to run the `/review`
consolidation gate and then git-commit (provenance) — NOT to author or write the
GUIDELINES content directly, even when PM has the approved delta in hand.

**Why:** The user corrected this on 2026-06-05 after PM wrote `power`/`mqtt`
GUIDELINES itself. GUIDELINES is each agent's own sanctioned write target
(SKILL.md Memory Policy); ownership must stay with the agent so the knowledge is
genuinely the specialist's and the boundary doesn't erode.

**How to apply:** After `/review` approves the deltas, dispatch each owning agent
to write its own `doc/<agent>_GUIDELINES.md` (passing any `/review` edits to
apply). PM then commits via `/scm`. The one allowed exception is drafting content
when an agent's terminal is broken (e.g. `/web` returned empty twice) — and even
then route it back through the agent to write the file if at all possible. See
[[dispatch-plan]] context in PM memory for the active wave.
