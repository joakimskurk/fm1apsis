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
- [x] compact 8-line modulated FDN
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

Status: **[x] complete and merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` via PR #18.**

- [x] source-derived 240×240 layout/visual contract
- [x] allocation-free bounded integer renderer
- [x] ten native APSIS surfaces including one shared SPACE/CATHEDRAL
- [x] deterministic visual hashes + full-frame/strip equivalence + ASAN/UBSAN
- [x] target integration reusing Dinghy canvas with no POOL growth
- [x] HAL-gated 8-row incremental LCD DMA updates outside `app_audio()`
- [x] exact audio preserved by native takeover after runtime sample 8192
- [x] real JieLi target + pinned emulator P8 gate
- [x] guarded PR #18 merge

Frozen technical P8 head `87575aa829f3fab6af43092b440d585745eecc25`: image 118,040 B; RAM 88,644 B; POOL 309,532 B; audible PCM exact P6/P7; cursor 123,392.

## Phase 9 — physical interaction parity

Status: **[x] complete and merged as `bbfab322557b6760425503268b550b789f796c23` via PR #19.**

- [x] freeze pinned FM-1 panel/control/gesture contract
- [x] HOME/STRING/DRIFT/CLOCK/SPIRAL/BLOOM/GRAVITY/CHAIN/KEEP/SPACE navigation
- [x] direct CLOCK and GRAVITY editing over existing shared state/runtime
- [x] source-locked 3×9 STRING keyboard mapping and bounded parameters
- [x] ordered raw-point SPIRAL route editing
- [x] BLOOM petal/core/grow performance + parameter contract
- [x] KEEP capture/recall/erase and CHAIN edit/transport contract
- [x] MOTHER live mix / `->1` structural sync and shared SPACE editing
- [x] DRIFT/TUNING control contract
- [x] relative encoder semantics with no hidden absolute pickup state; MASTER remains platform volume
- [x] deterministic routing/apply/dirty/performance tests + ASAN/UBSAN
- [x] generated target binding with activity-gated input consumption and existing LED/state feedback
- [x] project-dirty coalescing through existing P7 deferred persistence
- [x] direct STRING performance via P9-only audio adapter and finite main→audio mailbox
- [x] direct BLOOM performance via P9-only finite manual queue, shared M3c RNG and existing M4g binding/DSP
- [x] historical M4f 95,804-B bypass preserved; no baseline relaxation
- [x] real JieLi target + exact P8 idle PCM/runtime emulator regression
- [x] final documentation-head 10-workflow matrix PASS on `dc288b6424b957ad382997f62cd6924b4146f675`
- [x] guarded PR #19 squash merge

Frozen technical P9 head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- image 132,264 / 581,564 B
- RAM 89,492 / 98,304 B
- POOL 309,532 / 344,064 B
- package `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`
- audible PCM exact P6/P7/P8 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

## Phase 10 — String Motion and advanced interactive parity

Status: **[~] current. String Motion checkpoint P10D3 is green; P10E String Attractor editor is next.**

- [x] Clock-quantized String Motion capture
- [x] Classic/Dual ownership and frozen pitch context
- [x] overdub/manual override
- [x] finite main→audio mailbox + dedicated 19,208-B Motion POOL
- [x] deterministic Motion host/sanitizer contracts
- [x] real JieLi Motion target + exact frozen Phase-9 idle PCM/runtime regression
- [x] active pinned-emulator lifecycle: arm → next Clock 1 → record → loop → manual override → resume
- [ ] String Attractor editor
- [ ] Spiral Attractor editor
- [ ] selected Gravity-point editor
- [ ] final advanced-interaction target/emulator regression preserving frozen Phase-9 idle audio/runtime

P10D3 evidence on `e750655141bb87e83949bb262426ab2ee42d734b`:
- production P10D2 target: image 136,984 B; RAM 89,668 B; POOL 328,740 B
- production RAM headroom 8,636 B; POOL headroom 15,324 B
- active D3 telemetry target: image 137,240 B; RAM 89,732 B; POOL 328,740 B
- active lifecycle flags `ff`; final flags `14`
- Dual+LINK capture 149 points left / 149 right
- record start sample 220,672; finish 441,088; cycle 220,416 samples
- manual override sample 450,560 → resume 466,176
- runtime cursor 481,024; queue high-water 30/512; overflow 0; late flags 0
- active WAV peak 1,171; RMS 173

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

Begin **Phase 10E — String Attractor editor** as a host-first control/state layer above the already-frozen M4c String Attractor runtime. The first N physical keys must edit the existing `filter_attractor_gates` cells (maximum 16), reroll must change the existing bounded seed and let `apsis_string_attractor_sync()` regenerate deterministically, and no private scheduler/DSP state may be introduced. Preserve the Phase-9 String performance mapping outside the explicit Attractor edit context, then bind the editor to the FM-1 target only after host + sanitizer contracts are green.