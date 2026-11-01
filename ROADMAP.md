# ROADMAP

Short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`; frozen implementation detail remains in `PROJECT_STATE.md` and phase-specific documents.

Status legend: `[x]` complete, `[~]` active, `[ ]` not started.

## Phase 0 — reference freeze
- [x] Study 126 behavioural authority locked
- [x] Study 98 narrow String acoustic authority locked
- [x] behaviour map / parity matrix / port contract

## Phase 1 — pinned FM-1 skeleton
- [x] pinned Dinghy baseline and `FM-1_972` identity
- [x] host / real-target / pinned-emulator validation pipeline

## Phase 2 — pure musical/state core
- [x] defaults/state, tuning, Gravity, String, Drift, Clock, Spiral, Bloom, KEEP, CHAIN, MOTHER

## Phase 3 — deterministic transport/runtime
- [x] shared M3c sample-referenced scheduler/runtime
- [x] structural/runtime consumers and late binding
- [x] long deterministic traces and 512-event shared heap

Frozen host runtime remains 21,928 B; no module-private clocks.

## Phase 4 — five dry source engines
- [x] String persistent / finite Rhythm / Attractor
- [x] Drift
- [x] Clock
- [x] Spiral
- [x] Bloom

## Phase 5 — live structural/sounding integration
- [x] structural late binding
- [x] quantized Gravity sounding asymmetry
- [x] MOTHER `->1` sounding commit semantics
- [x] CHAIN discrete/continuous sounding semantics
- [x] step-one free-run/resync
- [x] merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`

## Phase 6 — shared Space / Cathedral
- [x] one shared room + five source sends
- [x] compact 8-line fixed-point FDN
- [x] quarter-rate 11.025-kHz target room while source/output remain 44.1 kHz
- [x] dedicated pre-`.pool` NOLOAD room storage
- [x] exact dry/audio regression and guarded merge
- [x] merged as `cd321e9f7999d0568d280d89e7e3574c3da66054`

## Phase 7 — persistent KEEP / CHAIN storage
- [x] 8 KEEP slots + 32 CHAIN cells/settings
- [x] versioned little-endian CRC journal over two NOR sectors
- [x] corruption/torn-write/wraparound tests
- [x] pre-audio boot read + deferred audio-safe writes
- [x] merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285`

## Phase 8 — native 240×240 visuals
- [x] ten deterministic APSIS surfaces
- [x] allocation-free renderer using existing Dinghy canvas
- [x] HAL-gated incremental LCD work outside `app_audio()`
- [x] exact audio regression
- [x] merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457`

## Phase 9 — physical interaction parity
- [x] pinned FM-1 panel/control/gesture contract
- [x] all ten surfaces navigable from existing panel vocabulary
- [x] direct String/Bloom performance via finite mailboxes/adapters
- [x] existing shared runtime/state/persistence authorities reused
- [x] exact target/emulator regression and final matrix
- [x] merged as `bbfab322557b6760425503268b550b789f796c23` via PR #19

Frozen Phase-9 target: image 132,264 B; RAM 89,492 B; POOL 309,532 B.

## Phase 10 — String Motion and advanced interactive parity

Status: **[x] complete and squash-merged to `main` as `378d2023df2afe59475be53c09cc001788586514` via PR #20.**

- [x] Clock-quantized 1/2/4/8-cycle String Motion capture
- [x] Classic/Dual ownership, free-Dual overdub and frozen pitch context
- [x] manual override/resume on shared Clock timeline
- [x] String Attractor editor over existing M4c authority
- [x] Spiral Attractor editor over existing M4f authority
- [x] selected Gravity-point editor over existing 24-point grid
- [x] no private scheduler, musical RNG or duplicate musical state
- [x] final exact-head matrix 22/22 PASS on `ce5857a52c6326011e5af7cd7f73819b420550fc`

Frozen Phase-10 production baseline:
- image **140,152 / 581,564 B**
- RAM **89,684 / 98,304 B**
- POOL **328,740 / 344,064 B**
- package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0

Phase 10 is JieLi-target/pinned-emulator verified, **not physical-FM-1 verified**.

## Phase 11 — FM-1 optimisation

Status: **[~] active on `p11-optimization`. Measurement first; no redesign.**

