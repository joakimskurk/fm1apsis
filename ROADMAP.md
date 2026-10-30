# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed
- [~] active / implementation complete, merge pending
- [ ] not started

## Phase 0 - reference freeze

- [x] lock Study 126 behavioural source
- [x] behavioural map / parity matrix / port contract
- [x] verify attached current `index.html` is the same source modulo LF/CRLF
- [x] lock Study 98 as String acoustic reference only

## Phase 1 - pinned FM-1 skeleton

- [x] pinned Dinghy baseline and APSIS identity `FM-1_972`
- [x] host / real target / emulator validation pipeline

## Phase 2 - pure musical/state core

- [x] defaults/state, tuning, Gravity, String, Drift, Clock, Spiral, Bloom, KEEP, CHAIN, MOTHER

Merged through `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

## Phase 3 - deterministic transport/runtime

- [x] M3a shared sample-referenced event grid — merged `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- [x] M3b structural/runtime consumers — merged `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- [x] M3c late binding/long deterministic trace — merged `ec0dc0b350793572f19040efc938f00121128dc6`

Frozen M3c runtime: 21,928 B; 512-event shared heap; no module-private clocks.

## Phase 4 - five dry source engines

### M4a - String persistent dry core

Merged `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7.

- [x] Study-98 Foundation + Classic sustain
- [x] Study-126 Dual BODY/WIRE preservation
- [x] fixed-point 44.1 kHz stereo Q15 renderer
- [x] deterministic host references + ASAN/UBSAN
- [x] real JieLi target + emulator

Frozen host: Classic 1477 / `b20610c1`; Dual 823/787 / `10d23cf4`.

### M4b - String finite transients + shared Rhythm audio

Merged `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8.

- [x] fixed 80 logical +32 fade-tail finite pool
- [x] Study-98 normal/pizz partial primitive
- [x] Study-126 BODY/composite topology
- [x] shared `STRING_RHYTHM`; no private scheduler
- [x] callback-boundary proof: never early, <1 block late
- [x] target/emulator regression

Frozen finite refs: transient 1309 / `97142d3e`; BODY 947 / `c1168570`.

### M4c - String Attractor excitation

Merged `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9.

- [x] seeded S&H planner and source random ordering
- [x] shared target−45 ms bind; no private queue
- [x] 64 pending slots; stress 60/64 and heap 415/512
- [x] late-read pitch/levels/brightness/Foundation
- [x] exact target-sample finite voice delay
- [x] real JieLi target + emulator

### M4d - persistent Drift dry engine

Merged `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10.

- [x] explicit source-defect disposition for missing persistent browser wiring
- [x] paired sine field around tuned Gravity root
- [x] source delta/depth/level/width laws
- [x] 25 ms smoothing
- [x] target/emulator + exact M4c bypass

Frozen host Drift: 488 / `0aec6bc2`.

### M4e - Clock dry engine

Merged `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11.
Final documentation head `ba3c05e4…`; core #231 + staged #220 PASS.

- [x] STONE / WOOD / GLASS / SOFT and `pitch_centi=50..200`
- [x] passive shared MAIN / SNAP / INNER / GHOST consumers
- [x] deterministic Clock-local Snap noise; shared RNG untouched
- [x] 32-slot fixed finite pool; stress 23/32, 176 triggers, 0 drops
- [x] target-only `__divdi3`/`__udivdi3` compatibility shim
- [x] real target + emulator + exact M4d PCM/runtime bypass

Frozen host Clock:
- STONE 2081 / `edcc4a81`
- GLASS 1880 / `0adf9829`
- SNAP 738 / `04bc514a`

