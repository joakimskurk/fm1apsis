# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Start here

For a new ChatGPT chat or returning contributor, read these first in order:

1. `PROJECT_STATE.md` - current milestone, exact unfinished state and next action
2. `ROADMAP.md` - operational phase/milestone sequence
3. `DECISIONS.md` - durable architecture/product decisions
4. `TESTING.md` - what has actually been verified
5. `CHANGELOG.md` - milestone history
6. relevant files under `docs/` and the active source branch

The repository, not chat history, is the durable source of project state.

## Current status

- Phase 0 reference freeze: complete
- Phase 1 pinned FM-1 skeleton/build/emulator pipeline: complete
- Phase 2 pure musical/state core: complete and merged
- Phase 3 deterministic shared transport/runtime: complete and merged
- Phase 4 dry sources:
  - M4a String persistent: merged
  - M4b String finite/Rhythm: merged
  - M4c String Attractor: merged
  - M4d Drift persistent: merged
  - M4e Clock: merged as `db9d0b3ecbc7931be343cec7717103059d3c5160`
  - M4f Spiral: implementation complete; PR #12 pending only final documentation-head double gate
  - M4g Bloom: not started
- active branch: `m4f-spiral`
- active draft PR: #12
- technically final M4f implementation/CI head: `11569371ea18bc8a5e06f28d09ddef90f9835e29`
- core #260 and staged #249 passed, including real JieLi target, exact default cursor invariant and exact M4e Spiral-DSP bypass
- physical FM-1 validation has not yet been performed

See `PROJECT_STATE.md` for the exact continuation point. The next implementation slice after M4f merge is **M4g / Bloom dry engine**.

## Core rule

APSIS must remain **APSIS**.

This project is not a conventional groovebox reinterpretation and not a generic synth/sequencer using APSIS names. The target is the closest practical 1:1 port of the musical model, timing rules, state relationships, sound roles and visual language of the browser instrument. Only the interaction layer is translated where mouse/touch gestures do not exist on FM-1.

The source instrument is one musical state seen through:

- STRING
- DRIFT / TUNING
- CLOCK
- SPIRAL
- BLOOM
- GRAVITY
- CHAIN
- MOTHER

The first five are audible source roles. GRAVITY and TUNING shape shared pitch/harmony. CHAIN operates on musical-memory lanes. MOTHER is the shared mix/structural-sync layer. The shared CATHEDRAL / SPACE return is part of the instrument identity and is not to be replaced by unrelated per-part effects.

## Platform baseline

The port follows a pinned Dinghy discipline:

- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS FM-1 product identity `FM-1_972`
- verify unchanged Dinghy host + target before applying the APSIS overlay
- keep audio/state/UI code testable on host before device optimisation
- no allocation, flash access, screen drawing or waiting inside audio render
- use fixed-capacity queues and voice pools
- measure RAM, pool and callback progress continuously
- preserve updater/rescue paths
- do not optimise away characteristic behaviour before parity tests exist

The exact pin is recorded in `UPSTREAM.lock.json`.

## Timing/runtime baseline

Pinned Dinghy audio is 44.1 kHz with 256-frame callbacks (~5.80 ms).

The frozen M3c shared runtime remains the only musical scheduler:
- `sizeof(ApsisRuntime)=21,928 B`
- 512-entry shared event heap
- no module-private clocks
- canonical random trace `78771ff5`
- canonical late trace `0e6fc422`

The pinned three-second emulator scenario now also freezes **runtime cursor 123,648 exactly**. This catches dry-engine callback-budget regressions that a weaker “cursor advanced” check would miss.

No real ISR-cycle margin is claimed because physical hardware is unavailable.

## Phase-4 dry engines

### String

Study 98 is the narrow acoustic authority for old String features; Study 126 remains behavioural/timing/state/safety authority.

Merged M4a–M4c provide persistent Foundation/Classic/Dual, finite transients/Rhythm and String Attractor excitation.

### Drift

M4d restores the identity-bearing paired beating field around the tuned Gravity root as an explicit documented source-wiring repair. Merged as `d7e39e2481c33628f402bed51d761b8be8304bb3`.

### Clock

M4e passively consumes the shared Clock events with STONE/WOOD/GLASS/SOFT voices and deterministic Clock-local Snap noise. Merged as `db9d0b3ecbc7931be343cec7717103059d3c5160`.

### Spiral

M4f reuses the existing M3c occurrence/token/target−80 ms late-bind lifecycle. Acoustic planning state is stored externally and a fixed 32-leg renderer implements THREAD/GLASS/REED/HALO plus source main/shadow topology.

Frozen host refs:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- worst stress 30/32 legs, 0 steals, 0 drops, hash `02be4bc5`

Technically final M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`

The default source route is empty, so default emulator PCM remains exact M4e. Audible Spiral is proven by host/full-chain tests plus real target compile/link; physical listening remains unverified.

## Project documents

Root continuity files:
- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `TESTING.md`
- `CHANGELOG.md`

Detailed design/reference files include:
- `docs/PORT_CONTRACT.md`
- `docs/ARCHITECTURE.md`
- `docs/UI_MAPPING.md`
- `docs/PLAN.md`
- `docs/PARITY_MATRIX.md`
- `docs/reference/SOURCE_LOCK.md`
- `docs/reference/BEHAVIOR_MAP.md`
- `docs/reference/M4D_DRIFT_CONTRACT.md`
- `docs/reference/M4E_CLOCK_CONTRACT.md`
- `docs/reference/M4F_SPIRAL_CONTRACT.md`

## Important design consequence

The 240x240 screen should draw real APSIS surfaces - String, orbiting Clock, Spiral, Bloom and Gravity - in a reduced native form. It should **not** replace them with generic parameter lists as the primary interface.

## Reference source

Study 126 is the behavioural source of truth. Frozen SHA-256:
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

Study 98 is separately locked only as the narrow String acoustic reference; it does not supersede Study 126 behaviour/timing/state authority.
