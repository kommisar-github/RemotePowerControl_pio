# Bug hunt — "WiFi=connected but mqtt loop() stops" (handoff to /review for adjudication)

**Date:** 2026-06-06
**Status:** Three specialist analyses collected; they AGREE on the mechanism but DISAGREE on permanence. `/review` to adjudicate by reading the actual code.

## Symptom (from the author)
Very rarely, the device reaches: `WiFi.status()==WL_CONNECTED` but `mqtt_client.loop()` stops being called → no keepalive PINGREQ → broker drops the client (LWT).

## DECISIVE new datapoints (author, 2026-06-06)
1. **The deadlock self-resolves after DAYS, without reboot.** Not ~20 s, not reboot-required — multi-day.
2. Broker-side wire behavior NOT confirmed (unknown whether MQTT is truly silent on the wire vs HA-stale-only).

→ This reframes the question. A **multi-day** self-resolving deadlock points to a **timer/counter overflow** mechanism, not a pure 20 s transient (net) nor a permanent lockup (firmware). Prime suspects: signed `long period = (unsigned long) - (unsigned long)` overflow (~24.8 days); `millis()` rollover (~49.7 days); or a slowly-clearing external condition.

---

## Agreed mechanism (all 3 specialists)
The gate: `main.cpp:320  if (wifiConnect.connectionEstablished()) mqttClient.handleClient();`
`WiFiConnect.cpp:300: connectionEstablished() = (WiFi.status()==WL_CONNECTED) && connection_established;`
- `mqttClient.handleClient()` is the ONLY handler behind a gate; everything else in loop() is unconditional.
- `connection_established` writers: `initial_connection_setup()`→true (only restorer after boot); `try_reconnect()`→false; `begin()`→false.
- `initial_connection_setup()` (restorer) is reached only from `handleClient()`'s `!connection_established` branch, gated by a 10 s timer (`TEST_CONNECTION_INTERVAL_NB=10000`) AND requires `WiFi.status()==WL_CONNECTED` at the poll instant.
- The MQTT wrapper itself is self-healing once the gate opens (reconnect every 10 s, resubscribe() inside reconnect()). Root cause is UPSTREAM in /net's gate.

## /firmware position (task b8591049)
Permanent stuck possible (Hyp A): `try_reconnect()`→`WiFi.reconnect()` (every 60 s in the recovery branch) repeatedly drops the association; every 10 s poll lands in a `WL_DISCONNECTED` window → gate never reopens. Also flagged **Hyp C: `long period = t - test_connection_lastActivity` (signed from unsigned) goes negative after ~24.8 days → `period >= 10000` never fires** (noted as edge case).
Fix direction: in the recovery branch don't call `WiFi.reconnect()` if `WiFi.status()==WL_CONNECTED`; `initial_connection_setup()` alone suffices.

## /net position (task 2019ae5e)
Gate self-heals within ≤20 s (between association drops at least one of ~6 polls/min sees WL_CONNECTED → reopen; `try_reconnect()` in the recovery branch fires at most every 60 s, not every 10 s). Ranks as the persistent symptom instead **H3 = net H2**: `mqtt_conn_timestamp` not cleared on WiFi-lost (`Test.cpp:188`) → on reconnect `EVENT_MQTT_RESTORED` suppressed → `refresh_all_switches_state()` never called → HA shows stale/offline though MQTT is alive. Also flagged Test.cpp bad-DHCP path (`Test.cpp:183`, networkID `192.168.66.0` / `!isIPAddressSet`) calling `try_reconnect()` every 60 s as a periodic gate-closer.
Note: net's H3 does NOT match the "loop() stops / no pings" symptom (H3 = MQTT alive but HA stale) — a symptom-interpretation gap to resolve.

## /mqtt position (task 5c56a956)
MQTT wrapper not the bug; self-heals when gate open; `_enabled` always true in prod (M-1 dormant). Root cause upstream at the `connectionEstablished()` gate. Confirmed net H2 corrupts event ordering but does NOT block recovery (resubscribe runs inside reconnect()).

---

## What /review must adjudicate
1. **Reconcile with the DAYS timescale.** Neither "≤20 s self-heal" (net) nor "permanent until reboot" (firmware) fits "self-resolves after days." Identify the exact mechanism that (a) closes the gate, (b) keeps it closed for DAYS, (c) reopens it after days. Read the ACTUAL code: `WiFiConnect::handleClient()` recovery branch, `initial_connection_setup()`, `try_reconnect()`, `reconnect()`, the timer variable TYPES (signed vs unsigned) and their update sites, `Test::test_connection()` bad-DHCP path, and PubSubClient keepalive default.
2. **Test the overflow hypotheses concretely:** does any timer comparison on the recovery path use a signed `long` holding an unsigned `millis()` difference (24.8-day sign flip), or depend on `millis()` rollover (49.7 days)? Does a sign-flip make `period >= 10000` (or a reconnect interval check) stop firing, freezing recovery until the next rollover/flip — i.e. days?
3. **Decide whether the "loop() stops / no pings" symptom is the gate (closed `connection_established`) or net H2 (HA-stale-but-alive), or both as siblings.**
4. **Give the precise root cause + minimal fix** (and whether the fix is owned by /net, /firmware, or both at the C4 boundary). Analysis only — no source edits.

