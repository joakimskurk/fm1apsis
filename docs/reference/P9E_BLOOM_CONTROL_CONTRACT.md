# P9E BLOOM physical-control contract

Date: 2026-10-31
Status: **P9E1 performance-intent contract frozen; P9E2 parameter controls and P9H target/audio binding remain.**
Base: P9D `86ab76e40c63e414eed49bc2c489776a1066bc1d`.

## Authority

Study 126 remains behavioural authority. `M4G_BLOOM_CONTRACT.md` remains the native Bloom musical/audio contract. `UI_MAPPING.md` is hardware design rationale, not permission to invent a second Bloom engine.

The source identity that must survive physical translation is:

1. individual petals can be performed independently;
2. the central core can trigger the complete current Bloom;
3. outward core growth is a progressive performance gesture that opens/fires petals sequentially;
4. UNFURL (first-petal-to-last temporal distance) is not harmonic spread;
5. all manual sounding must reuse the same current Bloom harmony/voicing/acoustic primitives as automatic Bloom.

## P9E1 mapping

### Petals

The first physical note keys map one-for-one to the currently available Bloom voices. The count is obtained from the existing `apsis_bloom_harmony_from_state()` + `apsis_bloom_voice_count()` path, so current DEPTH/FOLLOW semantics remain authoritative:

- TRIAD -> keys 0..2;
- SEVENTH -> keys 0..3;
- NINTH -> keys 0..4;
- FOLLOW -> current Gravity voice count, clamped by the existing Bloom engine to 3..5.

A press beyond the current voice count is not turned into a generic synth note.

### Core

Physical key **13** (the centre of the FM-1's 27-key note field) is the dedicated CORE gesture. CORE has priority if a petal edge and CORE edge occur in the same input frame. It emits a full-Bloom trigger intent only; P9H binds that intent to the existing full Bloom planner/DSP path.

### Progressive grow

REC owns grow while held:

- REC press -> `grow_begin`;
- SELECT encoder motion while REC is held -> signed raw `grow_detents`;
- REC release -> `grow_end`.

Petal/CORE note edges are suppressed while REC owns the gesture, including the release frame, so a grow cannot accidentally emit a second manual Bloom.

**No grow sensitivity or petal threshold table is frozen in P9E1.** The locked-source evidence available to this port establishes sequential threshold behaviour but does not currently provide a trustworthy exact radial threshold formula. The control core therefore forwards raw detents. P9H/P9E2 may convert them to normalized progress only when that conversion is source-proven or, for encoder sensitivity, physically tuned on real FM-1 hardware. This avoids silently turning a hardware-feel choice into musical state.

Grow progress is UI/runtime gesture state. It must never be serialized into `ApsisState`, KEEP, CHAIN or project persistence.

## P9H binding requirements

- PETAL must trigger exactly one current Bloom voice using the existing Bloom voicing/tuning/acoustic path.
- CORE must trigger one complete current Bloom without scheduling a fake automatic Clock/Bloom event.
- GROW must progressively trigger each voice at most once per outward pass according to the eventual source-proven threshold law.
- Moving grow inward must not invent reverse note scheduling unless source evidence explicitly requires it.
- Manual triggers may sound while transport is stopped, matching the established manual-audition model.
- No private Bloom clock and no mutation of the M3c event timeline.

## Deferred to P9E2 / P9H

`UI_MAPPING.md` proposes K1 UNFURL, K2 harmonic spread, K3 length and K4 brightness. The native Bloom contract already knows their legal state ranges, but P9E1 does not guess missing browser slider detent increments. Parameter-intent mapping will be frozen separately and must obey MOTHER `->1` if the corresponding browser edit used the structural `applyParam()` path.

Physical FM-1 feel and real-hardware timing remain unverified.
