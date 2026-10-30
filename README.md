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
  - M4e Clock: merged
  - M4f Spiral: merged as `734229f4d23626a176a8366de82aab2c58c48cc9`
  - M4g Bloom: implementation complete; PR #13 pending only final documentation-head double gate and guarded merge
- active branch: `m4g-bloom`
- active draft PR: #13
- technically final M4g implementation/CI head: `6c54d1cd5ca354543237a14c257b3ec28ab424d1`
- core #292 and staged #281 passed, including real JieLi target, emulator and exact frozen-M4f Bloom bypass
- physical FM-1 validation has not yet been performed

See `PROJECT_STATE.md` for the exact continuation point. After M4g merges, the roadmap proceeds to **Phase 5 sounding-source structural integration**, not directly to shared Space/Cathedral.

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

The pinned three-second emulator scenario freezes **runtime cursor 123,648 exactly**. This catches dry-engine callback-budget regressions that a weaker “cursor advanced” check would miss.

No real ISR-cycle margin is claimed because physical hardware is unavailable.

## Phase-4 dry engines

### String

Study 98 is the narrow acoustic authority for old String features; Study 126 remains behavioural/timing/state/safety authority.

Merged M4a–M4c provide persistent Foundation/Classic/Dual, finite transients/Rhythm and String Attractor excitation.

### Drift

M4d restores the identity-bearing paired beating field around the tuned Gravity root as an explicit documented source-wiring repair. Merged as `d7e39e2481c33628f402bed51d761b8be8304bb3`.

### Clock

M4e passively consumes shared Clock events with STONE/WOOD/GLASS/SOFT voices and deterministic Clock-local Snap noise. Merged as `db9d0b3ecbc7931be343cec7717103059d3c5160`.

### Spiral

M4f reuses the existing M3c occurrence/token/target−80 ms late-bind lifecycle. Acoustic planning state is external and a fixed 32-leg renderer implements THREAD/GLASS/REED/HALO plus source main/shadow topology.

Frozen host refs:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- worst stress 30/32 legs, 90 oscillators, 0 steals, 0 drops, hash `02be4bc5`

Merged M4f target:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`

### Bloom

M4g reuses the existing M3c target / target−45 ms occurrence / causal target−50 ms petal-bind lifecycle. Occurrence freezes petal acoustic geometry; each bind consumes M3c's target-aware Gravity/tuning result and late-reads current Bloom voice/attack/brightness.

Frozen host refs:
- VEIL 175 / `6177470f`
- CHOIR 143 / `4b2969f8`
- ORGAN 122 / `d54a68d9`
- GLASS 129 / `ca1c3127`
- worst legal stress: 125/136 petals, 375 oscillators, 0 drops, hash `5881bb36`

Technically final M4g target:
- image 100,944 B
- RAM 87,336 B
- **RAM headroom only 10,968 B**
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`

The pinned three-second emulator does not reach a due Bloom occurrence, so M4g preserves the exact M4f callback/audio path until Bloom actually activates. Default PCM remains `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b` and runtime cursor remains exactly 123,648.

A dedicated Bloom-DSP/runtime bypass returns exactly to the frozen M4f target/package/PCM/runtime.

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
- `docs/reference/M4G_BLOOM_CONTRACT.md`

## Important design consequence

The 240x240 screen should draw real APSIS surfaces - String, orbiting Clock, Spiral, Bloom and Gravity - in a reduced native form. It should **not** replace them with generic parameter lists as the primary interface.

## Reference source

Study 126 is the behavioural source of truth. Frozen SHA-256:
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

Study 98 is separately locked only as the narrow String acoustic reference; it does not supersede Study 126 behaviour/timing/state authority.