---

## /review adjudication (2026-06-06)

**Verdict: ROOT CAUSE CONFIRMED — signed `period` sign-flip on the gate-reopen poll. `/firmware`'s Hyp C is correct (and is promoted from edge-case to primary by the DAYS datapoint); `/firmware`'s "permanent until reboot" and `/net`'s "≤20 s self-heal" are BOTH wrong. Owner: `/net`.**

### The exact mechanism (self-cited)

The gate-reopen poll lives in the `!connection_established` branch of `WiFiConnect::handleClient()`:

```
src/WiFiConnect.cpp:345  unsigned long t = millis();
src/WiFiConnect.cpp:346  if (!connection_established) {
src/WiFiConnect.cpp:347    long period = t - test_connection_lastActivity;   // SIGNED long = UNSIGNED diff
src/WiFiConnect.cpp:348    if (period >= TEST_CONNECTION_INTERVAL_NB) {       // 10000 ; NO `|| period < 0` guard
src/WiFiConnect.cpp:349      test_connection_lastActivity = t;
src/WiFiConnect.cpp:350      if (WiFi.status() == WL_CONNECTED) initial_connection_setup();   // the ONLY gate re-opener after boot
                            ...reconnect sub-branch...
```

Types (confirmed in `WiFiConnect.h`): `test_connection_lastActivity` is **`unsigned long`** (`WiFiConnect.h:97`); `t` is `unsigned long` (`:345`); but `period` is **signed `long`** (`:347`). On both MCUs `long`/`unsigned long` are 32-bit, `millis()` is 32-bit. So `t - test_connection_lastActivity` is computed modulo 2^32 and then reinterpreted as two's-complement: any true elapsed in **[2^31, 2^32) ms = [24.85 d, 49.71 d]** becomes **negative**, and `period >= 10000` is then **false** — the poll is skipped, `test_connection_lastActivity` is *not* refreshed (`:349` is inside the skipped block), so `period` stays negative and the gate-reopen poll is **frozen** until `millis()` rolls over.

**The smoking gun is the asymmetry within the same function:** the reconnect-throttle a few lines below *does* guard the sign flip —

```
src/WiFiConnect.cpp:363    period = t - last_reconnect_attempt;
src/WiFiConnect.cpp:364    bool reconnect_fl = (period >= RECONNECT_INTERVAL || period < 0 );   // <-- guarded
```

The author clearly knew negative `period` was possible (guarded `:364`) but **omitted the same `|| period < 0` on the gate-reopen poll `:348`.** That single missing clause is the bug.

### Why DAYS, and why it self-resolves without reboot

`test_connection_lastActivity` is only written in two places: `begin()` (`:129`, to `TEST_CONNECTION_INTERVAL_NB * -2` = `(unsigned long)-20000` ≈ `4 294 947 296`) and the gate-closed poll (`:349`). `setup()` uses the **blocking** `begin(SSID,PWD,HOST)` (`none_blocking=false`), which connects and calls `initial_connection_setup()` directly (`:214`) — the `handleClient()` recovery branch is **never entered during boot**, so `test_connection_lastActivity` keeps its `≈4.29e9` sentinel until the *first* gate closure.

Gate closure after boot happens when WiFi drops (`:382 → try_reconnect()` sets `connection_established=false`) or the hourly non-11N PHY retry fires. At that first closure, the recovery branch evaluates with the stale sentinel `L ≈ 2^32−20000`:

- `period = (long)(t − L) = (long)(t + 20000 mod 2^32)`.
- **uptime t < ~24.85 d:** `period` positive → poll fires every 10 s → reopens within ≤10 s of WiFi being up. *This is the common case and is exactly why the bug is RARE.* (`/net` saw only this case.)
- **uptime t ∈ [~24.85 d, ~49.71 d):** `t + 20000` ≥ 2^31 → `period` **negative** → poll frozen → gate stuck **closed**. It self-heals only when `t` reaches `2^32 − 20000` (**≈49.71 d uptime**), where `t+20000` wraps to a small positive value, `period ≥ 10000` fires again, and `:350` reopens the gate.

So a WiFi drop that happens to land in the device's **24.85–49.71-day uptime window** freezes the MQTT gate for the remainder of that window — **up to ~24.9 days, self-resolving at the `millis()` rollover, no reboot.** That is precisely "self-resolves after DAYS, without reboot." (The cb-timer block at `:373-374` does not rescue it — it uses its own stale sentinel `test_connection_cb_lastActivity` `:130` which sign-flips identically, and its `custom_callback` is `nullptr` under the 3-arg `begin()` used in `setup()`.)

