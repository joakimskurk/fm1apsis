# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-30 - M4e Clock implementation complete

Active branch: `m4e-clock`; draft PR #11.

Completed and implementation-gated:
- source-correct STONE / WOOD / GLASS / SOFT Clock acoustic state
- 0.50×..2.00× pitch represented as `pitch_centi=50..200`
- passive consumption of existing shared MAIN / SNAP / INNER / GHOST events
- fixed 32-slot Clock finite pool; measured worst legal stress 23/32, 176 triggers, 0 drops
- source-derived four kick profiles, GLASS accent overtone and Snap topology
- deterministic Clock-local Snap noise that does not perturb shared musical RNG
- target-only JieLi `__divdi3` / `__udivdi3` compatibility shim preserving exact integer DSP arithmetic
- deterministic host references: STONE 2081/`edcc4a81`, GLASS 1880/`0adf9829`, SNAP 738/`04bc514a`
- core #225 PASS normal + ASAN/UBSAN
- staged #214 PASS through real JieLi target + default emulator + Clock-DSP bypass

Implementation-head target:
- image 90,684 B
- RAM 49,128 B
- pool 30,720 B
- package SHA `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- Clock-on PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Clock-DSP bypass returns image/RAM/pool and PCM/runtime exactly to frozen M4d behavior; bypass PCM is `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`.

Status: implementation complete; exact final documentation-head double gate and PR #11 merge remain.

## 2026-10-30 - M4d Drift merged

PR #10 squash-merged to `main` as `d7e39e2481c33628f402bed51d761b8be8304bb3` after exact final-head core #214 + staged #203 gates.

Frozen host Drift reference: peak 488 / `0aec6bc2`.
Final default M4d PCM: `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`.
Dedicated Drift-DSP bypass proved exact M4c package/PCM/runtime preservation.

## 2026-10-29 - M4c String Attractor merged

PR #9 merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.

Completed:
- deterministic String Attractor S&H planner and shared-heap bind events
- source random-order preservation before Spiral/Clock ghost
- finite BODY/WIRE/Classic excitation through the shared String transient pool
- legal stress: shared heap 415/512, pending 60/64, zero overflow/starvation
- frozen random/late traces `b7a9a395` / `e777a771`
- real target + emulator regression

## 2026-10-29 - M4b finite String + Rhythm merged

PR #8 merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

Added fixed finite String transients, Study-126 BODY/composite behavior and passive shared `STRING_RHYTHM` audio handoff.

## 2026-10-29 - M4a persistent String merged

PR #7 merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Added the first persistent dry source engine with frozen Classic/Dual host references and FM-1 emulator audio.

## 2026-10-23 - M3a checkpoint started

Active branch: `m3-transport-core`.

Added / in progress:
- sample-referenced APSIS transport core
- fixed 256-entry event min-heap
- long-running whole-sample + Q32 fractional timeline
- browser swing/division/Clock-cycle semantics
- event generation for Clock-linked APSIS systems
- Gravity boundary prediction
- transport regression test suite
- native String timing-state representation corrections found during scheduler audit

Status at that checkpoint: implemented/committed but not yet fully CI-validated or target/emulator integrated.

## 2026-10-23 - M2c merged

Merge commit: `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

Completed Phase 2 state semantics for:
- KEEP partial musical memory
- CHAIN cell/lane/morph model
- MOTHER held structural-change queue and horizon behaviour

Verification:
- strict desktop tests PASS
- ASAN/UBSAN PASS
- unchanged Dinghy baseline PASS
- APSIS single-TU host PASS
- real JieLi target PASS
- emulator smoke PASS

M2c package SHA-256: `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`.

## 2026-10-23 - M2b completed

Added source-derived:
- Gravity point defaults/model
- ring/voicing helpers
- Bloom voice-leading planner
- related regression coverage

## 2026-10-23 - M2a completed

Added source-derived:
- complete initial APSIS state/defaults
- scale/tuning helpers
- String pitch model
- Drift delta model
- Clock structural model
- Spiral route and derived sequence
- deterministic attractor state

## 2026-10-23 - Phase 1 completed

Established:
- pinned Dinghy baseline
- APSIS overlay shell
- unique target identity `FM-1_972`
- host build
- real JieLi FM-1 target build
- staged CI
- FM-1 emulator smoke

## 2026-10-23 - Project reference freeze

Locked:
- browser `apsis.html` as behavioural source
- source SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- port contract
- architecture direction
- parity matrix
- source/behaviour maps
- FM-1 UI-mapping proposals
