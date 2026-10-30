# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-30 - M4g Bloom implementation complete

Active branch: `m4g-bloom`; draft PR #13; base is merged M4f `734229f4d23626a176a8366de82aab2c58c48cc9`.

Completed and implementation-gated:
- source-correct occurrence-time Bloom acoustic planning
- reuse of existing M3c target / target−45 ms occurrence / causal target−50 ms petal-bind lifecycle
- external 64-slot Bloom acoustic snapshot/bind bridge; `ApsisRuntime` remains 21,928 B
- per-petal late read of current VEIL/CHOIR/ORGAN/GLASS voice, attack and brightness while M3c remains pitch/tuning authority
- deterministic fixed-point finite Bloom renderer
- evidence-driven **136-petal** pool; legal stress reaches 125/136 petals and 375 oscillators with 0 drops
- frozen host refs VEIL 175/`6177470f`, CHOIR 143/`4b2969f8`, ORGAN 122/`d54a68d9`, GLASS 129/`ca1c3127`
- legal stress hash `5881bb36`
- real JieLi target and FM-1 emulator integration
- exact inactive/default callback path until an already-planned Bloom occurrence is due
- exact M4f target/package/PCM/runtime Bloom bypass

Technically final implementation/CI head: `6c54d1cd5ca354543237a14c257b3ec28ab424d1`.
- core #292 PASS
- staged #281 PASS all gates

Technically final M4g target:
- image 100,944 B
- RAM 87,336 B
- RAM headroom 10,968 B
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`

The pinned three-second default emulator does not reach a due Bloom occurrence, so PCM/runtime remain exact M4f: PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`, cursor 123,648, queue 28/512 and late trace `811c9dc5`.

Status: implementation complete; exact final documentation-head double gate and PR #13 guarded merge remain.

## 2026-10-30 - M4f Spiral merged

PR #12 squash-merged to `main` as `734229f4d23626a176a8366de82aab2c58c48cc9` after final documentation head `744559f89df733c4c167367efe30250b6dcdecdf` passed core #261 + staged #250.

Frozen host:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- worst stress 15 logical hits, 30/32 legs, 90 oscillators, 0 steals/drops, hash `02be4bc5`

Merged M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`

Default source route is empty, so PCM remains exact M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`. The exact cursor assertion remains 123,648.

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
