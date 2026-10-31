# Phase 9 Interaction Contract — FM-1 Physical Parity

Date: 2026-10-31
Status: P9A global panel/navigation, P9B CLOCK/GRAVITY, P9C STRING and P9D SPIRAL host contracts frozen; later surfaces follow, with generated target/LED integration in P9H.
Base: Phase-8 merged main `de60b5afea868c0f663a2c0d70b27787b302c86a`.

## Purpose

Phase 9 makes the existing APSIS instrument playable through the FM-1 panel without creating a second musical model or changing APSIS timing/state semantics. `docs/UI_MAPPING.md` remains design rationale; this file is the implementation contract.

## P9A — global panel

HOME=MOTHER, ENV=STRING, LFO=DRIFT, SEQ=CLOCK, ARP=SPIRAL, FX=BLOOM, GLO=GRAVITY, EDIT=CHAIN, SAVE=KEEP, SEL-held+FX-edge=SPACE. HOME is absolute. PLAY without SEL toggles the existing shared runtime transport; SEL+PLAY is reserved for CHAIN. P9A target code is edge-gated so the frozen P8 idle path remains exact.

## P9B — CLOCK / GRAVITY

CLOCK uses first `n` keys and MAIN/BETWEEN/SNAP/GROUP layers. GROUP holds at most three sorted unique boundaries and never silently replaces a fourth. GRAVITY uses the first eight keys for degree candidates and OCT-/OCT+ for Anchor/Colour/Air. Degree candidates must go through `apsis_runtime_request_gravity()`; committed degree is never written directly by UI code.

## P9C — STRING source keyboard on 27 keys

The browser source has a 9-key String keyboard (`A..L`) plus octave banks 0..3. Its keyboard is not a MIDI-note bypass: it chooses a musical String magnitude, converts that magnitude back into String split, then the normal String target/quantize/tuning path remains authoritative.

FM-1 exposes **three 9-key source banks simultaneously**:
- physical keys 0..8 = source keyboard in octave bank `base+0`;
- keys 9..17 = source keyboard in bank `base+1`;
- keys 18..26 = source keyboard in bank `base+2`;
- `base=0` exposes banks 0/1/2;
- OCT+ changes base to 1, exposing banks 1/2/3; OCT- returns to 0.

This deliberately preserves the source bank overlap rather than pretending the 27 keys are a conventional chromatic keyboard.

Source target law per 9-key group is retained:
- QUANTIZE off: `octave*12 + 16*(i/8)` semitones;
- CHROMATIC grid: `octave*12 + 12*(i/8)` before normal chromatic quantization;
- SCALE grid: walk the selected seven-note scale note-by-note using source octave indexing;
- CHORD grid: walk the current Gravity ring chord intervals using source octave indexing.

The target magnitude is inverted through the **existing native `apsis_string_magnitude_milli()` law**, including current `span`, instead of copying a second pitch formula. The result is only a split coordinate; P9H applies it with `apsis_string_set_side_split(..., mirror=1)`.

Classic always addresses RIGHT because Classic DSP reads RIGHT as its single String coordinate. Dual defaults to RIGHT/WIRE; holding SEL addresses LEFT/BODY using the same magnitude mirrored around the 1:1 centre. If several note edges arrive in one input frame, the lowest physical key wins deterministically.

SELECT fine-slide uses the browser keyboard nudge quantum `0.0045` split per detent. Positive detents move farther from 1:1 on either side. ALGORITHM toggles Classic/Dual. Manual key pluck must use existing finite String transient primitives directly in P9H, never fabricate a `STRING_RHYTHM` scheduler event.

## P9C — source-bounded live parameters

The locked browser parameter surface is authoritative:
- K1 RESONANCE: `0.30..0.88`, step `0.01`;
- K2 BRIGHTNESS: `0.20..0.82`, step `0.01`;
- K3 GLIDE: `0.018..0.380 s`, step `0.004 s`;
- K4 SPAN: discrete `0.72 / 1.00 / 1.28 / 1.56`.

The control core emits parameter intents in native state units; it does **not** mutate state. This is required because browser `motherShouldHold()` holds every non-MOTHER structural parameter while `->1` is armed. P9H must therefore route each String edit through the existing native MOTHER hold/pending/apply mechanism or apply it live only when hold is inactive.

## P9D — SPIRAL route geometry and ordered editing

Study 126 stores SPIRAL melody geometry as an **ordered list of raw point ids `0..35`**, max 30. `spiralAllowed(i)` is unconditional, so route entry must not pre-filter points through Gravity/scale. `derivedSequence()` owns the later musical interpretation; the drawn/stored route remains fixed when FOLLOW GRAVITY changes sounding pitch.

FM-1 uses the two-bank design from `UI_MAPPING.md`:
- bank 0: physical keys 0..26 -> Spiral points 0..26;
- bank 1: keys 0..23 -> points 12..35;
- keys 24..26 are intentionally inactive in bank 1;
- OCT+ selects bank 1, OCT- selects bank 0;
- simultaneous key edges resolve to the lowest valid physical key.

The overlap `12..26` is deliberate and preserves hand continuity when changing bank.

Route order and duplicate points are identity-bearing. The physical edit gestures therefore operate on **route indices**, not point membership:
- plain key edge: append the addressed point and select the appended route index;
- source max-30 append behaviour is retained: when already full, the oldest route item is dropped and the new point becomes index 29;
- SELECT encoder: move the route cursor cyclically across existing route indices;
- SEL + key edge: insert the addressed point **before** the current cursor; on an empty route this becomes index 0;
- insert at 30 items is rejected rather than silently evicting/reordering a different node;
- K1: move only the selected route node along raw Spiral point ids, clamped `0..35`, without changing route order;
- REC: delete exactly the selected route index and keep the cursor on the successor where possible, otherwise the new final item.

EDIT is **not** used as the delete modifier because its edge is already the global CHAIN navigation control. P9D does not include the SPIRAL attractor editor; that remains Phase 10. The browser pointer route editor mutates `state.spiral.selected` directly rather than going through `motherQueueParam()`, so P9D preserves that live route-edit identity. Parameter/toggle changes that already use `applyParam()`/`toggleParam()` remain subject to MOTHER `->1` in P9H.

## Reserved / later controls

REC remains contextual. String Motion recording/capture, selected Gravity-point editing and String/Spiral attractor editors remain Phase 10. SEL is a momentary modifier, never a hidden latch. MASTER remains platform output gain.

## Remaining Phase 9

- **P9E** — BLOOM petal/core/grow gesture and live parameters.
- **P9F** — KEEP capture/recall + CHAIN editing/control.
- **P9G** — MOTHER mixing/structural sync + SPACE editing + secondary pages.
- **P9H** — generated FM-1 integration, MOTHER-aware edit application, manual String transient binding, LEDs and exact target/emulator regression.

## Gates

Host tests cover P9A/P9B plus the source 3x9 String bank mapping, octave-window rules, scale/chord/chromatic/unquantized magnitude inversion, BODY/WIRE mirroring, source nudge quantum, Classic/Dual mode, exact K1..K4 source bounds, SPIRAL two-bank point mapping, route-cursor wrapping, duplicate-preserving append/insert/move/delete semantics, full-route append eviction and full-route insert refusal. The same suite runs under ASAN/UBSAN.

## Do not regress

One shared APSIS state graph; one M3c scheduler/runtime; HOME->MOTHER; one shared SPACE/CATHEDRAL; frozen P6/P7/P8/P9A audible PCM and cursor/queue/late telemetry; journaled persistence outside active audio; MOTHER pending semantics; no physical-hardware claims until a real FM-1 exists.
