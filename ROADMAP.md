# ROADMAP

Short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

Status legend: `[x]` complete, `[~]` active, `[ ]` not started.

## Phase 0 — reference freeze
- [x] Study 126 behavioural authority locked
- [x] Study 98 narrow String acoustic authority locked
- [x] behaviour map / parity matrix / port contract

## Phase 1 — pinned FM-1 skeleton
- [x] pinned Dinghy baseline and `FM-1_972` identity
- [x] host / real target / emulator validation pipeline

## Phase 2 — pure musical/state core
- [x] defaults/state, tuning, Gravity, String, Drift, Clock, Spiral, Bloom, KEEP, CHAIN, MOTHER

## Phase 3 — deterministic transport/runtime
- [x] M3a shared sample-referenced event grid
- [x] M3b structural/runtime consumers
- [x] M3c late binding/long deterministic trace

Frozen runtime: 21,928 B, 512-event shared heap, no module-private clocks.

## Phase 4 — five dry source engines
- [x] M4a String persistent
- [x] M4b String finite/Rhythm
- [x] M4c String Attractor
- [x] M4d Drift
- [x] M4e Clock
- [x] M4f Spiral
- [x] M4g Bloom

## Phase 5 — live structural behaviour / sounding integration
- [x] immediate structural late binding reaches sounding Spiral/Bloom
- [x] quantized Gravity asymmetry reaches sounding DSP
- [x] Mother `->1` commit semantics reach sounding sources
- [x] Chain discrete and continuous semantics reach sounding DSP
- [x] step-one free-run/resync reaches sounding Spiral energy
- [x] merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`

## Phase 6 — shared Space / Cathedral

Status: **[x] complete and merged as `cd321e9f7999d0568d280d89e7e3574c3da66054`.**

- [x] freeze shared-room topology / source sends / parameter roles
- [x] select compact 8-line modulated FDN after topology comparison
- [x] dedicated pre-`.pool` NOLOAD room storage
- [x] fixed-point host/reference room + target source/send integration
- [x] target quarter-rate 11.025 kHz Cathedral while sources/output remain 44.1 kHz
- [x] exact Phase-5 dry regression + audible-room emulator gate
- [x] guarded PR #16 merge

Frozen Phase-6 target: image 107,352 B; RAM 87,780 B; POOL 309,532 B; audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`; dry PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`; integrated cursor 123,392.

## Phase 7 — persistent KEEP / CHAIN storage

Status: **[x] complete and merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285` via PR #17.**

- [x] 8 KEEP slots + 32 CHAIN cells/settings persistence contract
- [x] explicit little-endian versioned CRC records
- [x] dedicated project sectors + alternating generation journal
- [x] corruption/torn-write/wraparound/fake-NOR recovery tests + sanitizers
- [x] pre-audio boot prefetch and deferred audio-safe saves
- [x] real JieLi target within budgets
- [x] audible PCM exact P6 and dry PCM exact Phase 5
- [x] final exact-head validation and guarded PR #17 merge

Frozen technical P7 candidate `e290813f5e67affc7404dcb9fc7af63d2c671a8c`: image 110,848 B; RAM 88,628 B; POOL 309,532 B; cursor 123,392.

## Phase 8 — native visual surfaces

Status: **[x] technical implementation complete on `87575aa829f3fab6af43092b440d585745eecc25`; PR #18 merge pending.**

- [x] freeze source-derived 240×240 layout/visual contract
- [x] inventory pinned FM-1 graphics/update path and redraw hazards
- [x] allocation-free bounded integer graphics primitives with clipping
- [x] native MOTHER / GRAVITY / STRING / CLOCK / SPIRAL / BLOOM / DRIFT-TUNING / CHAIN / KEEP / SPACE
- [x] String central veil + SPLIT anchors
- [x] Clock orbit/timeline treatment
- [x] Spiral route on spiral geometry
- [x] Bloom probability/cluster treatment
- [x] Gravity/Mother state-driven mappings
- [x] Chain/Keep non-generic treatment reading real persistent/runtime structures
- [x] one shared Space/Cathedral visual with five source sends
- [x] source-reference review against the frozen APSIS visual/state contract; no claim of screenshot/pixel-identical browser parity
- [x] deterministic per-surface hashes, full-frame/strip equivalence and ASAN/UBSAN
- [x] target integration reusing Dinghy canvas with no POOL growth
- [x] HAL-gated 8-row incremental LCD DMA updates outside `app_audio()`
- [x] preserve exact P7/P6 audible PCM by deferring native takeover until runtime sample 8192
- [x] expose all 10 surfaces to target adapter while leaving final SPACE physical mapping to Phase 9
- [x] real JieLi target + pinned emulator P8 gate
- [ ] guarded PR #18 merge

Frozen technical P8 head `87575aa829f3fab6af43092b440d585745eecc25`:
- image 118,040 / 581,564 B
- RAM 88,644 / 98,304 B
- POOL 309,532 / 344,064 B
- package `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`
- audible PCM exact P6/P7 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

## Phase 9 — physical interaction parity

Status: **[ ] next after Phase 8 merge.**

- [ ] freeze FM-1 control/gesture mapping contract
- [ ] direct mappings for Clock, Gravity, String, Spiral, Bloom, KEEP, CHAIN, MOTHER and SPACE
- [ ] encoder/pot pickup semantics that do not jump untouched parameters
- [ ] input routing and LED feedback from existing shared APSIS state
- [ ] deterministic routing tests before target integration
- [ ] target/emulator regression proving control polling does not alter frozen audio/runtime

## Phase 10 — String Motion and advanced interactive parity
- [ ] Clock-quantized capture
- [ ] Classic/Dual ownership and frozen pitch context
- [ ] overdub/manual override
- [ ] String/Spiral attractor and Gravity point editors

## Phase 11 — FM-1 optimisation
- [ ] real ISR timing under DSP load
- [ ] proven-safe arithmetic/memory optimization
- [ ] RAM/pool/stack headroom
- [ ] queue/voice stress
- [ ] UI redraw load
- [ ] Cathedral listening/tuning on hardware

## Phase 12 — hardware validation/release
- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

Finalize exact-head CI and guarded merge for Phase 8. Then begin Phase 9 by freezing the physical FM-1 interaction contract before changing any button/encoder/pot mappings.
