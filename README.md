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
- Phase 4 five dry source engines: complete and merged through M4g Bloom
- Phase 5 sounding-source structural integration: **technically complete; final documentation-head double gate + PR #15 merge pending**
- active branch: `p5-sounding-integration`
- active draft PR: #15
- Phase-5 verification head before docs freeze: `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`
- core #357 + staged #346 PASS
- physical FM-1 validation has not yet been performed

Phase 5 is intentionally verification-only: the existing M3c shared runtime plus M4f/M4g sounding bridges already satisfy source-defined structural timing. P5a–P5f prove that behaviour reaches actual DSP voices without changing the production firmware.

After PR #15 merges from an exact double-gated documentation head, the roadmap proceeds to **Phase 6 shared Space/Cathedral**.

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

## Source authority

Study 126 is behavioural/state/timing/safety authority. Frozen SHA-256:
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

Study 98 is separately locked only as the narrow String acoustic reference for features that actually existed there. LF SHA-256:
`43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Study 98 does not supersede Study 126 behaviour/timing/state authority.

## Platform baseline

- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS product identity `FM-1_972`
- 44.1 kHz / 256-frame audio callbacks
- no allocation, flash access, screen drawing or waiting inside audio render
- fixed-capacity queues and voice pools
- updater/rescue paths preserved

The exact pin is recorded in `UPSTREAM.lock.json`.

## Timing/runtime baseline

M3c remains the only musical scheduler:
- `sizeof(ApsisRuntime)=21,928 B`
- 512-entry shared event heap
- no module-private clocks
- random trace `78771ff5`
- late trace `0e6fc422`

The pinned three-second emulator scenario freezes runtime cursor **123,648 exactly**. This catches callback-budget regressions that a weaker “cursor advanced” check would miss.

No real ISR-cycle margin is claimed because physical hardware is unavailable.

## Phase-4 dry-engine baseline

Merged dry engines:
- M4a String persistent `2ea217f0…`
- M4b String finite/Rhythm `c96c00de…`
- M4c String Attractor `6fcdd8ff…`
- M4d Drift `d7e39e24…`
- M4e Clock `db9d0b3e…`
- M4f Spiral `734229f4…`
- M4g Bloom `713a1e9a…`

Frozen M4g target:
- image 100,944 B
- RAM 87,336 / 98,304 B
- **static RAM headroom only 10,968 B**
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- default PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- runtime cursor 123,648; queue 28/512; late trace `811c9dc5`

## Phase 5 — sounding structural semantics

P5a–P5f prove actual sounding DSP obeys the frozen structural model:
- immediate late binding reaches finite Spiral/Bloom oscillators;
- quantized Gravity preserves the deliberate Spiral-vs-Bloom pending-state asymmetry;
- Mother-held Gravity does not become audible before `->1` commit;
- Chain discrete state reaches sounding Spiral/Bloom in source order;
- Chain continuous Drift morph begins at the target and reaches the persistent Drift renderer;
- step-one RESYNC OFF preserves free-running Spiral attractor phase into sounding energy.

No production code was required for these proofs. Staged #346 rebuilt the exact M4g target/package/PCM/runtime, so Phase 5 consumes **0 additional static RAM**.

Normative starting contract: `docs/reference/P5A_SOUNDING_LATE_BINDING_CONTRACT.md`.

## Phase 6 consequence

Space/Cathedral is next after Phase 5 merges. It must remain **one shared room with five per-source sends**.

Because only 10,968 B of static target RAM remains, Phase 6 begins with source/topology freeze and memory prototypes, not immediate reverb integration. Compact modulated FDN and nested allpass/Schroeder candidates must be compared for diffusion, metallicity, decay, CPU and memory before choosing an implementation.

## Project documents

Root continuity files:
- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `TESTING.md`
- `CHANGELOG.md`

Detailed references include:
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
- `docs/reference/P5A_SOUNDING_LATE_BINDING_CONTRACT.md`

## Important visual consequence

The 240x240 screen should draw real reduced APSIS surfaces — String, orbiting Clock, Spiral, Bloom and Gravity — rather than replacing them with generic parameter lists as the primary interface.
