# DECISIONS

Only durable decisions that would be expensive or confusing to rediscover belong here.

## 2026-10-23 - APSIS identity is parity-first

**Decision:** The FM-1 port preserves APSIS as a distinct instrument. Port musical/state/timing relationships as close to 1:1 as practical; adapt only interaction where FM-1 differs from browser mouse/touch.

## 2026-10-23 - Source reference is frozen

**Decision:** Study 126 / `apsis.html` SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21` is behavioural/state/timing/safety authority. The attached LF `index.html` is the same source modulo LF/CRLF.

## 2026-10-23 - Dinghy baseline and product identity are pinned

**Decision:** Use Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`, tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`, and APSIS product `FM-1_972` until explicitly revalidated.

## 2026-10-23 - Phase order is state -> transport -> sound

**Decision:** Musical/state semantics and deterministic shared transport precede final DSP. Sound consumes APSIS semantics; it does not define them.

## 2026-10-23 - One shared state graph and one shared transport

**Decision:** STRING/DRIFT/CLOCK/SPIRAL/BLOOM/GRAVITY/KEEP/CHAIN/MOTHER are views of one state graph. Clock-derived musical time uses one sample-referenced fixed-capacity timeline; modules do not get private clocks.

## 2026-10-23 - Transport time representation

**Decision:** Long-running time is 64-bit whole samples plus Q32 fractional sample, not one 64-bit Q32 absolute value.

## 2026-10-23 - Fixed-capacity audio-side structures

**Decision:** Audio scheduling/voice management uses allocation-free fixed-capacity structures. Capacities are evidence-driven by legal worst-case stress. M3c raised the shared heap from 256 to 512 only after measured starvation.

## 2026-10-23 - Browser timer jitter is not a feature

**Decision:** Preserve musical boundary semantics and late-binding intent, not incidental JavaScript timer jitter. Native sample timing implements the same causality deterministically.

## 2026-10-23 - Spiral/Bloom late binding is mandatory

**Decision:** Event time remains fixed while final musical pitch resolves against source-visible state at the source-defined late observation point.

Native desktop profile:
- Spiral target−80 ms
- Bloom occurrence target−45 ms
- Bloom petal target−50 ms with causal occurrence clamp

Spiral does not project ordinary pending Gravity. Bloom may project ordinary pending Gravity target-aware under the frozen +0.5 ms rule. Mother-held future state is not projected.

## 2026-10-23 - KEEP / CHAIN / MOTHER semantics are source-defined

**Decision:** KEEP stores only source-defined partial musical memory. CHAIN remains the 32-cell lane system with chance/transpose/morph/scope/start-loop sync; continuous morph is source-limited to String position and Drift amount. MOTHER `->1` may hold structural edits while mixer/mute/output remain live.

## 2026-10-23 - Step-one resync defaults OFF

**Decision:** `stepOneResync` remains OFF by default. When enabled, resync precedes Clock-linked String/Spiral events at Clock 1.

## 2026-10-23 - Shared Space/Cathedral topology

**Decision:** Final room is one shared Space/Cathedral return with per-source sends. Prototype compact modulated FDN and nested allpass/Schroeder alternatives before choosing. Do not introduce unrelated per-source reverbs.

## 2026-10-23 - Project persistence uses dedicated journal storage

**Decision:** Full project storage will use versioned CRC-protected journal/two-slot persistence with safe write deferral. Dinghy's tiny settings slot is not the project store.

## 2026-10-23 - Verification language must be literal

**Decision:** Distinguish implemented, desktop-tested, sanitizer-tested, target-built, emulator-tested and physical-hardware-tested. Physical FM-1 remains unavailable.

## 2026-10-29 - Study 98 is a narrow String acoustic authority

**Decision:** Study 126 remains String behaviour/state/timing/Gravity/routing/Classic-vs-Dual/safety authority. Study 98 (`old_index.html`, LF SHA `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`) is only the acoustic/timbre reference for String features that existed there.

Native String restores old Foundation/Classic sine+Drift DNA and partial/pluck character without restoring removed broadband random-noise bow excitation or rolling back later safety fixes.

## 2026-10-29 - M3c deterministic identity is locked

**Decision:** Canonical replay uses `sizeof(ApsisRuntime)=21,928 B`, 512-event heap, seed `0x31415926`, random trace `78771ff5`, late trace `0e6fc422`, queue 352/512, Spiral 59/64, Bloom 44/64. Extreme max-swing reaches 357/512.

## 2026-10-30 - Drift persistent pair is a documented source-wiring repair

**Decision:** Native DRIFT is the paired beating field around the Gravity root even though late Study 126 no longer wires `beatPair()` as a persistent transport voice. Preserve source delta/depth/level and historical stereo-width law rather than making DRIFT silent.

## 2026-10-30 - Clock consumes the shared timeline

**Decision:** Clock audio passively consumes existing `CLOCK_MAIN`, `CLOCK_SNAP`, `CLOCK_INNER`, `CLOCK_GHOST` events. No second Clock scheduler/queue or Ghost random draw.

Snap uses deterministic Clock-local noise rather than consuming the shared musical RNG; browser global `Math.random()` coupling is unstable engine leakage.

## 2026-10-30 - Preserve exact integer DSP math on JieLi via target ABI shim

**Decision:** Keep frozen host fixed-point arithmetic unchanged. Where pi32v2 omits libgcc DI-mode division helpers, provide target-only `__divdi3` / `__udivdi3` via restoring division rather than weakening DSP math.

## 2026-10-30 - M4f Spiral reuses the frozen M3c lifecycle

**Decision:** Spiral audio never schedules another occurrence or bind. M4f snapshots only source-defined planning-time acoustic data keyed to the existing M3c token/slot, then consumes the existing target−80 ms late-bound pitch. `ApsisRuntime` does not grow.

M4f pool is fixed at 32 legs after legal stress reached 30/32 legs, 15 logical hits and 90 oscillators with 0 steals/drops (`02be4bc5`).

## 2026-10-30 - Inactive dry engines preserve exact emulator callback budget

**Decision:** Pinned three-second emulator must advance shared runtime cursor to **123,648 samples exactly**. A silent/inactive new engine may not consume a full 256-frame callback of budget. This remains a regression gate unless an explicitly justified scenario rebaselines it.

## 2026-10-30 - M4g Bloom reuses M3c occurrence and petal-bind causality

**Decision:** Bloom audio creates no second lifecycle. M4g mirrors existing M3c Bloom token/slot state externally: occurrence freezes only source-defined acoustic geometry; each existing petal bind supplies authoritative target-aware Gravity/tuning. No M4g random draw, queue event or `ApsisRuntime` growth.

Bloom finite pool is 136 petals after legal stress reached 125 simultaneous petals / 375 oscillators / 0 drops (`5881bb36`).

## 2026-10-30 - Bloom activation preserves exact M4f idle path

**Decision:** M4g stays on the exact M4f callback path until an already-planned Bloom occurrence is due. Once activated it remains latched so pending petals/tails survive later edits. The pinned default scenario therefore remains exact M4f PCM/cursor even with Bloom target code linked.

## 2026-10-30 - Static RAM is a hard Phase-5/6 constraint

**Decision:** M4g real target uses 87,336 / 98,304 B static RAM, leaving **10,968 B**. Phase 5/6 must not add large unconditional BSS without measured need. Memory savings may not silently trade away parity.

## 2026-10-30 - Phase 5 is verification-only when the existing architecture is already correct

**Decision:** Do not change production scheduler/DSP/runtime merely to manufacture a phase diff. P5a–P5f prove source-defined structural semantics end-to-end at the actual sounding DSP boundary. Because M3c + M4f/M4g already satisfy those contracts, Phase 5 adds tests/contracts/workflow gates but **no production musical or audio code**.

**Evidence:** final head `17e7fd1247b0f5675261a7395b0558b5bbd6780d` passed core #363 and staged #352 and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`; the real target remained exact M4g.