### P11A — baseline inventory
- [x] exact final Phase-10 image/RAM/POOL baseline
- [x] host `sizeof(ApsisRuntime) = 21,928 B`
- [x] original MotionStore 19,208 B accounts for the complete P9→P10 POOL increase
- [x] explicit measured-vs-unmeasured boundary
- [x] green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8`

### P11B — Motion storage falsification proof
- [x] one frozen pitch context per lane reproduces every captured point pitch exactly
- [x] free-Dual independent lane contexts/overdub proved
- [x] int16 pitch compression rejected as lossy
- [x] projected MotionStore 19,208 → 9,644 B with 1,200 points/lane and 32-Hz bound unchanged
- [x] ASAN/UBSAN green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c`

### P11C — compact lossless Motion store
- [x] point storage 8 B → 4 B; one 20-B frozen context/lane
- [x] MotionStore **19,208 → 9,644 B**
- [x] endpoint pitch cache avoids reconstructing quantized/tuned pitch every callback
- [x] host reference equivalence + ASAN/UBSAN
- [x] exact real JieLi production target
- [x] exact frozen inactive PCM/runtime
- [x] exact active Motion lifecycle
- [x] exact-head green run `41318100472` on `7ce6993065186aa66c89bc723ba3edfa95448596`
- [x] complete production + idle + active evidence artifact

Frozen P11C production target:
- image **140,408 / 581,564 B**
- RAM **89,716 / 98,304 B**; headroom **8,588 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package SHA-256 `e10c01525a808a4dea18d6b3f8777307bf4acf180c2cf958bebc4e1546b0d0be`
- POOL improvement vs Phase 10: **−9,564 B**

### P11D — real-target memory ownership inventory
- [x] exact P11C target regenerated with pinned JieLi stack
- [x] real `objdump -h/-t` and generated linker script captured
- [x] MotionStore independently confirmed as 9,644 B in `.apsis_motion_pool`
- [x] ordinary RAM span reconciled exactly: 12-B `.data` + 89,700-B `.bss` + 4-B alignment = 89,716 B
- [x] unique sized-symbol intervals cover 89,658 B; remaining 58 B is not treated as free RAM
- [x] largest ordinary RAM owners ranked
- [x] configured user/system stack ranges identified and explicitly separated from runtime high-water
- [x] green run `41320007779` on `2e1912f5d0ff8987382bd7af434fc322679bf762`

Largest ordinary target-RAM owners:
- `bloom_dsp` 22,864 B
- `apsis_runtime` 21,128 B on target ABI
- `string_transient_pool` 7,188 B
- `bloom_audio_runtime` 6,424 B
- `spiral_audio_runtime` 4,792 B

The top five account for 62,396 B / about 69.55% of the ordinary RAM span.

Static configured stack ranges:
- user stack 24,320 B + 256-B guard
- system stack 7,936 B + 256-B guard

These are **configured ranges only**, outside the 96-KiB ordinary RAM metric and after the POOL region. No runtime stack-high-water claim is made.

### P11E — capacity/high-water stress inventory

Status: **[ ] next. Measurement only; no capacity reduction yet.**

- [ ] deterministic worst-case shared M3c queue/event-heap telemetry
- [ ] Bloom DSP / occurrence / voice capacity high-water and steals/drops where available
- [ ] String transient logical/physical pool high-water, steals and drops
- [ ] Bloom manual/audio queue pressure
- [ ] Spiral audio/runtime slot pressure
- [ ] String Attractor pending/runtime pressure
- [ ] combined stress scenarios that intentionally approach existing bounds
- [ ] distinguish normal musical scenarios from artificial saturation probes
- [ ] select a P11F reduction candidate only if measured safety margin is convincing

P11E must use existing counters where possible and test-only telemetry where necessary. It must not create a new scheduler or alter production musical behavior.

### Later Phase-11 work
- [ ] P11F evidence-backed RAM/capacity optimisation if P11E identifies one
- [ ] target/emulator timing proxies, explicitly separated from physical ISR claims
- [ ] UI redraw/DMA workload investigation without changing visual authority
- [ ] combined control/UI/audio stress gates
- [ ] Cathedral listening/tuning on physical hardware when available

Phase-11 invariant: optimise only from measured evidence; preserve Phase-10 behavior and all frozen regression gates unless a change is explicitly re-approved.

## Phase 12 — hardware validation/release
- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] runtime stack high-water / real ISR margin and hardware timing
- [ ] DAC/listening quality
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

Promote the completed P11D read-only instrumentation/evidence to `p11-optimization`, then start **P11E capacity/high-water stress inventory** from that head. Instrument existing bounded pools/counters without resizing them; establish reproducible worst observed occupancy/steal/drop margins before any P11F capacity reduction is considered.
