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
- [x] final documentation-head matrix PASS
- [x] guarded PR #19 squash merge

Frozen technical P9 head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- image 132,264 / 581,564 B
- RAM 89,492 / 98,304 B
- POOL 309,532 / 344,064 B
- package `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`
- audible PCM exact P6/P7/P8 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

## Phase 10 — String Motion and advanced interactive parity

Status: **[x] feature-complete on technical head `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`; final exact-head documentation/CI gate on PR #20 is the only remaining merge step.**

- [x] Clock-quantized String Motion capture
- [x] Classic/Dual ownership and frozen pitch context
- [x] overdub/manual override/resume on shared Clock timeline
- [x] finite main→audio mailbox + dedicated 19,208-B Motion POOL
- [x] deterministic Motion host/sanitizer contracts
- [x] real JieLi Motion target + exact frozen Phase-9 idle PCM/runtime regression
- [x] active pinned-emulator Motion lifecycle
- [x] String Attractor editor over existing M4c authority
- [x] String Attractor real target + frozen idle + active emulator lifecycle
- [x] Spiral Attractor editor over existing M4f state/runtime
- [x] Spiral Attractor real target + frozen idle + active emulator lifecycle
- [x] selected Gravity-point editor over existing 24-point grid
- [x] Gravity selected-point real target + frozen idle + active emulator lifecycle
- [x] all advanced editors preserve Phase-9 mappings outside explicit edit contexts
- [x] no new private scheduler, musical RNG or duplicate musical state
- [x] final stacked target remains inside image/RAM/POOL budgets
- [x] same frozen Phase-9 idle PCM/runtime baseline preserved through P10G
- [ ] squash merge PR #20 after exact-head documentation matrix is green

Phase-10 evidence summary:
- P10D3 Motion: active flags `ff`, Dual+LINK 149/149 points, cycle 220,416 samples, queue 30, overflow/late 0
- P10E String Attractor: image 137,304 B; RAM 89,668 B; POOL 328,740 B; active `seen=3f`, gate `9535→9534`, seed `317→8236`
- P10F Spiral Attractor: image 138,840 B; RAM 89,684 B; POOL 328,740 B; active `seen=3f`, seed `911→4825`, route isolated
- P10G Gravity selected-point: final production image **140,152 / 581,564 B**, RAM **89,684 / 98,304 B**, POOL **328,740 / 344,064 B**
- P10G production package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- P10G active `seen=7f`, degree 4 selected without audition, six numeric fields + enabled edited, other 23 points stable, later plain degree 6 request preserved
- final inactive PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- final inactive cursor 123,392; queue 28/512; overflow 0; late flags 0

Phase 10 is emulator/JieLi-target verified, **not physical-FM-1 verified**.

## Phase 11 — FM-1 optimisation

Status: **[ ] next after Phase-10 merge. Measurement first; no redesign.**

- [ ] establish target/emulator timing proxies and explicitly separate them from real hardware ISR claims
- [ ] final Phase-10 image/RAM/POOL/stack budget inventory
- [ ] proven-safe arithmetic/memory optimization
- [ ] Motion POOL/storage efficiency review without changing musical semantics
- [ ] queue/voice stress and worst-case shared-runtime pressure
- [ ] UI redraw/DMA workload reduction without changing visual authority
- [ ] combined control/UI/audio stress gates
- [ ] Cathedral listening/tuning on hardware when physical FM-1 becomes available

Phase-11 invariant: optimise only from measured evidence; preserve Phase-10 behavior and all frozen regression gates unless a change is explicitly re-approved.

## Phase 12 — hardware validation/release
- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] real ISR margin and hardware timing
- [ ] DAC/listening quality
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

Run the final exact-head CI matrix on the Phase-10 documentation head. If every required core/staged/P6/P7/P8/P9/P10 host, target and active workflow is green on that exact SHA, squash-merge PR #20 into `main`. Then branch Phase 11 from the merged `main` and begin with measurement/budget inventory before changing code.
