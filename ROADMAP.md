# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed
- [~] active
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
- [x] merged `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7

### M4b - String finite transients + shared Rhythm audio
- [x] merged `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8

### M4c - String Attractor excitation
- [x] merged `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9

### M4d - persistent Drift dry engine
- [x] merged `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10
- frozen host 488 / `0aec6bc2`

### M4e - Clock dry engine
- [x] merged `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11
- frozen STONE 2081 / `edcc4a81`
- frozen GLASS 1880 / `0adf9829`
- frozen SNAP 738 / `04bc514a`
- pool stress 23/32, 176 triggers, 0 drops
- target image/RAM 90,684 / 49,128 B
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

### M4f - Spiral dry engine

- [x] merged `734229f4d23626a176a8366de82aab2c58c48cc9` via PR #12
- [x] final docs head `744559f89df733c4c167367efe30250b6dcdecdf` passed core #261 + staged #250
- [x] THREAD / GLASS / REED / HALO topology
- [x] occurrence-time acoustic snapshot + existing M3c target−80 ms pitch bind
- [x] external 64-slot bridge; no `ApsisRuntime` growth
- [x] fixed 32-leg renderer
- [x] worst acoustic stress 30/32 legs, 90 oscillators, 0 steals/drops
- [x] real JieLi target + emulator
- [x] exact M4e bypass
- [x] inactive default path preserves cursor 123,648

Frozen host:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- stress `02be4bc5`

Merged M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`

### M4g - Bloom dry engine

Status: **implementation complete; final documentation-head gate pending.**

Branch `m4g-bloom`; draft PR #13; base is actual M4f merge `734229f4d23626a176a8366de82aab2c58c48cc9`.
Normative reference: `docs/reference/M4G_BLOOM_CONTRACT.md`.
Technically final implementation/CI head: `6c54d1cd5ca354543237a14c257b3ec28ab424d1`.

Completed:
- [x] preserve existing M3c target / occurrence target−45 ms / petal-bind timeline
- [x] occurrence freezes plan-derived petal count, length/duration, unfurl/targets, width/pan and source-level/amplitude
- [x] petal bind re-reads target-aware Gravity/tuning + current voice/attack/brightness
- [x] VEIL / CHOIR / ORGAN / GLASS oscillator/filter/envelope topology
- [x] preserve future-aware ordinary pending Gravity and frozen-plan petals across later DEPTH edits
- [x] pure occurrence-time `apsis_bloom_audio` planner + source-vector tests
- [x] external 64-slot M3c Bloom snapshot/bind bridge; no runtime growth
- [x] fixed-point finite Bloom renderer
- [x] legal worst-case capacity stress and evidence-based pool freeze at **136 petals**
- [x] target integration with inactive/default callback-budget protection
- [x] Bloom-DSP bypass returning exactly to frozen M4f target/PCM/runtime
- [~] exact final documentation-head double gate + guarded squash merge

Frozen host normal + ASAN/UBSAN:
- VEIL 175 / `6177470f`
- CHOIR 143 / `4b2969f8`
- ORGAN 122 / `d54a68d9`
- GLASS 129 / `ca1c3127`
- worst stress: 186 triggers, **125/136 petals**, 375 oscillators, 0 drops, peak 266, hash `5881bb36`

Technically final M4g target:
- image **100,944 B**
- RAM **87,336 B**
- RAM headroom **10,968 B**
- pool **30,720 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`

Pinned default emulator remains exact M4f because no Bloom occurrence becomes due during the three-second reference scenario:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- cursor 123,648
- late trace `811c9dc5`

Bloom bypass is exact frozen M4f target/package/PCM/runtime:
- image/RAM/pool 95,804 / 58,056 / 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- same PCM/cursor/queue/trace as M4f

Implementation-head gates:
- core #292 PASS
- staged #281 PASS all gates including real JieLi target, emulator and exact M4f bypass

No shared Space during dry-engine work.

## Phase 5 - live structural behaviour / sounding-source integration

- [ ] sample-exact state application to sounding sources
- [ ] pending Gravity with sounding sources
- [ ] Mother held edits with audio engine
- [ ] Chain changes with audio/event engine
- [ ] prove M3c last-moment Spiral/Bloom binding drives sounding voices unchanged

Phase 5 begins only after M4g is merged from an exact double-gated final documentation head.

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

M4g leaves only 10,968 B of static RAM headroom on the current target. Treat RAM as a first-class constraint in Phase 5/6; do not spend it casually before real-device timing/stack evidence exists.

## Phase 12 - hardware validation/release

- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

1. Finish the M4g continuity/documentation freeze on top of `6c54d1cd5ca354543237a14c257b3ec28ab424d1`.
2. Run core + full staged validation on the exact final documentation head.
3. If both pass, update PR #13 metadata only, mark ready and squash-merge with the gated head SHA.
4. Create the Phase-5 branch from the actual M4g squash merge and begin sounding-source structural integration; do **not** skip directly to Space/Cathedral.
