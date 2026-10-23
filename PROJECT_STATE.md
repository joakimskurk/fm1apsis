# PROJECT_STATE

Last updated: 2026-10-23

## Current milestone

**Phase 3 / M3a - deterministic sample-referenced transport core.**

Phase 2 musical/state core is complete and merged to `main` at `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

Active branch: `m3-transport-core`.

## Product invariant

APSIS must remain a distinct instrument and as close to a 1:1 port of browser `apsis.html` as practical. Adapt the physical interaction to FM-1, but do not simplify APSIS into a generic synth/groovebox or collapse identity-bearing systems.

Locked browser reference:
- `apsis.html`
- SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- 606,747 bytes / 5,742 lines
- see `docs/reference/SOURCE_LOCK.md` and `docs/reference/BEHAVIOR_MAP.md`

Pinned FM-1 baseline:
- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS product identity `FM-1_972`

## What is complete

### Phase 1

Pinned Dinghy build skeleton, APSIS overlay/identity, host build, real JieLi target build, emulator smoke and CI validation pipeline are established.

### Phase 2

The pure musical/state core is implemented and merged:
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

M2c validation before merge:
- strict core tests: PASS
- ASAN/UBSAN: PASS
- unchanged Dinghy host + target: PASS
- APSIS single-translation-unit host: PASS
- real `FM-1_972` target package: PASS
- FM-1 emulator smoke: PASS
- measured target at M2c: image 56,700 B, RAM 14,104 B / 98,304 B, pool 30,720 B / 344,064 B
- M2c package SHA-256: `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`

## Current M3a implementation state

The browser scheduler has been audited against `apsis.html` around the central `tick()` path and quantized Gravity timing.

Implemented on `m3-transport-core`:
- `apsis/apsis_transport.h`
- `apsis/apsis_transport.c`
- `tests/test_transport.c`
- transport added to the host Makefile test/sanitizer build
- fixed-capacity 256-entry event min-heap
- absolute transport time stored as 64-bit whole samples plus Q32 fractional sample
- BPM clamp 35..180
- division semantics 1, 1/2, 1/4
- alternating browser swing law
- 3..24 pulse Clock cycle handling
- sample-referenced event generation for:
  - step-one resync
  - String filter clock event
  - playhead
  - Mother pulse
  - Clock main/accent
  - Spiral 1x/2x/4x subdivisions
  - String rhythm offset
  - Bloom occurrence/delay
  - Clock snap
  - Clock inner/between
  - Clock ghost
  - Chain pulse
- Gravity cycle-division normalization and future boundary prediction
- deterministic injected ghost random draw for testability
- long-running sample counter test coverage

State representation was corrected during scheduler audit so native state can represent browser timing options that the early skeleton could not:
- String rhythms `off/bow/pizz/motion`
- rhythm trigger `every/sequence`
- 24-pulse rhythm mask
- String filter `off/sequence/attractor`
- 24-pulse filter mask
- filter attractor cadence `0.5/1/2/4/8`
- String pulse diagnostic modes

## Important browser timing facts already audited

Browser `tick()` schedules about 1.2 s ahead. The musical pulse ordering around each Clock pulse is:
1. optional step-one resync
2. String Clock/filter event
3. playhead / String Motion pulse
4. Mother pulse bookkeeping/possible structural commit
5. main Clock percussion
6. Spiral attractor subdivisions/events
7. String rhythm
8. Bloom occurrence
9. snap / between / ghost Clock events
10. counters advance
11. Chain pulse bookkeeping/possible cell transition

Do not interpret JavaScript call order alone as final state-change order. Browser lead timers matter:
- Spiral late pitch rebind lead: ~80 ms desktop / 110 ms narrow UI
- Bloom occurrence lead: 45 ms, individual Bloom voice bind lead ~50/70 ms
- Mother pulse lead: 26 ms
- Chain pulse lead: 24 ms

Native transport should preserve musical boundary semantics rather than copy browser main-thread timer jitter.

## What is incomplete right now

M3a is **checkpointed but not yet complete**.

Specifically:
- the new transport core has not yet been run through the full GitHub Actions validation gates after the latest M3a commits
- `apsis_transport.[ch]` has not yet been added to `ci/apply_apsis.sh`
- `apsis_transport.[ch]` is not yet included in the real FM-1 single-translation-unit `overlay/dinghy.c`
- the transport is not yet consumed by `app_audio()`
- no audio/DSP is produced yet; M3 remains intentionally silent
- Chain/Mother runtime consumers are not yet wired to transport events
- Gravity pending commit is not yet wired to the event stream
- Spiral attractor mutation/late-binding runtime and Bloom late-binding runtime are not yet driven by transport
- String Motion pulse runtime is still later work

## Known bugs / risks

No confirmed runtime bug is currently recorded on `main`.

Current M3a risks to check before merge:
- compile errors or single-translation-unit symbol collisions when transport is added to the FM-1 overlay
- target RAM/code growth from the 256-entry event queue
- exact same-sample priority interactions between Mother/Chain state commits and late-bound Spiral/Bloom pitch resolution
- event queue sizing under 4x Spiral + Clock secondary events + future late-bound events
- browser `Math.random()` behaviour should not leak into deterministic native scheduling; random sources need explicit state/injection

## Verification status

### Verified on merged `main`

Phase 2 through M2c is fully automated-test, sanitizer, target-build and emulator-smoke verified. No physical FM-1 validation has been performed.

### Current branch `m3-transport-core`

Implemented and committed, but the latest transport work is **not yet fully CI-verified**. Do not claim M3a complete or merge it until the new transport tests, sanitizer, real target and emulator gates pass.

## Current work item

Finish **M3a: deterministic transport/event grid**.

## Exact next action

1. Inspect `apsis/apsis_transport.[ch]`, `tests/test_transport.c` and the latest `Makefile` on branch `m3-transport-core`.
2. Run/trigger host `make test` and `make sanitize` for the branch.
3. Fix any transport test/compiler issue before doing further architecture work.
4. Add `apsis_transport.h/.c` to `ci/apply_apsis.sh`.
5. Include `apsis_transport.h/.c` in `overlay/dinghy.c` so the real single-translation-unit FM-1 build sees it.
6. Run the complete existing staged validation: pinned upstream, unchanged Dinghy, APSIS host, real JieLi `FM-1_972` target, emulator smoke.
7. Record target resource numbers and package SHA.
8. Only after all gates are green, update this file / `TESTING.md`, open or finalize the M3a PR and merge.
9. Then proceed to M3b: runtime consumers for Mother/Chain/Gravity/Spiral/Bloom on the shared event timeline.

## Do not regress

- Do not begin DSP before deterministic transport is proven.
- Do not replace the shared scheduler with per-module clocks.
- Do not reduce APSIS to generic parameter pages or a generic sequencer.
- Do not make KEEP a full preset; it remains partial musical memory.
- Do not make CHAIN a generic scene sequencer; preserve lane scopes, chance/transpose/morph and sync rules.
- Mother mixer/master remain live even when `->1` holds structural edits.
- Spiral and Bloom must retain late binding against committed Gravity/Tuning.
- `stepOneResync` is OFF by default.
- Shared Cathedral/SPACE comes later and must be one shared room with per-source sends.
- Physical hardware is not available yet; emulator/host/CI are the active verification surfaces.
