# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-11-01 - Phase 9 physical interaction parity complete, merge pending

Technical firmware head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e` passed the complete technical matrix: core #503, staged #492, P6-final #95, P7-storage #82, P7-target #75, P8-visuals #85, P8-target #60, P9-controls #82, P9-H4b #6 and P9-target #40.

Completed:
- pinned FM-1 interaction vocabulary: 14 buttons, 7 relative/detent encoders, 27 keys; MASTER remains platform output gain and no encoder-push is assumed
- direct surface navigation for MOTHER, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, GRAVITY, CHAIN, KEEP and SEL+FX -> shared SPACE/CATHEDRAL
- deterministic CLOCK, GRAVITY, STRING, SPIRAL, BLOOM, KEEP/CHAIN, MOTHER, SPACE and DRIFT/TUNING control contracts
- MOTHER-aware structural apply/queue/commit bridge and Gravity requests through the existing shared runtime
- relative encoder edits without hidden absolute pickup state
- project-dirty coalescing through the existing Phase-7 deferred/journaled persistence path
- P9-only manual STRING audio over the existing finite String transient pool, using Study-126 keyboard level 0.080 and a one-byte main->audio mailbox
- P9-only manual BLOOM petal/core/grow audio over the existing M4g binding/DSP engine
- direct Bloom petal/grow source gain distinction, display/pending-Gravity behavior and finite late-bound core petals
- shared M3c RNG for manual Bloom core; no private UI RNG or musical scheduler
- real JieLi target build and exact frozen P8 idle PCM/runtime emulator regression

Frozen technical candidate:
- source `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`
- image 132,264 / 581,564 B
- RAM 89,492 / 98,304 B
- POOL 309,532 / 344,064 B
- package `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

A Phase-9 regression was caught rather than normalized: the first manual-String helper landed inside the historical M4b String Rhythm source and moved the M4f bypass image from 95,804 to 95,820 B. The baseline was not relaxed. The old M4b blobs were restored exactly, manual String was isolated into a P9-only module and the generator now hash-guards those historical sources. Staged Gate 5b is again exact at 95,804 B.

Physical FM-1 ergonomics, real ISR headroom under active manual performance, physical LEDs/display, DAC listening and real power-cut persistence remain unverified.

Next after guarded PR #19 merge: Phase 10 String Motion and advanced interactive parity.

## 2026-10-31 - Phase 8 native visuals merged

PR #18 merged to `main` as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` after final documentation/CI head `856c4e35ae1f041da8ccdc555f4c82b54f43a04a` passed core #460, staged #449, P6-final #54, P7-storage #41, P7-target #34, P8-visuals #44 and P8-target #19.

Completed:
- source-derived allocation-free 240×240 visual raster and canonical RGB565 palette
- ten deterministic native surfaces: MOTHER, GRAVITY, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, KEEP, CHAIN and SPACE/CATHEDRAL
- KEEP/CHAIN visuals read the existing persistent/runtime structures directly; no UI shadow musical state
- one shared Space/Cathedral view with five source sends
- frozen per-surface visual hashes plus full-frame/strip-equivalence tests
- normal and ASAN/UBSAN visual gates
- target renderer reuses Dinghy canvas with no second framebuffer and no POOL growth
- 8-row LCD updates gated by HAL-level DMA completion and kept outside `app_audio()`
- all ten surfaces target-addressable; final physical SPACE mapping intentionally deferred to Phase 9
- real JieLi target build within image/RAM/POOL budgets
- pinned emulator preserves exact P6/P7 audible PCM and integrated runtime telemetry

Frozen technical candidate:
- source `87575aa829f3fab6af43092b440d585745eecc25`
- image 118,040 / 581,564 B
- RAM 88,644 / 98,304 B
- POOL 309,532 / 344,064 B
- package `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

A first native redraw changed only early audio despite exact runtime telemetry. Frame-level comparison showed differences limited to frames 1795..4351. The baseline was not relaxed: native takeover is deferred until runtime sample 8192, restoring the complete frozen PCM hash.

## 2026-10-30 - Phase 7 persistent KEEP/CHAIN storage merged

PR #17 merged to `main` as `e9000b2b2f74d4fafe96df87f030cc55916a8285` after final documentation/CI head `2fd054d90a49ff8e52544c092937dadc8e4097d9` passed core #436, staged #425, P6-final #32, P7-storage #19 and P7-target #12.

Completed versioned little-endian CRC project records, two-sector alternating journal, fake-NOR recovery/corruption tests, pre-audio boot prefetch, deferred stopped-transport main-loop saves and exact P6/Phase-5 audio regressions.

Frozen technical candidate: `e290813f5e67affc7404dcb9fc7af63d2c671a8c`, image 110,848 B, RAM 88,628 B, POOL 309,532 B.

## 2026-10-30 - Phase 6 shared Space/Cathedral merged

PR #16 merged to `main` as `cd321e9f7999d0568d280d89e7e3574c3da66054`. Completed one shared room, compact modulated 8-line FDN, dedicated pre-`.pool` NOLOAD storage, quarter-rate 11.025-kHz target Cathedral, exact Phase-5 dry regression and audible-room emulator gate.

Frozen Phase-6 target: image 107,352 B; RAM 87,780 B; POOL 309,532 B.

## 2026-10-30 - Phase 5 sounding integration merged

Phase 5 verified existing M3c + Phase-4 DSP structural behaviour end-to-end and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5` without changing production scheduler/DSP/runtime.

## 2026-10-30 - M4g Bloom merged

M4g completed source-correct occurrence planning, M3c occurrence/petal-bind reuse, deterministic finite Bloom DSP, 136-petal evidence-driven pool and exact M4f bypass. Frozen M4g target: image 100,944 B, RAM 87,336 B, pool 30,720 B, package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`.

## 2026-10-30 - M4f Spiral merged

PR #12 merged as `734229f4d23626a176a8366de82aab2c58c48cc9`.

## 2026-10-30 - M4e Clock merged

PR #11 merged as `db9d0b3ecbc7931be343cec7717103059d3c5160`.

## 2026-10-30 - M4d Drift merged

PR #10 merged as `d7e39e2481c33628f402bed51d761b8be8304bb3`.

## 2026-10-29 - M4c String Attractor merged

PR #9 merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.

## 2026-10-29 - M4b finite String + Rhythm merged

PR #8 merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

## 2026-10-29 - M4a persistent String merged

PR #7 merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

## 2026-10-29 - Phase 3 completed

M3a `fe2911c43fd69e63839220482e50bb10c7a2e4c2`, M3b `a61a40dc83c9b52129f7d3f920da26a9a61837d9`, M3c `ec0dc0b350793572f19040efc938f00121128dc6`.

## 2026-10-23 - Phase 2 completed

M2c `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361` completed pure state semantics.

## 2026-10-23 - Phase 1 completed

Established pinned Dinghy baseline, APSIS identity, host build, real JieLi target build, staged CI and FM-1 emulator smoke.

## 2026-10-23 - Project reference freeze

Locked Study 126 behavioural authority, port contract, architecture direction, parity matrix and source behaviour maps.