Frozen M4e target/emulator baseline:
- image 90,684 B
- RAM 49,128 B
- pool 30,720 B
- package `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- runtime cursor 123,648; queue 28/512; late trace `811c9dc5`

### M4f - Spiral dry engine

Status: **implementation complete; PR #12 pending only final documentation-head double gate**.

Active branch `m4f-spiral`, based directly on M4e merge `db9d0b3ecbc7931be343cec7717103059d3c5160`.
Technically final implementation/CI head: `11569371ea18bc8a5e06f28d09ddef90f9835e29` — core #260 PASS, staged #249 PASS.

Normative reference: `docs/reference/M4F_SPIRAL_CONTRACT.md`.

- [x] source-resolve THREAD / GLASS / REED / HALO acoustic topology
- [x] freeze energy/duration/noteLength/source-level/Drift delta+width at planning time
- [x] consume existing M3c Spiral token/slot/target−80 ms bind; no new event or random draw
- [x] preserve late Gravity/register/tuning rebind and current-voice filter-cutoff edge semantics
- [x] external 64-slot snapshot/bind runtime; `ApsisRuntime` remains 21,928 B
- [x] fixed 32-leg finite renderer; +8 ms shadow remains DSP-local
- [x] deterministic host refs normal + ASAN/UBSAN
- [x] worst acoustic stress: 15 logical, 30/32 legs, 90 oscillators, 0 steals, 0 drops
- [x] real JieLi target + default emulator
- [x] Spiral-DSP bypass returns exactly to frozen M4e target/PCM/runtime
- [x] idle callback path preserves exact frozen cursor 123,648 when source-default route is empty
- [x] emulator smoke now asserts exact 123,648 cursor, not merely nonzero progress
- [ ] exact final documentation-head double gate and PR #12 squash-merge

Frozen host refs:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- stress hash `02be4bc5`

Technically final M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- default PCM remains frozen M4e because default Spiral route is empty
- queue 28/512; cursor 123,648; late trace `811c9dc5`

### Remaining dry engine

- [ ] M4g Bloom

No shared Space during initial dry-engine work.

## Phase 5 - live structural behaviour / sounding-source integration

- [ ] sample-exact state application to sounding sources
- [ ] pending Gravity with sounding sources
- [ ] Mother held edits with audio engine
- [ ] Chain changes with audio/event engine
- [ ] prove M3c last-moment Spiral/Bloom binding drives sounding voices unchanged

## Phase 6 - shared Space / Cathedral

- [ ] prototype compact modulated FDN
- [ ] prototype nested allpass/Schroeder candidate
- [ ] compare diffusion/metallicity/decay/CPU/memory
- [ ] one shared room with five per-source sends

## Phase 7 - persistent KEEP / CHAIN storage

- [ ] 8-slot project persistence
- [ ] 32-cell chain persistence
- [ ] versioned CRC records
- [ ] two-slot/journal recovery
- [ ] power-loss/corruption tests

## Phase 8 - native visual surfaces

- [ ] MOTHER / GRAVITY / STRING / CLOCK / SPIRAL / BLOOM / DRIFT-TUNING / CHAIN / KEEP / SPACE

## Phase 9 - physical interaction parity

- [ ] direct FM-1 mappings for Clock, Gravity, String, Spiral, Bloom, KEEP, CHAIN, MOTHER

## Phase 10 - String Motion and advanced interactive parity

- [ ] Clock-quantized capture
- [ ] Classic/Dual ownership and frozen pitch context
- [ ] overdub/manual override
- [ ] String/Spiral attractor and Gravity point editors

## Phase 11 - FM-1 optimisation

- [ ] real ISR timing under DSP load
- [ ] bit-identical reduction of proven-safe 64-bit intermediates
- [ ] RAM/pool/stack headroom
- [ ] queue and voice-pool stress
- [ ] UI redraw load
- [ ] Cathedral tuning

## Phase 12 - hardware validation/release

- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

1. Commit/freeze the M4f continuity documentation on top of technically final `11569371…`.
2. Gate that exact documentation head with `APSIS core` and full `APSIS staged validation`.
3. If both are green, update PR #12 metadata without moving the head, mark ready and squash-merge with `expected_head_sha` equal to the gated docs head.
4. Create M4g/Bloom from the actual M4f merge commit and source-resolve Bloom acoustics before DSP code.
