## Section 9 — Agent interaction map

Unlike a pure fan-out project, this firmware has **standing cross-specialist arrows** worth diagramming — the callback wiring (§7) creates real producer→consumer edges that meet inside `main.cpp`. PM dispatches to everyone; the labelled edges are the §7 contracts.

```
                              ┌─────────┐
                              │   pm    │  (coordinator, 1M)
                              └────┬────┘
              dispatch / reconcile │ (to every agent)
        ┌──────────┬──────────┬────┴─────┬──────────┬──────────┐
        ▼          ▼          ▼          ▼          ▼          ▼
   ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
   │  arch  │ │ review │ │  power │ │  mqtt  │ │  net   │ │firmware│
   └────────┘ └────────┘ └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘
   (design)   (audit all)    │          │          │          │
                             │          │          │      owns main.cpp
                             │          │          │      shell + wiring
        ┌──────┐             │          │          │          │
        │ web  │◀──C5 RSwitch API───────┘          │          │
        └──────┘                        │          │          │
                                        │          │          │
   power ──C2 status-change cb──▶ main.cpp ◀── owns shell ── firmware
   mqtt  ──C3 set→actuate──▶ power         (C6 shared-file: 3 owners)
   net(Test) ──C4 conn events──▶ main.cpp ──▶ mqtt re-publish + power refresh
   mqtt ──C1 topic/payload grammar──▶ HA / web / firmware
        scm: invoked by pm for commits/tags; no standing data edge.
```

**How to read it:** every specialist receives work from PM (top fan-out). The bottom block shows the §7 contracts as directed edges — `power→main→mqtt` (C2), `mqtt→power` (C3), `net→main→{mqtt,power}` (C4), `power→web` (C5), and `main.cpp` as the C6 shared file with three section-owners. `arch`/`review` have no data edges (they design/audit), and `scm` has none (mechanical). The hub is `main.cpp` — which is exactly why C6 exists.
