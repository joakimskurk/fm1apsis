# PROJECT_STATE

Last updated: 2026-10-23

## Current milestone

**Phase 3 / M3a - deterministic sample-referenced transport core.**

Phase 2 musical/state core is complete and merged to `main` at `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

Unfinished work is intentionally checkpointed on:
- branch `m3-transport-core`
- draft PR #4: `M3a: deterministic sample-referenced transport core`
- last transport code/test commit before handoff documentation: `17faba6bd944726c804d52e5456afaadf3e99093`

A new chat should inspect the current branch HEAD and PR #4 before changing code. Documentation-only commits after the code commit are expected.

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

## Current M3a branch state

Implemented on `m3-transport-core`:
- `apsis/apsis_transport.h`
- `apsis/apsis_transport.c`
- `tests/test_transport.c`
- transport included in root Makefile test/sanitizer build
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

### Current M3a code
The new transport source and regression suite have now passed GitHub Actions workflow **APSIS core**, including:
- source-derived core regression: PASS
- ASAN/UBSAN: PASS

Evidence run: `APSIS core` run #49 on branch commit `338d8fb863fab31951f2077382c4e692ef395f6e`. Later handoff commits are documentation-only and do not change transport code.

Transport is **not yet target-validated** because it is still absent from the Dinghy overlay copy/include path. A staged build that does not compile `apsis_transport.[ch]` into `overlay/dinghy.c` does not count as target verification of M3a.

No physical FM-1 validation has been performed; hardware is not currently available.

## Incomplete / current risks

M3a is checkpointed but **not complete or merge-ready**:
- `apsis_transport.[ch]` is not yet copied by `ci/apply_apsis.sh`
- transport is not yet included in the real FM-1 single-translation-unit `overlay/dinghy.c`
- transport is not yet consumed by `app_audio()`
- Chain/Mother/Gravity/Spiral/Bloom runtime consumers are not yet wired to the shared event stream
- no DSP/audio yet; M3 remains intentionally silent

Risks to test before merge:
- compile/symbol issues when transport joins the single translation unit
- RAM/code cost of the 256-entry event queue
- same-sample priority between Mother/Chain commits and late-bound Spiral/Bloom pitch
- queue high-water under 4x Spiral plus secondary events and later bound events
- native randomness must remain explicit/deterministic rather than implicit `Math.random()`

## Exact next action

1. Checkout/inspect `m3-transport-core` / draft PR #4.
2. Read `apsis/apsis_transport.[ch]`, `tests/test_transport.c`, `Makefile`, `ci/apply_apsis.sh`, `overlay/dinghy.c` and source-lock docs.
3. Add `apsis_transport.h/.c` to `ci/apply_apsis.sh`.
4. Include `apsis_transport.h/.c` in `overlay/dinghy.c` so the actual single-translation-unit FM-1 build compiles the transport.
5. Run the complete staged validation again: core, pinned upstream, unchanged Dinghy, APSIS host, real JieLi `FM-1_972` target, emulator smoke.
6. Fix any single-TU/target issue before expanding M3 functionality.
7. Record target image/RAM/pool deltas and package SHA in `TESTING.md` / this file.
8. Only after every gate is green: mark PR #4 ready, finalize M3a and squash-merge.
9. Next milestone after merge: M3b runtime consumers for Mother/Chain/Gravity/Spiral/Bloom on the shared event timeline.

## Do not regress

- Do not begin DSP before deterministic transport is proven.
- Do not replace the shared scheduler with per-module clocks.
- KEEP is partial musical memory, not a full preset.
- CHAIN retains lane scopes/chance/transpose/morph/sync semantics.
- Mother mixer/master remain live while `->1` holds structural edits.
- Spiral and Bloom retain late binding against committed Gravity/Tuning.
- `stepOneResync` is OFF by default.
- Shared Cathedral/SPACE comes later as one shared room with per-source sends.
