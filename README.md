# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Status

Planning / reference freeze. No DSP implementation has been accepted yet.

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

The initial port should follow the proven SkurkyMello discipline:

- build on a **pinned Dinghy baseline**, not a floating checkout;
- verify unchanged Dinghy host + target before applying the APSIS overlay;
- keep audio/state/UI code testable on host before device optimisation;
- no allocation, flash access, screen drawing or waiting inside audio render;
- use fixed-capacity queues and voice pools;
- measure RAM, pool and audio render time continuously;
- preserve the updater/rescue path and use a unique FM-1 package identity;
- do not optimise away characteristic behaviour before parity tests exist.

Current reference baseline candidate: Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`, the same pinned baseline currently used by the SkurkyMello FM-1 project. Final lock will be recorded in `UPSTREAM.lock.json` when implementation begins.

## FM-1 timing assumption

For the pinned Dinghy generation, the application audio callback runs at **44.1 kHz with 256-frame blocks**. That is about 5.80 ms of audio per callback. APSIS should target substantially less than the full deadline under worst-case load; the initial engineering target is <= ~50% of the callback period until hardware measurements justify another budget.

## Documents

- `docs/PORT_CONTRACT.md` — what “1:1” means and what may / may not change
- `docs/ARCHITECTURE.md` — proposed native engine decomposition
- `docs/UI_MAPPING.md` — FM-1 physical interaction model
- `docs/PLAN.md` — staged implementation plan and exit gates
- `docs/PARITY_MATRIX.md` — feature-by-feature audit checklist

## Important design consequence

The 240×240 screen should draw real APSIS surfaces — String, orbiting Clock, Spiral, Bloom and Gravity — in a reduced native form. It should **not** replace them with generic parameter lists as the primary interface. Parameters may live on secondary pages, but the playable surface remains the instrument.

## Reference source

The current browser `apsis.html` supplied to the project is the behavioural source of truth for this port. It is intentionally not copied into this public repository during the planning phase.
