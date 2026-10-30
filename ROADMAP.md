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

## Phase 3 - deterministic transport/runtime

- [x] M3a shared sample-referenced event grid — merged `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- [x] M3b structural/runtime consumers — merged `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- [x] M3c late binding/long deterministic trace — merged `ec0dc0b350793572f19040efc938f00121128dc6`

Frozen runtime: 21,928 B; 512-event shared heap; no module-private clocks.

## Phase 4 - five dry source engines

- [x] M4a String persistent — `2ea217f09c143d30385f136e84e2b29d582987a0`
- [x] M4b String finite/Rhythm — `c96c00debf9a67948f1e874e36b090999dacadb9`
- [x] M4c String Attractor — `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
- [x] M4d Drift — `d7e39e2481c33628f402bed51d761b8be8304bb3`
- [x] M4e Clock — `db9d0b3ecbc7931be343cec7717103059d3c5160`
- [x] M4f Spiral — `734229f4d23626a176a8366de82aab2c58c48cc9`
- [x] M4g Bloom — `713a1e9ac746a1a4d19ba825a0e6765b337ba410`

Frozen M4g target: image 100,944 B; RAM 87,336 / 98,304 B; pool 30,720 / 344,064 B; package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`.

## Phase 5 - live structural behaviour / sounding-source integration

- [x] P5a immediate structural late binding reaches sounding Spiral/Bloom DSP
- [x] P5b quantized Gravity pending asymmetry reaches sounding Spiral/Bloom DSP
- [x] P5c Mother `->1` reaches sounding sources only after commit
- [x] P5d Chain discrete reaches sounding Spiral/Bloom in source order
- [x] P5e Chain continuous Drift morph reaches sounding DSP on target
- [x] P5f step-one free-run/resync reaches sounding Spiral energy
- [x] final head `17e7fd1247b0f5675261a7395b0558b5bbd6780d` passed core #363 + staged #352
- [x] squash-merged via PR #15 as `d677a887cf6dc8552634bde5a71fec0221c299b5`

Phase 5 changed no production scheduler/DSP/runtime and consumed zero additional target RAM.

## Phase 6 - shared Space / Cathedral

Status: **active on `p6-space-cathedral` from merged Phase 5. Pure host room engine complete; target/audible integration next.**

- [x] freeze Study-126 shared-room topology / source sends / parameter roles
- [x] document browser convolution IR as perceptual target rather than literal FM-1 implementation
- [x] freeze POOL-region storage for large delay memories; ordinary RAM only for small state
- [x] prototype compact modulated FDN
- [x] prototype nested allpass/Schroeder hybrid
- [x] compare diffusion / metallicity / decay / damping / deterministic hash / memory
- [x] select compact 8-line modulated FDN from evidence
- [x] silent real-target memory gate using exact selected arrays
- [x] prove room-disabled target preserves frozen M4g/Phase-5 PCM/runtime
- [x] prove dedicated pre-`.pool` NOLOAD Space storage avoids Dinghy boot-time pool clearing while retaining full POOL-budget accounting
- [x] implement selected 8-line fixed-point FDN with external self-initialising delay storage
- [x] implement source-defined trim / guard / early reflections / predelay / tail HP / FDN damping / filtered echo / wet return
- [x] deterministic host reference + ASAN/UBSAN gates for FDN and full room front-end/topology
- [~] split existing dry render into five post-Mother source contributions and feed one shared room-input accumulator
- [ ] integrate `apsis_space_room_render()` into `app_audio()` using the already-proven dedicated NOLOAD storage
- [ ] real target + emulator audible-room gate
- [ ] room-disabled bypass regression returns exact frozen dry PCM/runtime/cursor
- [ ] freeze Phase-6 target/audio references and guarded merge

Selected memory envelope:
- ordinary RAM stays **87,336 / 98,304 B** during silent storage probe
- Space input history **24,696 B**
- Space echo **211,680 B**
- Space FDN tail **38,340 B**
- total POOL-region use **305,436 / 344,064 B**
- remaining pool headroom **38,628 B**

Silent target gate head `11ce731a740d11262d988c352a79774bbea3103a`:
- core #375 PASS
- staged #364 PASS
- image 100,944 B
- RAM 87,336 B
- pool 305,436 B
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- cursor 123,648; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Current pure-room host head `014c6f843d6e8e7a15ac35e9bbc372e460756e9c`:
- core #398 PASS normal + ASAN/UBSAN
- staged #387 PASS existing dry target/emulator + silent storage gate
- FDN default peak/hash **217 / `a190ca86`**
- full room default peak/hash **47 / `1350a6da`**
- full room first non-zero sample **927**
- no-echo hash `97669507`; dark hash `5ff1f59a`

Do **not** create unrelated per-source reverbs. Do **not** put large delay arrays in `.bss`. Do **not** place Space delay storage inside Dinghy's ordinary zeroed `.pool` range; it must remain in the dedicated pre-`.pool` NOLOAD section and be self-initialised/validity-tracked by Space.

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

1. Refactor the target audio render so String, Drift, Clock, Spiral and Bloom are available as separate post-Mother stereo contributions **without changing their summed dry PCM**.
2. Multiply those five contributions by the frozen source sends and sum them into one shared Space input.
3. Bind the dedicated NOLOAD history/echo/FDN storage and call `apsis_space_room_render()` after dry source rendering.
4. Add a CI-only room-disabled target variant requiring exact frozen Phase-5/M4g dry PCM/runtime/cursor.
5. Add an audible-room target/emulator scenario and only then freeze Phase-6 references / merge PR #16.