**Why WiFi reads `WL_CONNECTED` during the freeze:** while frozen, the app never calls `WiFi.reconnect()` (that call is inside the frozen `:363-368` block), so the ESP's own auto-reconnect (default on) can restore the association independently. Result: `WiFi.status()==WL_CONNECTED` while `connection_established` stays `false` → `connectionEstablished()` (`:300-301`) returns false → `main.cpp:320` never calls `mqttClient.handleClient()` → no PubSubClient `loop()` → no PINGREQ → broker LWT-drops the client. **Exact match for the reported symptom.**

### Symptom adjudication: gate-freeze vs. net H2 — siblings, not the same

- **"loop() stops / no pings / broker LWT"** = the **frozen `connection_established` gate** (`mqttClient.handleClient()` is never called). This is THE matching root cause.
- **net H2** (`Test.cpp:188`, `mqtt_conn_timestamp` not cleared on WiFi-lost) makes MQTT **alive but HA-stale** — the MQTT `loop()` keeps running and PINGREQ keeps flowing; `refresh_all_switches_state()` is skipped. H2 does **not** stop pings, so it does **not** explain this symptom.
- They are **independent sibling defects**. Fixing H2 will not fix this deadlock, and fixing this deadlock will not fix H2. Both should be fixed.

### Reconciling the three specialists

- **`/firmware` Hyp C is right and is the answer** — it named "signed `long period = t − test_connection_lastActivity` goes negative after ~24.8 days → `period >= 10000` never fires." That is line 347-348 exactly. Its error was ranking Hyp C as an edge case behind Hyp A, and calling the result "permanent until reboot": it is **not** permanent — it self-heals at the ~49.7 d rollover.
- **`/net` "≤20 s self-heal" is wrong in the overflow window** — that reasoning silently assumes `period` is always positive, which holds only for uptime < ~24.85 d. `/net` correctly found H2 but H2 is a different symptom.
- **`/firmware` Hyp A** (60 s `WiFi.reconnect()` re-dropping the link) is a plausible aggravator *before* the freeze but is not required for, and does not explain, the days-then-heal behavior. Secondary.

### Minimal fix + owner

**Owner: `/net`** (`src/WiFiConnect.cpp` is `/net`'s file; no C4-boundary change needed — the fix is entirely inside `WiFiConnect`).

- **One-line minimal patch (matches the existing idiom at `:364`):** on `:348` change
  `if (period >= TEST_CONNECTION_INTERVAL_NB)` → `if (period >= TEST_CONNECTION_INTERVAL_NB || period < 0)`.
- **Correct/robust fix (recommended):** make duration deltas **unsigned** so the sign-flip class cannot exist —
  `unsigned long period = t - test_connection_lastActivity; if (period >= TEST_CONNECTION_INTERVAL_NB)`. Unsigned subtraction is rollover-correct and never negative. Apply the same to the sibling signed-`period` deltas: `:386` (`test_try_N_lastActivity`, same latent freeze on the hourly 11N retry, lower impact), and for consistency `:347/:363` and the boot loop `:201`. The now-dead `|| period < 0` clauses can be dropped.
- **Systemic note:** the pattern `long period = (unsigned)millis() - (unsigned)stamp; if (period >= INTERVAL)` recurs at `WiFiConnect.cpp:201, 347, 363, 386`. Recommend a `/net` GUIDELINES entry: *"millis() interval math must use `unsigned long`; never assign a `millis()` difference to a signed type."* This is a numeric-type cousin of the audit's "silently-ignored-config" family and should be captured in the next consolidation wave.

### Residual uncertainty (stated honestly)

1. **Broker wire behavior is unconfirmed** (author noted). The "no PINGREQ → LWT" step is inferred from the gate logic and PubSubClient's keepalive model, not observed on the wire. The firmware-side mechanism (gate frozen → `handleClient()` not called) is certain; the exact broker disposition is consistent but unverified.
2. **Exact freeze duration per incident depends on `t_drop`** (when the WiFi drop lands in the bad window) and on the history-dependent value of `test_connection_lastActivity` at that moment. The first-drop-after-boot case (stale `−20000` sentinel) is the cleanest and most likely; later cycles shift the window but the defect class is identical and recurs roughly once per ~49.7 d of uptime if a drop lands in the bad half-window.
3. **The `WL_CONNECTED`-during-freeze observation depends on the underlying stack auto-reconnect succeeding** without the app's `WiFi.reconnect()`. This is the default behavior and matches the author's report; if the stack did not auto-reconnect, the same freeze would instead present as "stuck, WiFi down" rather than "WiFi connected." Either way the gate-freeze is the defect.

