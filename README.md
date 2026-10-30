# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Start here

For a new ChatGPT chat or a returning contributor, read these first in order:

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
  - M4e Clock dry engine: implementation complete, PR #11 final documentation gate pending
  - Spiral: not started
  - Bloom: not started
- active branch: `m4e-clock`
- active draft PR: #11
- validated M4e implementation head: `285aacb5309fd9abaa51814e4ea6a09a3e480a10`
- core #225 and staged #214 passed, including real JieLi target, Clock-on emulator and Clock-DSP bypass back to frozen M4d PCM/runtime
- physical FM-1 validation has not yet been performed

See `PROJECT_STATE.md` for the exact continuation point. The next implementation slice after M4e merge is the Spiral dry engine.

## Core rule

APSIS must remain **APSIS**.

This project is not a conventional groovebox reinterpretation and not a generic synth/sequencer using APSIS names. The target is the closest practical 1:1 port of the musical model, timing rules, state relationships, sound roles and visual language of the browser instrument. Only the interaction layer is translated where mouse/touch gestures do not exist on FM-1.

The source instrument is treated as one musical state seen through:

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
- measure RAM, pool and audio render time continuously
- preserve updater/rescue paths
- do not optimise away characteristic behaviour before parity tests exist

The exact pin is recorded in `UPSTREAM.lock.json`.

## FM-1 timing assumption

For the pinned Dinghy generation, the application audio callback runs at **44.1 kHz with 256-frame blocks**, about 5.80 ms per callback. APSIS initially targets comfortably below the full deadline under worst-case load, with <= ~50% of callback time as the provisional engineering target until hardware measurements justify another budget.

No real ISR-cycle margin is claimed yet because physical hardware is not available.

## Project documents

Root continuity files:
- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `TESTING.md`
- `CHANGELOG.md`

Detailed design/reference files include:
- `docs/PORT_CONTRACT.md` - what 1:1 means and what may / may not change
- `docs/ARCHITECTURE.md` - native engine decomposition
- `docs/UI_MAPPING.md` - FM-1 physical interaction model
- `docs/PLAN.md` - detailed staged implementation plan and exit gates
- `docs/PARITY_MATRIX.md` - feature-by-feature audit checklist
- `docs/reference/SOURCE_LOCK.md` - frozen browser source identity/landmarks
- `docs/reference/BEHAVIOR_MAP.md` - source-derived behaviour/default map
- `docs/reference/M4D_DRIFT_CONTRACT.md` - persistent Drift parity disposition/contract
- `docs/reference/M4E_CLOCK_CONTRACT.md` - Clock dry-engine acoustic/timing/random contract

## Important design consequence

The 240x240 screen should draw real APSIS surfaces - String, orbiting Clock, Spiral, Bloom and Gravity - in a reduced native form. It should **not** replace them with generic parameter lists as the primary interface. Parameters may live on secondary pages, but the playable surface remains the instrument.

## Reference source

The browser `apsis.html` supplied to the project is the behavioural source of truth for this port. It is intentionally not copied into this public repository.

Frozen reference SHA-256:
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

Study 98 is separately locked only as the narrow String acoustic reference; it does not supersede Study 126 behavior/timing/state authority.
