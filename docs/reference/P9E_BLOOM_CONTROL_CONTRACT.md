# P9E BLOOM physical-control contract

Date: 2026-10-31
Status: **P9E performance + parameter host contract frozen; P9H target/audio binding remains.**
Base: P9D `86ab76e40c63e414eed49bc2c489776a1066bc1d`; P9E1 `f8f9fdad2cace22be84d29c3b9f4b2ad501b5996`.

## Authority

Study 126 remains behavioural authority. `M4G_BLOOM_CONTRACT.md` remains the native Bloom musical/audio contract. `UI_MAPPING.md` is hardware design rationale, not permission to invent a second Bloom engine.

The source identity that must survive physical translation is:

1. individual petals can be performed independently;
2. the central core can trigger the complete current Bloom;
3. outward core growth is a progressive performance gesture that opens/fires petals sequentially;
4. UNFURL (first-petal-to-last temporal distance) is not harmonic spread;
5. all manual sounding must reuse the same current Bloom harmony/voicing/acoustic primitives as automatic Bloom.

## Petals and core

The first physical note keys map one-for-one to the currently available Bloom voices. The count comes from the existing `apsis_bloom_harmony_from_state()` + `apsis_bloom_voice_count()` path:

- TRIAD -> keys 0..2;
- SEVENTH -> keys 0..3;
- NINTH -> keys 0..4;
- FOLLOW -> current Gravity voice count, clamped by the Bloom engine to 3..5.

A press beyond the current voice count is not turned into a generic synth note.

Physical key **13** (the centre of the FM-1's 27-key note field) is the dedicated CORE gesture. CORE has priority if a petal edge and CORE edge occur in the same input frame. It emits a full-Bloom intent only; P9H binds that intent to the existing full Bloom planner/DSP path.

## Progressive grow

REC owns grow while held:

- REC press -> `grow_begin` and reset transient grow state;
- SELECT motion while REC is held -> signed `grow_detents`;
- REC release -> `grow_end`.

Petal/CORE note edges are suppressed while REC owns the gesture, including the release frame.

The source threshold law is now explicitly frozen from Study 126 `updateBloomGesture()`:

`threshold = n===1 ? 0.22 : 0.14 + (i/(n-1))*0.72`

For normal Bloom voice counts this is:

- 3 voices: 14%, 50%, 86%;
- 4 voices: 14%, 38%, 62%, 86%;
- 5 voices: 14%, 32%, 50%, 68%, 86%.

Each petal fires at most once per outward pass. Moving inward does not un-fire or create reverse note scheduling. Grow progress and fired-mask are transient gesture state and must never be serialized into `ApsisState`, KEEP, CHAIN or project persistence.

FM-1 SELECT sensitivity is a hardware adaptation, not a browser semantic: the host contract currently uses **4% normalized opening per encoder detent**. This value is deterministic for emulator development and may be retuned only from real-hardware feel testing without changing the source threshold law.

## Live parameter intents

`UI_MAPPING.md` is frozen for the main Bloom performance page and the browser parameter panel supplies the exact bounds/steps:

- K1 UNFURL: `0..3.6 s`, source step `0.01 s` -> native `0..3600 ms`, 10 ms/detent;
- K2 HARMONIC SPREAD: `0..1`, source step `0.01`;
- K3 LENGTH: `0.65..6.00 s`, source step `0.01 s` -> native `650..6000 ms`, 10 ms/detent;
- K4 BRIGHTNESS: `0.12..1.00`, source step `0.01`.

The control module emits parameter intents only. These browser controls are written through `applyParam()`, so P9H must send them through the existing MOTHER `->1` hold/pending/apply path when armed rather than mutating `ApsisState` directly.

Secondary Bloom parameters (cycles/rate/Clock point/delay/register/width/attack/voice/tuning) remain for the later secondary-page work; P9E does not hide them behind unrelated gestures.

## P9H binding requirements

- PETAL triggers exactly one current Bloom voice through existing Bloom voicing/tuning/acoustic primitives.
- CORE triggers one complete current Bloom without scheduling a fake automatic Clock/Bloom event.
- GROW triggers newly crossed petals from the frozen threshold law; no private Bloom clock and no M3c timeline mutation.
- Manual performance gestures remain live even when MOTHER `->1` is armed; structural parameter intents obey MOTHER pending semantics.
- Manual triggers may sound while transport is stopped, matching the existing audition model.

## Verification boundary

Host tests cover current voice-count ownership, CORE priority, REC grow ownership, exact 3/4/5-voice threshold crossings, no reverse re-fire, and exact source parameter bounds/steps under normal and ASAN/UBSAN builds. Generated-target binding remains P9H. Physical FM-1 feel and real hardware timing remain unverified.
