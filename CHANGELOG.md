# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-30 - M4f Spiral implementation complete

Active branch: `m4f-spiral`; draft PR #12.

Completed and implementation-gated:
- source-correct THREAD / GLASS / REED / HALO acoustic topology
- planning-time capture of energy, duration/noteLength, source level, Drift delta/width, voice topology and envelope/filter identity
- reuse of existing M3c Spiral token/slot and target−80 ms late bind; no new scheduler event or random draw
- external 64-slot acoustic snapshot/bind bridge; `ApsisRuntime` remains 21,928 B
- fixed 32-leg finite renderer with DSP-local +8 ms shadow
- deterministic host references: THREAD 1230/`aef6c83e`, GLASS 1157/`9efa9bdc`, REED 967/`e67a3c15`, HALO 1256/`e7ed6161`
- eight-second worst acoustic stress: 15 logical, 30/32 legs, 90 oscillators, 0 steals, 0 drops, hash `02be4bc5`
- real JieLi target and default FM-1 emulator
- exact M4e target/PCM/runtime Spiral-DSP bypass
- default route-empty callback-path fix restoring the frozen emulator cursor from 123,392 to 123,648
- permanent emulator assertion requiring cursor 123,648

Technically final implementation/CI head: `11569371ea18bc8a5e06f28d09ddef90f9835e29`.
- core #260 PASS
- staged #249 PASS all gates

Technically final M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`

Default emulator uses the source-default empty Spiral route, so PCM remains exact M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`; audible Spiral is verified by host/full-chain tests plus real target compile/link rather than this default listening scenario.

Status: implementation complete; exact final documentation-head double gate and PR #12 merge remain.

## 2026-10-30 - M4e Clock merged

PR #11 squash-merged to `main` as `db9d0b3ecbc7931be343cec7717103059d3c5160` after final documentation head `ba3c05e4…` passed core #231 + staged #220.

Frozen host Clock:
- STONE 2081 / `edcc4a81`
- GLASS 1880 / `0adf9829`
- SNAP 738 / `04bc514a`

Merged M4e target:
- image 90,684 B
- RAM 49,128 B
- pool 30,720 B
- package `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Clock-DSP bypass preserves frozen M4d PCM/runtime. Clock-local deterministic Snap noise does not perturb shared musical RNG.

## 2026-10-30 - M4d Drift merged

PR #10 squash-merged to `main` as `d7e39e2481c33628f402bed51d761b8be8304bb3` after exact final-head core #214 + staged #203 gates.

Frozen host Drift: 488 / `0aec6bc2`.
Final default M4d PCM: `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`.
Dedicated Drift-DSP bypass proved exact M4c package/PCM/runtime preservation.

## 2026-10-29 - M4c String Attractor merged

PR #9 merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.

Completed deterministic String Attractor S&H planning, shared-heap target−45 ms binds, source random-order preservation, finite BODY/WIRE/Classic excitation, legal capacity stress and real target/emulator regression.

## 2026-10-29 - M4b finite String + Rhythm merged

PR #8 merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

Added fixed finite String transients, Study-126 BODY/composite behaviour and passive shared `STRING_RHYTHM` audio handoff.

## 2026-10-29 - M4a persistent String merged

PR #7 merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Added the first persistent dry source engine with Study-98 Classic/Foundation acoustic identity, Study-126 Dual preservation, frozen host references and FM-1 emulator audio.

## 2026-10-29 - Phase 3 completed

- M3a merged `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b merged `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c merged `ec0dc0b350793572f19040efc938f00121128dc6`

M3c froze the 21,928-byte runtime, 512-entry shared heap, 64 Spiral slots, 64 Bloom slots and long deterministic random/late traces before DSP work began.

## 2026-10-23 - Phase 2 completed

M2c merge `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361` completed pure state semantics for KEEP, CHAIN, MOTHER and the earlier musical modules. Desktop, ASAN/UBSAN, real target and emulator gates passed.

## 2026-10-23 - Phase 1 completed

Established pinned Dinghy baseline, APSIS overlay shell, product `FM-1_972`, host build, real JieLi target build, staged CI and FM-1 emulator smoke.

## 2026-10-23 - Project reference freeze

Locked Study 126 behavioural source SHA `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`, port contract, architecture direction, parity matrix and source behaviour maps. Study 98 was later locked only as the narrow String acoustic reference.
