# DECISIONS

Only durable decisions that would be expensive or confusing to rediscover belong here.

## 2026-10-23 - APSIS identity is parity-first

**Decision:** The FM-1 port must preserve APSIS as a distinct instrument. Port the musical/state/timing relationships as close to 1:1 as practical, adapting only the physical interaction where FM-1 differs from browser mouse/touch.

## 2026-10-23 - Source reference is frozen

**Decision:** Study 126 / `apsis.html` SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21` is the behavioural source authority.

The attached LF `index.html` is the same source modulo LF/CRLF; no source relock occurred.

## 2026-10-23 - Dinghy baseline and product identity are pinned

**Decision:** Use Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`, tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`, and APSIS product `FM-1_972` until an explicit revalidation changes them.

## 2026-10-23 - Phase order is state -> transport -> sound

**Decision:** Musical/state semantics and the deterministic shared transport must precede final DSP. Sound code consumes APSIS semantics; it does not define them.

## 2026-10-23 - One shared state graph and one shared transport

**Decision:** STRING/DRIFT/CLOCK/SPIRAL/BLOOM/GRAVITY/KEEP/CHAIN/MOTHER are views of one state graph. Clock-derived musical time uses one sample-referenced fixed-capacity event timeline; modules do not get private clocks.

## 2026-10-23 - Transport time representation

**Decision:** Long-running time is 64-bit whole samples plus Q32 fractional sample, not one 64-bit Q32 absolute value.

## 2026-10-23 - Fixed-capacity audio-side structures

**Decision:** Audio scheduling/voice management uses fixed-capacity allocation-free structures. Capacities are evidence-driven through legal worst-case stress, not guessed.

M3c raised the shared heap from 256 to 512 only after 256 measurably starved. M3c late-bind pools remain 64 Spiral +64 Bloom because measured stress stayed within those bounds.

## 2026-10-23 - Browser timer jitter is not a feature

**Decision:** Preserve musical boundary semantics and late-binding intent, not incidental JavaScript `setTimeout` jitter. Native sample timing may implement the same causality deterministically.

## 2026-10-23 - Spiral/Bloom late binding is mandatory

**Decision:** Event time remains fixed while final musical pitch is resolved against the latest source-visible state at the source-defined late observation point.

Native desktop profile:
- Spiral target−80 ms
- Bloom occurrence target−45 ms
- Bloom petal target−50 ms with causal occurrence clamp

Source narrow/mobile values remain regression facts (Spiral −110 ms, Bloom petal −70 ms) but are browser timer-safety margins, not the FM-1 runtime profile.

## 2026-10-23 - KEEP is not a full preset

**Decision:** KEEP stores only the source-defined partial musical memory: String tension/anchors, Drift amount, Clock geometry, Spiral route, Tuning mode and Gravity harmony.

## 2026-10-23 - CHAIN preserves lane semantics

**Decision:** CHAIN remains a 32-cell musical-memory lane system with chance, transpose, morph, lane scopes and start/loop sync. Continuous morph is limited to source-defined String position and Drift amount.

## 2026-10-23 - MOTHER structural hold keeps mixer live

**Decision:** `->1` may hold structural edits, but `mother.*` mixer/mute controls and output/master remain live. Pending items preserve first insertion order; repeat param updates replace value in place; repeated toggles cancel; pending Gravity applies after ordered items.

## 2026-10-23 - Step-one resync defaults OFF

**Decision:** `stepOneResync` remains OFF by default. If enabled, resync is processed before Clock-linked String/Spiral events at Clock 1.

## 2026-10-23 - Shared Space/Cathedral topology

**Decision:** The final room is one shared Space/Cathedral return with per-source sends. Prototype compact modulated FDN and nested allpass/Schroeder alternatives before choosing. Do not introduce unrelated per-source reverbs.

## 2026-10-23 - Project persistence uses dedicated journal storage

**Decision:** Full APSIS project storage will use a dedicated versioned CRC-protected journal/two-slot strategy with safe write deferral. Dinghy's tiny settings slot is not the project store.

## 2026-10-23 - Verification language must be literal

**Decision:** Distinguish implemented, desktop-tested, sanitizer-tested, target-built, emulator-tested and physical-hardware-tested. Physical FM-1 remains unavailable.

## 2026-10-29 - Study 98 is a narrow String acoustic authority

**Decision:** Study 126 remains authority for String state/performance/timing/Gravity/routing/Classic-vs-Dual and later safety fixes. Study 98 (`old_index.html`, LF SHA `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`) is only the acoustic/timbre reference for String features that existed there.

Native String therefore restores the preferred old Foundation/Classic sine+Drift DNA and old partial/pluck character without restoring removed broadband random-noise bow excitation or rolling back newer behaviour.

## 2026-10-29 - M3c deterministic identity is locked

**Decision:** Canonical 30 s host replay uses seed `0x31415926` with random trace `78771ff5`, late trace `0e6fc422`, queue 352/512, Spiral slots 59/64 and Bloom slots 44/64. Extended max-swing stress reaches queue 357/512.

`ApsisRuntime` remains 21,928 B unless an explicitly scoped later phase changes it.

## 2026-10-30 - Drift persistent pair is a documented source-wiring repair

**Decision:** Native DRIFT is the identity-bearing paired beating field around the Gravity root even though late Study 126 no longer wires `beatPair()` as a persistent transport voice. Preserve the source delta/depth/level law and historical stereo-width law rather than making DRIFT silent.

**Why:** The port contract and earlier source history establish the paired field as DRIFT identity; the missing Study-126 transport connection is treated as a source wiring defect, not a new synth-design opportunity.

## 2026-10-30 - Clock consumes the shared timeline

**Decision:** M4e Clock audio passively consumes existing `CLOCK_MAIN`, `CLOCK_SNAP`, `CLOCK_INNER` and `CLOCK_GHOST` events. It may use DSP-local sample delay and deterministic pulse-0 bootstrap, but never another Clock scheduler/queue or second Ghost random draw.

## 2026-10-30 - Snap noise uses a local deterministic RNG

**Decision:** Native Clock Snap reuses a deterministic Clock-local noise realization and never consumes the frozen shared musical RNG.

**Why:** Study 126's one-time Snap buffer uses global unseeded `Math.random()` shared with musical systems; that coupling is unstable engine leakage rather than reproducible musical semantics.

## 2026-10-30 - Preserve exact integer DSP math on JieLi via target ABI shim

**Decision:** Keep frozen host fixed-point arithmetic unchanged. Where pi32v2 omits libgcc DI-mode division helpers, provide target-only standard ABI `__divdi3` / `__udivdi3` using shift/compare/subtract restoring division instead of weakening DSP math.

## 2026-10-30 - M4f Spiral reuses the frozen M3c lifecycle

**Decision:** Spiral audio does not schedule another occurrence or bind. M4f snapshots only the acoustic data that Study 126 freezes at planning time, keyed to the existing M3c token/slot, then consumes the existing target−80 ms late-bound pitch result.

Planning-time snapshot includes energy, duration/noteLength, source level, Drift delta/width, voice topology, attack/tail, filter Q and main/shadow topology. Route/Gravity/register/tuning remain M3c late-bound. The source edge case where a later voice change affects cutoff family but not topology/Q/envelope is preserved.

The +8 ms shadow remains DSP-local. `ApsisRuntime` does not grow.

## 2026-10-30 - M4f Spiral finite pool is 32 legs

**Decision:** Use 32 fixed Spiral legs. Evidence: eight-second legal worst acoustic stress reaches 30/32 legs, 15 logical hits and 90 oscillators with 0 steals and 0 drops. Reference stress hash is `02be4bc5`.

## 2026-10-30 - Inactive dry engines must preserve the frozen emulator callback budget

**Decision:** The pinned three-second emulator scenario must advance the shared runtime cursor to **123,648 samples exactly**, not merely nonzero. `ci/emulator_smoke.sh` enforces this.

M4f initially reduced the cursor to 123,392 even with the source-default empty Spiral route because the callback still cleared the external binding batch and scanned an empty 32-leg renderer every block. The accepted architecture keeps the exact M4e callback path until Spiral first has both sound and a non-empty route, then latches M4f active permanently so pending binds/tails remain safe.

**Why:** A silent/inactive new dry engine must not quietly consume one full 256-frame callback of the pinned emulator budget. This gate applies to later Bloom/Space work unless an explicitly justified scenario change deliberately rebaselines it.
