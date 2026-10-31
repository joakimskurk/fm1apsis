# Phase 9 Interaction Contract — FM-1 Physical Parity

Date: 2026-10-31
Status: P9A global panel/navigation contract frozen; per-surface musical mappings follow in P9B–P9G.
Base: Phase-8 merged main `de60b5afea868c0f663a2c0d70b27787b302c86a`.

## Purpose

Phase 9 makes the existing APSIS instrument playable through the M-VAVE FM-1 panel. It may adapt physical gestures, but it may not create a second musical model or change APSIS timing/state semantics.

This contract supersedes `docs/UI_MAPPING.md` wherever that older file is only a physical proposal. The older document remains useful design rationale.

## Authoritative hardware vocabulary

Pinned Dinghy exposes:
- 27 note keys, F3..G5 as bits 0..26;
- 14 debounced buttons: OCT-, OCT+, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC;
- 7 detent encoders: SELECT, ALGORITHM, K1, K2, K3, K4, PRESETS;
- LEDs for the 14 buttons and 27 note keys;
- one analogue MASTER control already owned by `platform.c` as final output gain;
- no encoder-push event in the pinned Dinghy input API.

Therefore:
- MASTER remains platform output level in Phase 9;
- no primary APSIS action may depend on pressing an encoder;
- encoder rotation is consumed with `fm1_enc_take()` and may report multiple signed detents per main-loop pass;
- button actions use debounced press edges plus current held state;
- note-key performance uses held state and/or note press edges as required by each surface.

## P9A global navigation lock

These direct surface mappings retain the proven Phase-8 shell:

| FM-1 press | APSIS surface |
| --- | --- |
| HOME | MOTHER |
| ENV | STRING |
| LFO | DRIFT / TUNING |
| SEQ | CLOCK |
| ARP | SPIRAL |
| FX | BLOOM |
| GLO | GRAVITY |
| EDIT | CHAIN |
| SAVE | KEEP |
| SEL held + FX press | SPACE / CATHEDRAL |

Rules:
1. HOME is absolute and wins if several navigation edges arrive in the same consumed input frame.
2. SPACE requires an FX **press edge** while SEL is held at input consumption. SEL+FX arriving together therefore qualifies; pressing SEL later while FX is merely held does not.
3. FX without SEL remains BLOOM. SPACE is a shared-room secondary surface, not a sixth source.
4. OCT-/OCT+, SEL, PLAY and REC are not stolen as additional primary surface buttons.
5. Navigation state is UI-only; it is not musical shadow state.

## Global transport lock

- PLAY with no SEL modifier requests one global transport start/stop toggle through the existing shared `ApsisRuntime` lifecycle.
- SEL+PLAY is reserved for the later CHAIN interaction contract and must not accidentally toggle global transport.
- No second transport or UI clock may be introduced.

The target implementation must use the existing runtime start/stop path, including the existing String Attractor lifecycle wrapper where required by the generated target. P9A only freezes the gesture; target integration follows after deterministic routing tests.

## Reserved controls

- REC is contextual. Phase 9 may use it for source-defined performance gestures such as Bloom growth or Chain editing.
- STRING Motion recording/capture remains Phase 10 and is **not** smuggled into P9A.
- OCT-/OCT+ are contextual register/ring/bank navigation. Their exact per-surface behavior is frozen in later P9 subphases.
- SEL is a momentary modifier, not a hidden mode latch.

## Encoder convention

The physical convention is fixed even while exact parameter lists are frozen surface by surface:
- SELECT: current object / continuous primary focus;
- ALGORITHM: mode/sub-surface/page selection;
- K1..K4: visible live parameters;
- PRESETS: memory/sequence/context navigator.

An encoder turning on a surface must mutate only the parameter assigned to that encoder on the currently visible page. Entering a surface or page must never force untouched parameters to the physical/previous encoder position; the encoders are relative detent devices, so there is no absolute pickup problem to solve.

## Phase split

- **P9A** — panel vocabulary, direct navigation, SPACE chord, global transport gesture.
- **P9B** — CLOCK key grid + GRAVITY eight-point direct play.
- **P9C** — STRING key map + continuous encoder slide/basic live parameters.
- **P9D** — SPIRAL route entry/edit and bank semantics.
- **P9E** — BLOOM petal/core/grow gesture and live parameters.
- **P9F** — KEEP capture/recall + CHAIN editing/control.
- **P9G** — MOTHER mixing/structural sync + SPACE editing + secondary parameter pages.
- **P9H** — generated FM-1 integration, LED feedback and exact target/emulator regression.

Advanced String Motion capture, String/Spiral attractor editing and selected Gravity point editing remain Phase 10 as already defined by `docs/PLAN.md`.

## P9A deterministic gate

Host tests must prove:
- the pinned 14-button / 7-encoder / 27-key vocabulary;
- all nine direct Phase-8 surface mappings;
- SEL+FX reaches SPACE while FX alone reaches BLOOM;
- FX-edge/current-SEL-held semantics for the SPACE chord, including simultaneous SEL+FX;
- HOME precedence;
- invalid UI surface recovery to MOTHER;
- PLAY alone produces a global transport toggle action;
- SEL+PLAY does not produce that global action.

Before P9H target integration, the generated target must static-assert the local button/encoder ids against pinned Dinghy's `BTN_*` / `ENC_*` constants.

## Do not regress

- one shared APSIS musical state graph;
- one M3c scheduler/runtime, no UI clock;
- HOME -> MOTHER;
- one shared SPACE/CATHEDRAL, not a source or per-source effect page;
- Phase-8 visual surface order and deterministic hashes;
- native visual boot takeover at/after runtime sample 8192 unless exact PCM is revalidated;
- exact audible P6/P7/P8 PCM and integrated cursor/queue/late telemetry;
- persistence remains journaled/CRC-protected and flash mutation remains outside active audio;
- physical behavior is not called hardware-verified until a real FM-1 exists.
