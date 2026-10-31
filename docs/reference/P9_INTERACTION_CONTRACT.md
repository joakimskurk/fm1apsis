# Phase 9 Interaction Contract — FM-1 Physical Parity

Date: 2026-10-31
Status: P9A global panel/navigation, P9B CLOCK/GRAVITY, and P9C1 STRING-coordinate host contracts frozen; later surface contracts follow, with generated target/LED integration completed in P9H.
Base: Phase-8 merged main `de60b5afea868c0f663a2c0d70b27787b302c86a`.

## Purpose

Phase 9 makes the existing APSIS instrument playable through the M-VAVE FM-1 panel. It may adapt physical gestures, but it may not create a second musical model or change APSIS timing/state semantics.

This contract supersedes `docs/UI_MAPPING.md` wherever that older file is only a physical proposal. The older document remains useful design rationale.

## Authoritative hardware vocabulary

Pinned Dinghy exposes 27 note keys, 14 debounced buttons, 7 detent encoders, button/key LEDs, a 240x240 display, and one analogue MASTER already owned by platform output gain. There is no encoder-push event in the pinned input API. MASTER therefore remains output level and no APSIS action may depend on an encoder click.

## P9A global navigation lock

HOME=MOTHER, ENV=STRING, LFO=DRIFT, SEQ=CLOCK, ARP=SPIRAL, FX=BLOOM, GLO=GRAVITY, EDIT=CHAIN, SAVE=KEEP, and SEL-held + FX press-edge=SPACE. HOME is absolute. FX alone remains BLOOM. PLAY without SEL toggles the existing shared runtime transport; SEL+PLAY is reserved for CHAIN. P9A target integration remains edge-gated so the frozen P8 idle loop/LED path stays unchanged.

## P9B CLOCK key-grid lock

CLOCK uses the first `n` keys for the authoritative 3..24 steps. ALGORITHM cycles MAIN/BETWEEN/SNAP/GROUP. MAIN/BETWEEN/SNAP toggle their existing masks. GROUP uses positions 1..n-1, keeps at most three unique sorted boundaries, removes an existing boundary when tapped, adds only into a free slot, never silently replaces a fourth boundary, and leaves step zero inert.

## P9B GRAVITY direct-control lock

The lowest eight note keys address degrees 0..7; lowest key wins if several edges arrive in one consumed frame. OCT-/OCT+ cycle Anchor/Colour/Air, with simultaneous edges cancelling. The target adapter must route the selected degree through `apsis_runtime_request_gravity()`; it must never write committed `gravity.degree` directly.

## P9C1 STRING coordinate lock

P9C starts by freezing physical position without inventing a keyboard pitch model. The existing String coordinate remains authoritative:
- Classic DSP reads RIGHT as its one centre String coordinate, so Classic always addresses RIGHT;
- Dual defaults to RIGHT/WIRE;
- holding SEL in Dual addresses LEFT/BODY;
- key 0 represents the 1:1 centre (`split=0.5`) on the selected side;
- key 26 represents that side's outer source limit (RIGHT=0.85, LEFT=0.15);
- intermediate keys are linearly spaced in **split coordinate**, not semitones;
- if several key press edges arrive together, the lowest physical key wins deterministically because one String side is one continuous position, not a polyphonic keyboard;
- SELECT fine-slide uses 128 equal detent increments across one half-string; positive detents always move farther away from the centre, so the tactile direction is consistent on BODY and WIRE;
- ALGORITHM detents toggle Classic/Dual; an even number of detents leaves the mode unchanged.

The host control layer returns side/split intent only. P9H must apply it through `apsis_string_set_side_split(..., mirror=1)`, preserving Dual Link semantics and the legacy `split_q15` normalization. Actual pitch continues through `apsis_string_target_step_milli()`, current Gravity, quantize grid, span, tuning and pitch-memory hysteresis. There is no P9 pitch lookup table.

Manual key pluck must not fabricate a `STRING_RHYTHM` scheduler event. P9H will use the already accepted finite BODY/WIRE/Classic transient primitives directly on the key edge, while M3c remains the only musical scheduler.

K1 resonance, K2 brightness, K3 glide and K4 span remain the second P9C slice. Their physical ranges/step sizes are not guessed in P9C1; they are frozen only against source-derived bounds.

## Reserved / later controls

REC remains contextual. STRING Motion recording/capture is Phase 10, not P9. Selected Gravity-point editing and String/Spiral attractor editors are also Phase 10. SEL remains a momentary modifier, never a hidden mode latch.

## Phase split

- **P9A** — panel vocabulary, direct navigation, SPACE chord, global transport gesture.
- **P9B** — CLOCK four-layer key grid + GRAVITY eight-degree/ring direct controls.
- **P9C1** — STRING side/key coordinate, SELECT fine-slide, Classic/Dual mode.
- **P9C2** — source-bounded STRING K1..K4 live parameters.
- **P9D** — SPIRAL route entry/edit and bank semantics.
- **P9E** — BLOOM petal/core/grow gesture and live parameters.
- **P9F** — KEEP capture/recall + CHAIN editing/control.
- **P9G** — MOTHER mixing/structural sync + SPACE editing + secondary parameter pages.
- **P9H** — generated FM-1 integration, direct transient binding, LED feedback and exact target/emulator regression.

## Deterministic gates

Host tests cover the complete frozen P9A/P9B behavior plus P9C1 side focus, lowest-key resolution, exact String split endpoints, monotonic 27-key coordinate mapping, SELECT direction/clamping, and Classic/Dual mode wrap. ASAN/UBSAN runs the same contract.

## Do not regress

One shared APSIS state graph; one M3c scheduler/runtime; HOME->MOTHER; one shared SPACE/CATHEDRAL; Phase-8 visual order; frozen P6/P7/P8/P9A audible PCM and cursor/queue/late telemetry; journaled persistence outside active audio; no physical-hardware claims until a real FM-1 exists.
