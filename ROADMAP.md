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

- [x] M4a String persistent — merged `2ea217f09c143d30385f136e84e2b29d582987a0`
- [x] M4b String finite/Rhythm — merged `c96c00debf9a67948f1e874e36b090999dacadb9`
- [x] M4c String Attractor — merged `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
- [x] M4d Drift — merged `d7e39e2481c33628f402bed51d761b8be8304bb3`
- [x] M4e Clock — merged `db9d0b3ecbc7931be343cec7717103059d3c5160`
- [x] M4f Spiral — merged `734229f4d23626a176a8366de82aab2c58c48cc9`
- [x] M4g Bloom — merged `713a1e9ac746a1a4d19ba825a0e6765b337ba410` via PR #13

Frozen M4g target:
- image 100,944 B
- RAM 87,336 / 98,304 B
- static RAM headroom 10,968 B
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- pinned default PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

## Phase 5 - live structural behaviour / sounding-source integration

Status: **technical verification complete; final documentation-head gate + merge pending on PR #15.**

Base: merged M4g `713a1e9ac746a1a4d19ba825a0e6765b337ba410`  
Verification head before docs freeze: `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`  
Core #357 PASS; staged #346 PASS.

- [x] P5a immediate structural late binding reaches actual sounding Spiral/Bloom DSP
- [x] P5b quantized Gravity pending asymmetry reaches actual sounding Spiral/Bloom DSP
- [x] P5c Mother-held Gravity remains inaudible until `->1` commit, then reaches sounding sources
- [x] P5d Chain discrete boundary reaches sounding Spiral/Bloom in source order
- [x] P5e Chain continuous Drift morph begins on target and reaches sounding Drift DSP
- [x] P5f step-one RESYNC OFF free-runs into sounding Spiral energy; existing M3c slot owns trigger identity
- [x] all frozen M3/M4 host references remain unchanged
- [x] real JieLi target + emulator remain exact M4g because Phase 5 required no production code change
- [~] final continuity-document freeze + exact-head core/staged gate + guarded squash merge

Phase-5 architecture result:
- no production scheduler change
- no new event queue
- no new random draw
- no `ApsisRuntime` growth
- no new DSP pool or persistent BSS

## Phase 6 - shared Space / Cathedral

Start only after Phase 5 is merged from an exact double-gated documentation head.

- [ ] freeze Study-126 shared Space/Cathedral topology and source-send semantics
- [ ] prototype compact modulated FDN under the real 10,968 B static-RAM headroom constraint
- [ ] prototype nested allpass/Schroeder candidate under the same constraint
- [ ] compare diffusion / metallicity / decay / CPU / memory / target linkability
- [ ] choose one shared room with five per-source sends
- [ ] host reference + sanitizer gate
- [ ] real target memory gate before committing to large delay/storage structures
- [ ] emulator gate and dry-bypass regression

Do **not** create unrelated per-source reverbs. Do **not** spend the remaining static RAM casually; delay memory may need a different storage strategy than unconditional BSS.

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

1. Complete the Phase-5 continuity/documentation freeze on `p5-sounding-integration`.
2. Run core + full staged validation on that exact final documentation head.
3. If both pass, update PR #15 metadata only, mark ready and squash-merge with the gated head SHA.
4. Create the Phase-6 branch from the actual Phase-5 merge commit.
5. Freeze the Space/Cathedral source/topology contract and run memory prototypes before integrating a room into `app_audio()`.
