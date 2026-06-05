## Section 2 — Design principles

**Why specialists, not just parallelism.** This is a solo-operator embedded project, so true N-terminal parallelism is largely moot (one attention head). The roster earns its keep through **knowledge routing**: each specialist is a persistent holder of one hard-won domain that a generic agent would re-derive (often wrongly) every dispatch. The firmware is small in line count but **dense in non-obvious, vendor-specific knowledge** — that density, not file volume, is what justifies splitting it.

Concretely, the four domain hubs hold knowledge that does not transfer:

- **`/power`** holds the `RSwitch` state machine and the *electrical* reading of the detect GPIO: optocoupler pull-up means **LOW = power ON, HIGH = power OFF**; rapid pulsing = the PC's sleep/standby LED; the magic timings (`PULSE_PRECESION_MILLS`, `LONG_PULSE_DURATION_DETECT_SEC`, the >5-short-pulse SLEEP threshold, 5-minute periodic re-validation). Get any of these inverted and the device reports the opposite of reality.
- **`/mqtt`** holds the Home-Assistant-facing topic grammar (`<area>/<host>/power/{state,status}/<idx>`, retained-vs-not per topic), the `<0|1|3><switch_idx>` set-payload byte layout, LWT semantics, and the PubSubClient buffer/recone idioms.
- **`/net`** holds the WiFi/OTA/mDNS transport: reconnect gating, PHY-mode switching, ArduinoOTA's `espota` flow, and the ESP8266-vs-ESP32 mDNS include divergence and the `Test` connection-health event model.
- **`/firmware`** holds the build/platform layer: PlatformIO env matrix, `sysenv` secret injection, boot-safe pin maps, and the `loop()`/`setup()` init ordering that wires every module together.

If any of these collapsed to "runs a few functions," it would lose the warm context that makes its dispatches cheap and correct. The 5-bullet hard qualifier (§3) is met comfortably by all four — none is a dumping ground.

**Context economy via the matrix.** Each design/reference doc carries a ~50-token `## Abstract` (TL;DR / Load-when / Key-facts / Owner / Related). PM cites `Context docs:` per dispatch, so a fork pays only for the knowledge it needs. Example: a `/power` fork dispatched to *"tune the SLEEP-detection pulse threshold"* loads `DOC_OWNERSHIP_MATRIX.md` + `POWER_STATE.md` + `HARDWARE.md` (≈ 18–22K), **not** `MQTT_API.md`, `BUILD.md`, or `ARCHITECTURE.md` in full — which a cold fork would pull (≈ 60–70K) just to find the relevant constants. A `/web` fork updating the toggle UI loads only `ARCHITECTURE.md` (web-route section) + the relevant interface contract, never the RSwitch timing internals.

**Relationship to default agents.** `PM.md` ships four defaults — **pm, arch, review, scm** — kept structurally identical to the templates; only project-specific context is injected. `scm` needs essentially zero customization (git is git). What we inject:

| Default | Injected at bootstrap |
|---|---|
| **pm** | The 9-agent roster + §7 interface contracts + §8 wave map (from `README.txt` TODOs) + the matrix + the three project invariants and their owners. PM routes by the §6.4 quick-reference. |
| **arch** | The module/component map (`ARCHITECTURE.md`): the `setup()` init order, the cooperative `handleClient()` loop contract, module boundaries, and the dual-MCU `#ifdef` convention. Owner of `ARCHITECTURE.md`. |
| **review** | A project-specific risk checklist: RAM/heap fragmentation, the non-blocking-loop invariant (flag new `delay()` in the loop path), boot-safe-GPIO invariant, secrets-never-committed invariant, retained-topic correctness, ESP8266-vs-ESP32 parity on every change. |
| **scm** | Nothing beyond defaults. Note in its SKILL.md only that the build is PlatformIO and that `version` strings (`APP_VER`, per-module header comments) bump on release — canonical text owned by `/firmware`, referenced not restated. |
