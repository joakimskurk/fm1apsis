# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 3 / M3a - deterministic sample-referenced transport core.**

Phase 2 musical/state core is complete and merged to `main` at `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

M3a is implemented and verified through the FM-1 emulator on:
- branch `m3-transport-core`
- PR #4: `M3a: deterministic sample-referenced transport core`
- target/emulator-verified implementation HEAD `17b073e4174d7d27144d64a455e4080b5b777ea3`

Documentation-only commits after the verified implementation commit are expected before merge.

## Product invariant

APSIS must remain a distinct instrument and as close to a 1:1 port of browser `apsis.html` as practical. Adapt physical interaction to FM-1, but do not simplify APSIS into a generic synth/groovebox or collapse identity-bearing systems.

Locked browser reference:
- `apsis.html`
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- 606,747 bytes / 5,742 lines
- see `docs/reference/SOURCE_LOCK.md` and `docs/reference/BEHAVIOR_MAP.md`

Pinned FM-1 baseline:
- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS product identity `FM-1_972`

## What is complete

### Phase 1
Pinned Dinghy build skeleton, APSIS overlay/identity, host build, real JieLi target build, emulator smoke and CI validation pipeline.

### Phase 2
Pure musical/state core merged to `main`:
- versioned defaults/state
- scales/tuning
- Gravity point model and voicing helpers
- String position/ratio/pitch model
- Drift delta model
- Clock structure/grouping and swing interval law
- Spiral route/derived sequence and deterministic attractor state
- Bloom harmony and voice-leading planner
- KEEP partial musical-memory semantics
- CHAIN cell/lane/morph semantics
- MOTHER pending queue/horizon semantics

M2c verification before merge:
- strict core tests PASS
- ASAN/UBSAN PASS
- unchanged Dinghy host + target PASS
- APSIS single-translation-unit host PASS
- real `FM-1_972` target package PASS
- FM-1 emulator smoke PASS
- image 56,700 B
- RAM 14,104 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA-256 `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`

## M3a implementation

Implemented on `m3-transport-core`:
- `apsis/apsis_transport.h`
- `apsis/apsis_transport.c`
- `tests/test_transport.c`
- transport included in root Makefile test/sanitizer build
- transport copied into the Dinghy overlay by `ci/apply_apsis.sh`
- transport included in the real `overlay/dinghy.c` single translation unit
- fixed-capacity 256-entry event min-heap
- absolute transport time as 64-bit whole samples + Q32 fractional sample
- BPM 35..180 and 1 / 1/2 / 1/4 division semantics
- alternating browser swing law
- 3..24 pulse Clock cycles
- sample-referenced events for step-one resync, String filter clock, playhead, Mother, Clock main/accent, Spiral subdivisions, String rhythm, Bloom, snap, inner/between, ghost and Chain
- Gravity cycle division normalization and future boundary prediction
- deterministic injected ghost random value for testing
- long-running sample-counter tests

State representation was also corrected so native String state can represent browser timing options: rhythm `off/bow/pizz/motion`, `every/sequence`, 24-pulse rhythm/filter masks, filter `off/sequence/attractor`, attractor cadence `0.5/1/2/4/8`, and pulse diagnostic modes.

## Browser timing facts already audited

Browser `tick()` schedules about 1.2 seconds ahead. Logical pulse order:
1. optional step-one resync
2. String Clock/filter event
3. playhead / String Motion pulse
4. Mother pulse / possible commit
5. main Clock percussion
6. Spiral subdivisions/events
7. String rhythm
8. Bloom occurrence
9. snap / between / ghost
10. counters advance
11. Chain pulse / possible cell transition

Do not infer final state-change priority solely from JavaScript call order. Browser lead timers matter:
- Spiral late pitch rebind ~80 ms desktop / 110 ms narrow UI
- Bloom occurrence 45 ms; individual Bloom voice bind ~50/70 ms
- Mother pulse 26 ms
- Chain pulse 24 ms

Native transport should preserve musical boundary semantics, not main-thread timer jitter.

## Verification status

### Merged `main`
Phase 2/M2c is automated-test, sanitizer, target-build and emulator-smoke verified.

### M3a
M3a is now **verified through emulator**.

Verified implementation commit:
- `17b073e4174d7d27144d64a455e4080b5b777ea3`

GitHub Actions evidence:
- `APSIS core` run #59: regression + `tests/test_transport.c` + ASAN/UBSAN PASS
- `APSIS staged validation` run #49: every gate PASS
  - source core + pinned upstream
  - unchanged Dinghy host + real target
  - APSIS overlay host shell
  - real JieLi `FM-1_972` target package
  - FM-1 emulator smoke

M3a target evidence:
- app image: 56,680 B / 581,564 B (20 B smaller than M2c)
- RAM `.data + .bss`: 14,104 B / 98,304 B (no change from M2c)
- RAM headroom: 84,200 B
- pool: 30,720 B / 344,064 B (no change from M2c)
- pool headroom: 313,344 B
- package bytes: 610,110 B
- product identity: `FM-1_972`
- package SHA-256: `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`

M3a emulator evidence:
- boot/input/silent-audio contract PASS
- stereo channels: 2
- WAV frames: 124,928
- WAV peak: 0, expected because M3a remains intentionally silent

Important resource caveat: M3a compiles `apsis_transport.[ch]` in the real target single translation unit, but the transport/event queue is intentionally not yet consumed or instantiated by `app_audio()`. The unchanged RAM/pool figures therefore do **not** represent the eventual live 256-event queue/runtime footprint; that becomes measurable when M3b wires the shared timeline into runtime state.

No physical FM-1 validation has been performed; hardware is not currently available.

## Incomplete / current risks

M3a transport itself is target/emulator-proven. Remaining runtime work belongs to M3b/M3c:
- transport is not yet consumed by `app_audio()`
- Chain/Mother/Gravity/Spiral/Bloom runtime consumers are not yet wired to the shared event stream
- no DSP/audio yet; M3 remains intentionally silent
- live queue RAM/high-water cost is not measurable until the runtime queue is instantiated
- same-sample priority between Mother/Chain commits and late-bound Spiral/Bloom pitch must be proven when consumers are wired
- native randomness must remain explicit/deterministic rather than implicit `Math.random()`

## Exact next action

1. Commit the M3a verification documentation and ensure CI remains green on the final PR head.
2. Mark PR #4 ready for review/finalization.
3. Squash-merge PR #4 to `main` once required checks are green.
4. Start M3b from updated `main`.
5. M3b first task: instantiate the shared transport/event queue and wire structural/runtime consumers for Mother/Chain/Gravity/Spiral/Bloom without adding DSP.
6. Measure live queue/runtime RAM and high-water once the queue is actually instantiated.
7. Preserve late-binding semantics for Spiral/Bloom into M3c before any Phase 4 sound engine work.

## Do not regress

- Do not begin DSP before deterministic transport is proven. M3a now satisfies that transport gate; Phase 4 DSP still waits for M3b/M3c runtime timing semantics.
- Do not replace the shared scheduler with per-module clocks.
- KEEP is partial musical memory, not a full preset.
- CHAIN retains lane scopes/chance/transpose/morph/sync semantics.
- Mother mixer/master remain live while `->1` holds structural edits.
- Spiral and Bloom retain late binding against committed Gravity/Tuning.
- `stepOneResync` is OFF by default.
- Shared Cathedral/SPACE comes later as one shared room with per-source sends.
