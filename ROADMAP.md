# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed
- [~] active / partially implemented / merge pending
- [ ] not started

## Phase 0 - reference freeze

- [x] lock `apsis.html` hash/size/line landmarks
- [x] behavioural map
- [x] parity matrix
- [x] port contract and UI mapping
- [x] verify 2026-10-29 attached `index.html` is the same frozen source modulo LF/CRLF
- [x] lock Study 98 as String acoustic reference only

## Phase 1 - pinned FM-1 skeleton

- [x] pin Dinghy baseline
- [x] unchanged host baseline
- [x] unchanged real target baseline
- [x] APSIS product identity `FM-1_972`
- [x] host overlay
- [x] CI staged validation
- [x] emulator smoke

## Phase 2 - pure musical/state core

- [x] defaults/versioned state
- [x] scales/tuning
- [x] Gravity model/helpers
- [x] String pitch model
- [x] Drift model
- [x] Clock model
- [x] Spiral route/attractor model
- [x] Bloom harmony/voice-leading
- [x] KEEP
- [x] CHAIN
- [x] MOTHER

Merged through commit `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

## Phase 3 - deterministic transport

### M3a - shared sample-referenced event grid

- [x] audit central browser `tick()` and Gravity boundary rules
- [x] correct native String timing-state representation gaps found by audit
- [x] fixed-capacity event queue design
- [x] long-running sample + Q32 fractional timeline design
- [x] implement Clock pulse/swing/subdivision/event generation
- [x] implement transport regression test source
- [x] add transport to desktop Makefile test/sanitizer targets
- [x] source/core regression passes in GitHub Actions
- [x] ASAN/UBSAN passes in GitHub Actions
- [x] add transport files to `ci/apply_apsis.sh`
- [x] add transport to `overlay/dinghy.c` single translation unit
- [x] run real JieLi target gate with transport compiled and record resource use
- [x] run FM-1 emulator smoke with transport compiled
- [x] finalize PR #4 and squash-merge

Merged to `main` at `fe2911c43fd69e63839220482e50bb10c7a2e4c2`. Final rebased PR head passed `APSIS core` #60 and staged validation #50 through emulator.

### M3b - structural/runtime consumers

Merged to `main` at `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5.

- [x] establish M3b project/source reference documentation
- [x] instantiate shared transport/event queue in FM-1 runtime
- [x] add runtime coordinator regression tests
- [x] Mother horizon + next-1 commit on shared timeline
- [x] Mother blocked-dispatcher retry/recovery without losing pending state
- [x] Chain pulse/cell transitions on shared timeline
- [x] preserve browser Chain meter snapshot after start
- [x] Gravity immediate vs quantized pending commit
- [x] Spiral attractor progression and symbolic event scheduling
- [x] Bloom occurrence and voice-plan scheduling
- [x] preserve source-derived boundary/lead semantics without browser timer jitter
- [x] integrate silent timeline driver into `app_audio()`
- [x] remove target-incompatible live 64-bit division helpers while preserving Q32 timing
- [x] measure live runtime RAM footprint
- [x] measure host stress queue high-water: 14 / 256, no overflow
- [x] measure emulator target-RAM live queue high-water: 7 / 256, no overflow
- [x] full M3b target/emulator implementation gate (`APSIS core` #81, staged #71)
- [x] final documentation-head gate (`APSIS core` #84, staged #74)
- [x] mark PR #5 ready and squash-merge

Final implementation/evidence head: `001d69a758369e2d14665ba2fd176c1fd116990b`.
Final documentation head: `6a8a182cbd0559cc2164c78bf956fbba7e0ee919`.

### M3c - late binding and long timing trace

Active branch: `m3c-late-binding`, based on M3b merge `a61a40dc83c9b52129f7d3f920da26a9a61837d9`.

- [~] audit exact Study 126 Spiral/Bloom late-bind state-observation points
- [ ] document occurrence-time vs bind-time frozen/re-read state contract
- [ ] add source-derived regression expectations before implementation
- [ ] Spiral final pitch resolution against latest committed Gravity/Tuning
- [ ] Bloom per-petal late pitch resolution
- [ ] deterministic random source traceability
- [ ] fixed-capacity event queue stress/high-water tests with late-bind events
- [ ] long deterministic reference trace
- [ ] full Phase 3 target/emulator validation

M3c remains intentionally silent. Do not start Phase 4 DSP until M3c is merged and green.

## Phase 4 - five dry source engines

- [ ] String — Study 98 acoustic target inside Study 126 behavioural contract
- [ ] Drift
- [ ] Clock
- [ ] Spiral
- [ ] Bloom

No shared Space yet. Do not begin Phase 4 until M3b/M3c transport/runtime semantics are proven.

## Phase 5 - live structural behaviour / late binding integration

- [ ] integrate pending Gravity with sounding sources
- [ ] integrate Mother held structural edits with audio engine
- [ ] integrate Chain changes with audio/event engine
- [ ] prove last-moment Spiral/Bloom binding

## Phase 6 - shared Space / Cathedral

- [ ] prototype compact modulated FDN
- [ ] prototype nested allpass/Schroeder candidate
- [ ] compare diffusion/metallicity/decay/CPU/memory
- [ ] implement one shared room with five per-source sends

## Phase 7 - persistent KEEP / CHAIN storage

- [ ] 8-slot project persistence
- [ ] 32-cell chain persistence
- [ ] versioned CRC records
- [ ] two-slot/journal recovery
- [ ] power-loss/corruption tests

## Phase 8 - native APSIS visual surfaces

- [ ] MOTHER
- [ ] GRAVITY
- [ ] STRING
- [ ] CLOCK
- [ ] SPIRAL
- [ ] BLOOM
- [ ] DRIFT/TUNING
- [ ] CHAIN
- [ ] KEEP
- [ ] SPACE secondary page

## Phase 9 - physical FM-1 interaction parity

- [ ] Clock direct key grid
- [ ] Gravity eight-point play
- [ ] String key surface + encoder slide
- [ ] Spiral route editing
- [ ] Bloom petal/core/grow native gesture
- [ ] KEEP / CHAIN / MOTHER physical mappings

## Phase 10 - String Motion and advanced interactive parity

- [ ] arm on next Clock 1
- [ ] 1/2/4/8-cycle capture
- [ ] Classic/Dual and BODY/WIRE/BOTH ownership
- [ ] frozen pitch context
- [ ] overdub preservation
- [ ] manual override
- [ ] String/Spiral attractor editors
- [ ] Gravity point editor

## Phase 11 - FM-1 optimisation

- [ ] ISR timing
- [ ] RAM/pool/stack headroom
- [ ] queue high-water
- [ ] voice-pool stress
- [ ] UI redraw load
- [ ] Cathedral resource tuning

## Phase 12 - hardware validation/release

- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] exact release SHA-256

Physical hardware is not currently available, so no item in Phase 12 is verified.