## 2026-10-30 - Sounding identity stays in the layer that owns it

**Decision:** Phase-5 tests must read scheduler/attractor identity from the existing M3c slot/token rather than expanding M4f/M4g acoustic snapshots with duplicate structural fields merely for test convenience.

**Why:** occurrence/acoustic snapshots should contain only source-frozen acoustic data. Route/trigger identity and late-bound structural state remain owned by the shared runtime. This keeps Phase-4 contracts narrow and avoids redundant state that can diverge.

## 2026-10-30 - Phase 6 starts with topology + memory prototypes, not immediate reverb integration

**Decision:** Freeze the Study-126 Space/Cathedral contract and compare compact modulated FDN and nested allpass/Schroeder candidates before integrating a room into `app_audio()`.

The result must remain one shared room with five per-source sends. Delay-memory placement is an explicit design problem; do not assume a large unconditional BSS delay network fits.

## 2026-10-30 - Phase 6 selects the compact 8-line modulated FDN

**Decision:** Use the deterministic 8-line modulated FDN as the native Cathedral tail topology; reject the tested nested allpass/Schroeder candidate for this port iteration.

**Why:** Across 4 / 8.4 / 12 s, the FDN tracks the Study-126 statistical proxy on decay endpoint, stereo decorrelation and HF density while keeping metallicity around 0.11. The Schroeder proxy measures roughly 0.69–0.73 metallicity and diverges strongly at long decay. The FDN also uses 38,340 B tail storage versus 50,900 B.

Selected geometry is frozen in `docs/reference/P6_SPACE_PROTOTYPE_RESULTS.md`: delays `1871,1999,2137,2281,2441,2593,2767,2953`, +16 reserve per line, deterministic low-rate modulation and normalized Hadamard feedback.

## 2026-10-30 - Space owns a dedicated pre-.pool NOLOAD region

**Decision:** Long-lived Space delay memory must be placed in a dedicated NOLOAD `.apsis_space_pool` section immediately **before** Dinghy's ordinary `.pool`, within the same hardware POOL region. It must not be placed inside Dinghy's ordinary zeroed `.pool` range.

**Why:** pinned Dinghy `fm1_cstart()` clears `[_pool_start, _pool_end)` at boot. Adding 274,716 B to ordinary `.pool` delayed startup by one 256-frame callback even with no room renderer active. A runtime-retention call was not the root cause; link-time retention inside ordinary `.pool` still produced cursor 123,392.

With Space before `_pool_start`, real target accounting still sees the complete footprint through `_pool_end`, but Dinghy clears only its original pool. Gate head `11ce731a740d11262d988c352a79774bbea3103a` passed core #375 + staged #364 with image 100,944 B, RAM 87,336 B, pool 305,436 / 344,064 B, exact dry PCM `6c3b8fff…aa2b`, cursor 123,648 and frozen runtime trace.

## 2026-10-30 - Space delay memory is self-initialising

**Decision:** Production Space may not depend on CRT zeroing or perform a giant boot/start memset over its 274,716 B NOLOAD storage. Delay histories must track validity/fill and return silence for unwritten history until naturally filled.

**Why:** this preserves the frozen dry startup/callback path and makes room activation cost explicit. Input history, echo and each FDN line own their write/fill state; unread NOLOAD bytes are never interpreted as audio.
