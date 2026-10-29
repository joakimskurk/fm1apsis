# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed and merged
- [~] active / partially implemented
- [ ] not started

## Phase 0 - reference freeze

- [x] lock `apsis.html` hash/size/line landmarks
- [x] behavioural map
- [x] parity matrix
- [x] port contract and UI mapping

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
- [~] finalize M3a PR #4 and merge

Active branch: `m3-transport-core`. Target/emulator-verified implementation commit: `17b073e4174d7d27144d64a455e4080b5b777ea3`. See `PROJECT_STATE.md` for exact continuation.

### M3b - structural/runtime consumers

- [ ] instantiate shared transport/event queue in FM-1 runtime
- [ ] Mother horizon + next-1 commit on shared timeline
- [ ] Chain pulse/cell transitions on shared timeline
- [ ] Gravity immediate vs quantized pending commit
- [ ] Spiral attractor progression and symbolic event scheduling
- [ ] Bloom occurrence and voice-plan scheduling
- [ ] preserve source-derived boundary/lead semantics without browser timer jitter
- [ ] measure live queue/runtime RAM and queue high-water

### M3c - late binding and long timing trace

- [ ] Spiral final pitch resolution against latest committed Gravity/Tuning
- [ ] Bloom per-petal late pitch resolution
- [ ] deterministic random source(s)
- [ ] fixed-capacity event queue stress/high-water tests
- [ ] long deterministic reference trace
- [ ] full Phase 3 target/emulator validation

## Phase 4 - five dry source engines

- [ ] String
- [ ] Drift
- [ ] Clock
- [ ] Spiral
- [ ] Bloom

No shared Space yet.

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
