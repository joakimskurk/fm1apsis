# Phase 9 Interaction Contract — FM-1 Physical Parity

Date: 2026-10-31
Status: P9A global panel/navigation and P9B CLOCK/GRAVITY host contracts frozen; later surface contracts follow in P9C–P9G, with generated target/LED integration completed in P9H.
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

The target implementation uses the existing runtime start/stop path, including the existing String Attractor lifecycle wrapper. P9A target integration is edge-gated so the frozen P8 idle loop and LED path remain unchanged; persistent P9 LED feedback is intentionally deferred to P9H.

## P9B CLOCK key-grid lock

CLOCK uses the first `n` physical note keys, where the authoritative `ApsisClockState.n` remains constrained to 3..24. The three remaining FM-1 keys are not filled with unrelated features.

`ALGORITHM` cycles one UI-only edit layer with wraparound:
1. MAIN
2. BETWEEN
3. SNAP
4. GROUP

The layer byte is not musical state. The musical truth remains `main_mask`, `inner_mask`, `snap_mask` and `bounds[3]` in `ApsisClockState`.

Key rules:
- in MAIN/BETWEEN/SNAP, a note-key **press edge** toggles the corresponding bit for that Clock position;
- all eligible press edges in one consumed frame are applied, so fast multi-key edits are deterministic rather than silently reduced to one key;
- keys at positions `>= n` are inert;
- GROUP uses positions 1..`n-1` as boundaries; position 0 is intentionally inert because no group can begin before Clock step 1;
- GROUP toggles an existing boundary off, otherwise adds a boundary when one of the three source slots is free;
- attempting to add a fourth simultaneous boundary is a no-op rather than silently replacing another boundary;
- stored boundaries are normalized to unique ascending values with unused slots zero-filled.

P9B does **not** yet assign BPM, Clock length, division, swing or voice encoders. Those remain later live-parameter work; this subphase freezes the geometry/key edit first.

## P9B GRAVITY direct-control lock

The lowest eight physical note-key indices map directly to Gravity degrees 0..7. If several degree press edges arrive in the same consumed frame, the lowest physical degree wins deterministically; the adapter must not emit a race of multiple harmony requests.

`OCT-` / `OCT+` cycle the selected Gravity ring with wraparound:
- 0 = Anchor
- 1 = Colour
- 2 = Air

If OCT- and OCT+ edges are consumed together, they cancel and the current ring is retained.

Critically, a physical degree press is only a **candidate**. The generated target must route that candidate through `apsis_runtime_request_gravity()` using the current selected ring, current runtime sample and the established runtime/MOTHER semantics. P9B must never write `state.gravity.degree` directly, because that would bypass immediate-vs-quantized change timing, target-minus-18ms commit timing and MOTHER `->1` ownership.

Selected-point editing (`SEL + degree`), per-point parameter editing and advanced Gravity preview behavior remain later work; they are not smuggled into the P9B direct-degree contract.

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
- **P9B** — CLOCK four-layer key grid + GRAVITY eight-degree/ring direct controls.
- **P9C** — STRING key map + continuous encoder slide/basic live parameters.
- **P9D** — SPIRAL route entry/edit and bank semantics.
- **P9E** — BLOOM petal/core/grow gesture and live parameters.
- **P9F** — KEEP capture/recall + CHAIN editing/control.
- **P9G** — MOTHER mixing/structural sync + SPACE editing + secondary parameter pages.
- **P9H** — generated FM-1 integration, LED feedback and exact target/emulator regression.

Advanced String Motion capture, String/Spiral attractor editing and selected Gravity point editing remain Phase 10 as already defined by `docs/PLAN.md`.

## P9A/P9B deterministic gate

Host tests must prove:
- the pinned 14-button / 7-encoder / 27-key vocabulary;
- all nine direct Phase-8 surface mappings;
- SEL+FX reaches SPACE while FX alone reaches BLOOM;
- FX-edge/current-SEL-held semantics for the SPACE chord, including simultaneous SEL+FX;
- HOME precedence;
- invalid UI surface recovery to MOTHER;
- PLAY alone produces a global transport toggle action;
- SEL+PLAY does not produce that global action;
- CLOCK layer wrap in both directions and invalid-layer recovery;
- MAIN/BETWEEN/SNAP toggle only first-`n` key positions;
- GROUP add/remove/full/no-step-zero behavior and normalized boundary ordering;
- GRAVITY direct-degree selection from only the first eight keys;
- deterministic lowest-degree resolution for simultaneous degree edges;
- OCT-/OCT+ ring wrap and simultaneous-edge cancellation.

Before P9H target integration, the generated target must static-assert the local button/encoder ids against pinned Dinghy's `BTN_*` / `ENC_*` constants. GRAVITY target integration must additionally prove that degree gestures call the existing runtime request path rather than mutating committed degree directly.

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
