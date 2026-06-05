## Section 12 — Open gaps

Decisions only the user can make. Each is referenced by a forward callout in §1 where it gates earlier content.

- **`Q:` (Q-MEMORY)** Should bootstrap **seed `doc/MEMORY.md`** now with the known facts (optocoupler polarity, the detect-pin-disconnected bug, the dual-MCU pin maps), or leave it a stub that PM populates on the first phase close? Gates whether §6.1's `MEMORY.md` row is populated at t0. *(Recommendation: seed it — those three facts are non-obvious and bug-causing.)*
- **`Q:` (Q-MARKERS)** Should the one-time **in-file section banners** (`// === /mqtt section ... ===`) be inserted into `main.cpp` during bootstrap (the v0.3.18 default), or kept only in the §7 C6 table + matrix? Gates §7 C6 and §10. *(Recommendation: insert — `main.cpp` is genuinely multi-owner and forks open it often.)*
- **`Q:` (Q-HA-DISCOVERY)** Does the project want **Home Assistant MQTT auto-discovery** (device publishes its own HA config topics) added in Phase 1, or does HA stay manually configured against the documented topic grammar? Gates whether §8 Phase 1 adds a `/mqtt` discovery wave.
- **`Q:`** Is the **office vs home** build split (`build_flags_office`/`SMARTHOME_*_OFFICE`) still active, or legacy? It affects whether `BUILD.md` documents two network profiles or one, and whether `/firmware` carries the office flags as live knowledge.
- **`Q:`** Is the dormant **`detect_power_status_new()`** state-machine variant intended to replace the active `_old()` implementation, or is it abandoned experimental code to delete? Affects `/power`'s `POWER_STATE.md` (document both vs. remove one) and a likely Phase 1 cleanup wave.
- **`Q:`** Should the blocking relay pulses (`short_push` 800 ms, `long_push` 10 s `delay()`) be **refactored to non-blocking** within v1 (they violate the non-blocking-loop invariant and stall OTA/MQTT during a force-off), or is the blocking behavior acceptable given pulses are user-initiated and infrequent? Affects whether a `/power` refactor wave is added to Phase 1.
- **`Q:`** Target **board variants** beyond `d1_mini` (ESP8266) and `wemos_d1_mini32` (ESP32)? If more boards are planned, `HARDWARE.md`'s pin table and the boot-safe-GPIO invariant need per-board sections.

---

### Pre-handoff self-check (producer-side rubric)

1. **Agent count matches across artifacts** — §3 has 9 `####` agent blocks (pm, arch, review, scm, power, mqtt, net, web, firmware); §4 `agents.json` has 9 entries; §5 SKILLS.md table has 9 rows. ✓
2. **Coordinators have no `model` field** — `pm`, `arch`, `review` carry no `"model"` key in §4. ✓
3. **Every specialist has a `model` field** — scm (haiku), power/mqtt/net/web/firmware (sonnet) all explicit. ✓
4. **Phase count matches roadmap** — §8 covers Phase 0–4; §1's v1 boundary + post-v1 IVR note map 1:1 (Phases 1–3 = v1 README items, Phase 4 = post-v1 IVR). ✓
5. **Non-delegable waves iff Learning-First** — project is not Learning-First (§8 states so); §8 contains no `(author, /learn reviews)` markers. ✓
6. **Every specialist SKILL.md block has ≥5 concrete bullets** — power 12, mqtt 11, net 10, firmware 11, web 6; all concrete (API names, pin numbers, magic constants, vendor pitfalls), none generic. ✓
7. **Every common candidate has an explicit decision** — §3 declines `/devops`, `/db`, `/security`, `/perf`, `/release` with rationale, plus domain candidate `/hardware` (declined → promoted to a doc). ✓

**Pre-handoff checks: 7/7 pass.**

**Deviation callouts** (per `BOOTSTRAP_PROMPT.md` Common-deviations appendix, stated explicitly rather than silent):
- Wave maps are **mostly Serial** because the v1 scope has one load-bearing specialist per milestone and a single shared-scope `loop()` runtime where most work converges on `main.cpp`.
- **`scm` has an empty domain-knowledge block** — git is git; the only note is a reference to `/firmware`'s version-bump protocol.
- **`/web` SKILL.md is 6 bullets** because the domain is two files of standard `ESP8266WebServer` + PROGMEM HTML; padding to match the quirk-dense specialists would dilute signal.
- **Single-repo, no matrix appendices** — the ESP8266/ESP32 dual-target is `#ifdef`, not separate platforms with separate roadmaps; Appendix A/B intentionally omitted, not stubbed.
