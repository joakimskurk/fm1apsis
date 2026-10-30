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

- [x] freeze Study-126 shared-room topology / source sends / parameter roles
- [x] compare compact modulated FDN vs nested allpass/Schroeder
- [x] select compact 8-line modulated FDN
- [x] dedicate pre-`.pool` NOLOAD room storage and prove memory accounting
- [x] implement fixed-point FDN and full shared-room host/reference engine
- [x] split target sources into post-Mother contributions and one shared send accumulator
- [x] integrate shared room into `app_audio()`
- [x] preserve exact Phase-5 PCM in room-return-disabled P6 variant
- [x] retime generated FM-1 room to 11.025 kHz internal rate while sources/output stay 44.1 kHz
- [x] real JieLi target build fits image/RAM/POOL
- [x] audible-room emulator gate
- [x] explicit P6 boot-to-stop runtime baseline with zero late/overflow flags
- [x] guarded PR #16 squash merge

Frozen Phase-6 target: image 107,352 B; RAM 87,780 B; POOL 309,532 B; audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`; dry PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`; integrated cursor 123,392.

## Phase 7 — persistent KEEP / CHAIN storage

Status: **[x] complete and merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285` via PR #17.**

- [x] freeze persisted data contract for 8 KEEP slots + 32 CHAIN cells/settings
- [x] explicit little-endian versioned CRC records
- [x] dedicated project sectors at `0x000F4000` / `0x000F5000`
- [x] alternating two-slot generation journal with header-last commit
- [x] canonical decode/range validation; transient CHAIN transport excluded
- [x] desktop torn-write/corruption/generation-wrap tests
- [x] fake FM-1 NOR save/reboot/failure/recovery tests
- [x] ASAN/UBSAN persistence gates
- [x] pre-audio boot prefetch + later RAM-only project apply
- [x] deferred main-loop saves with transport stopped and audio quiesced
- [x] static guard: no erase/program/service path reachable from `app_audio()`
- [x] real JieLi target build within image/RAM/POOL budgets
- [x] audible emulator remains bit-exact P6
- [x] dry-only emulator remains exact Phase 5
- [x] final exact-head core + staged + P6-final + P7-storage + P7-target all PASS
- [x] guarded PR #17 squash merge

Frozen technical P7 candidate `e290813f5e67affc7404dcb9fc7af63d2c671a8c`:
- image 110,848 / 581,564 B
- RAM 88,628 / 98,304 B
- POOL 309,532 / 344,064 B
- package `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`
- audible PCM exact P6 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- dry PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

## Phase 8 — native visual surfaces

Status: **[~] active.**

- [ ] freeze 240×240 layout/visual contract
- [ ] inventory FM-1 graphics/update path and redraw budget
- [ ] native bounded-cost graphics primitives
- [ ] MOTHER / GRAVITY / STRING / CLOCK / SPIRAL / BLOOM / DRIFT-TUNING / CHAIN / KEEP / SPACE
- [ ] String central veil + visible SPLIT anchors
- [ ] Clock orbit/timeline trace
- [ ] Spiral route on spiral geometry
- [ ] Bloom probability/cluster density
- [ ] Gravity/Mother active mapping overlays
- [ ] Chain/Keep readable non-generic treatment
- [ ] visual comparison against locked APSIS/browser reference
- [ ] deterministic visual regression evidence before Phase 9

## Phase 9 — physical interaction parity
- [ ] direct FM-1 mappings for Clock, Gravity, String, Spiral, Bloom, KEEP, CHAIN, MOTHER, SPACE

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

Begin Phase 8A from merged Phase 7: freeze the native 240×240 visual contract, inventory the pinned display path, and implement reusable bounded-cost graphics primitives. Do not introduce UI shadow state or disturb the frozen audio/runtime/persistence paths.