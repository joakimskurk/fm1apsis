# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed and merged
- [~] active / partially implemented
- [ ] not started

## Phase 0 - reference freeze
- [x] source lock, behaviour map, parity matrix, port contract, UI mapping

## Phase 1 - pinned FM-1 skeleton
- [x] pinned Dinghy, host/target baseline, APSIS `FM-1_972`, CI, emulator smoke

## Phase 2 - pure musical/state core
- [x] defaults/tuning/Gravity/String/Drift/Clock/Spiral/Bloom/KEEP/CHAIN/MOTHER
- [x] merged through `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`

## Phase 3 - deterministic transport

### M3a - shared sample-referenced event grid
- [x] audit browser `tick()` and Gravity boundaries
- [x] correct String timing-state representation gaps
- [x] fixed-capacity event queue
- [x] long-running sample + Q32 fractional timeline
- [x] Clock pulse/swing/subdivision/event generation
- [x] transport regression test source
- [x] transport wired into desktop Makefile test/sanitizer targets
- [~] run/fix new host tests and sanitizers
- [ ] add transport to `ci/apply_apsis.sh`
- [ ] add transport to `overlay/dinghy.c`
- [ ] real JieLi target + resource measurement
- [ ] emulator smoke
- [ ] finalize PR and merge

### M3b - structural/runtime consumers
- [ ] Mother horizon/next-1 commit
- [ ] Chain cell transitions
- [ ] Gravity pending commit
- [ ] Spiral attractor/event scheduling
- [ ] Bloom scheduling

### M3c - late binding / long trace
- [ ] Spiral final pitch late binding
- [ ] Bloom petal late binding
- [ ] deterministic random sources
- [ ] queue high-water/stress tests
- [ ] long deterministic timing trace

## Phase 4 - five dry sources
- [ ] String
- [ ] Drift
- [ ] Clock
- [ ] Spiral
- [ ] Bloom

## Phase 5 - live structural integration
- [ ] Gravity / Mother / Chain + sounding engine
- [ ] prove late binding in live engine

## Phase 6 - shared Space / Cathedral
- [ ] FDN candidate
- [ ] nested allpass/Schroeder candidate
- [ ] choose by sound + CPU/memory
- [ ] one shared room, five sends

## Phase 7 - persistent KEEP / CHAIN storage
- [ ] CRC/versioned two-slot journal and recovery

## Phase 8 - native APSIS visual surfaces
- [ ] MOTHER, GRAVITY, STRING, CLOCK, SPIRAL, BLOOM, DRIFT/TUNING, CHAIN, KEEP, SPACE

## Phase 9 - physical FM-1 interaction parity
- [ ] all primary performance surfaces

## Phase 10 - String Motion + advanced interaction
- [ ] capture/overdub/frozen context/attractor editors

## Phase 11 - optimisation
- [ ] ISR, RAM/pool/stack, queue, voices, UI, Cathedral

## Phase 12 - hardware validation/release
- [ ] physical FM-1 install/rescue/controls/stress/storage/release SHA

See `PROJECT_STATE.md` for exact continuation point and `TESTING.md` for real verification level.
