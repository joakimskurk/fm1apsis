# Phase 9 Interaction Contract — FM-1 Physical Parity

Date: 2026-11-01
Status: frozen through P9H4b target integration.
Base: merged Phase-8 main `de60b5afea868c0f663a2c0d70b27787b302c86a`.
Technical Phase-9 head: `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`.

## Purpose

Phase 9 makes the existing APSIS instrument playable through the pinned FM-1 panel without creating a second musical model, private module clocks or UI-owned DSP state. `docs/UI_MAPPING.md` is design rationale; this file is the implementation contract.

## Global panel / navigation

Pinned FM-1 vocabulary:
- 14 buttons
- 7 detent/relative encoders
- 27 note keys
- MASTER remains platform output gain
- no encoder-push input exists

Canonical navigation:
- HOME = MOTHER
- ENV = STRING
- LFO = DRIFT/TUNING
- SEQ = CLOCK
- ARP = SPIRAL
- FX = BLOOM
- GLO = GRAVITY
- EDIT = CHAIN
- SAVE = KEEP
- SEL-held + FX-edge = SPACE/CATHEDRAL

HOME is absolute. PLAY without SEL toggles the existing shared runtime transport. SEL+PLAY is reserved for CHAIN and cannot accidentally toggle global transport. SEL is a momentary modifier, never a hidden latch.

The target input path is activity/edge gated. Idle operation preserves the frozen P8 main-loop/audio behavior and exact PCM/runtime regression.

Relative encoder deltas are applied to the current parameter value. Because the hardware exposes detents rather than absolute pot positions, there is no absolute pickup crossing and no hidden soft-takeover state is required.

## CLOCK / GRAVITY

CLOCK uses the first `n` keys and MAIN/BETWEEN/SNAP/GROUP edit layers. GROUP holds at most three sorted unique boundaries and never silently replaces a fourth.

GRAVITY uses the first eight keys for degree candidates. OCT-/OCT+ cycles Anchor/Colour/Air ring focus. Degree candidates always go through `apsis_runtime_request_gravity()`; UI code never writes committed degree directly. Existing immediate/quantized and MOTHER-pending semantics therefore remain authoritative.

## STRING — source keyboard on 27 keys

The browser source owns a 9-key String keyboard (`A..L`) plus octave banks 0..3. It is not a MIDI-note bypass: it chooses a musical String magnitude, inverts that magnitude back into the String split coordinate, and then the normal String target/quantize/tuning path remains authoritative.

FM-1 exposes three source banks simultaneously:
- keys 0..8 = source bank `base+0`
- keys 9..17 = source bank `base+1`
- keys 18..26 = source bank `base+2`
- `base=0` exposes 0/1/2
- OCT+ selects `base=1`, exposing 1/2/3
- OCT- returns to `base=0`

The overlap is deliberate. The target magnitude is inverted through the existing `apsis_string_magnitude_milli()` law, including current `span`; the control layer does not own a second pitch formula.

Source target law per 9-key group:
- QUANTIZE off: `octave*12 + 16*(i/8)` semitones
- CHROMATIC: `octave*12 + 12*(i/8)` before normal quantization
- SCALE: selected scale note-by-note using source octave indexing
- CHORD: current Gravity-ring chord intervals using source octave indexing

Classic always addresses RIGHT because Classic DSP reads RIGHT as its one String coordinate. Dual defaults to RIGHT/WIRE; SEL addresses LEFT/BODY using the same magnitude mirrored around 1:1. Lowest physical note edge wins deterministically if several arrive in one frame.

SELECT fine-slide uses the source keyboard nudge quantum `0.0045` split per detent. ALGORITHM toggles Classic/Dual.

K1..K4 source bounds:
- K1 RESONANCE `0.30..0.88`, step `0.01`
- K2 BRIGHTNESS `0.20..0.82`, step `0.01`
- K3 GLIDE `0.018..0.380 s`, step `0.004 s`
- K4 SPAN discrete `0.72 / 1.00 / 1.28 / 1.56`

These edits route through the existing MOTHER-aware apply bridge when `->1` is armed.

### Direct STRING audio

P9C maps keys to Study-126 `keyboardStringPress()`, so the P9-only manual acoustic adapter freezes keyboard pluck level **0.080**.

It reuses the existing finite String transient pool instead of fabricating `STRING_RHYTHM` events:
- Classic = source BODY + WIRE composite at one perceived pitch, including 4-ms WIRE offset
- Dual = selected BODY/LEFT or WIRE/RIGHT side
- 30-ms / 1323-sample retrigger debounce
- main loop publishes only a one-byte side request
- audio callback is sole transient-pool owner

Historical M4b/M4f source files remain frozen; Phase 9 manual audio lives in `apsis_string_manual_audio.*`.

## SPIRAL — raw ordered route editing

Study 126 stores an ordered list of raw point ids `0..35`, max 30. `spiralAllowed(i)` is unconditional; route entry does not pre-filter through Gravity/scale. `derivedSequence()` owns later musical interpretation.

Banks:
- bank 0: keys 0..26 -> points 0..26
- bank 1: keys 0..23 -> points 12..35
- keys 24..26 are inactive in bank 1
- OCT+ selects bank 1; OCT- selects bank 0

Route order and duplicate points are identity-bearing:
- plain key = append and select appended route index
- full source-style append drops the oldest item and places the new point at index 29
- SELECT moves route cursor cyclically
- SEL+key inserts before current cursor; full-route insert is rejected
- K1 moves only the selected raw point id, clamped 0..35
- REC deletes exactly selected route index and preserves successor/final cursor behavior

EDIT is not the delete modifier because it remains global CHAIN navigation. Spiral Attractor editing remains Phase 10.

## BLOOM — petal / core / grow

Performance semantics stay live under MOTHER `->1`.

Physical contract:
- low keys address active petals
- center key 13 is CORE
- REC-held + SELECT detents is the grow gesture
- REC-held note edges are suppressed during grow

Source grow thresholds are frozen exactly:
- one voice: `0.22`
- otherwise `0.14 + i/(n-1)*0.72`

K1..K4 parameter intents:
- K1 UNFURL `0..3600 ms`, 10-ms detent
- K2 HARMONIC SPREAD `0..1`, 0.01 detent
- K3 LENGTH `650..6000 ms`, 10-ms detent
- K4 BRIGHTNESS source-bounded Q15 edit

### Direct BLOOM audio

Manual Bloom is P9-only and reuses the existing M4g binding/DSP engine.

Source distinctions:
- direct petal scale `1.04`
- grow petal scale `0.78 + 0.055*i`
- direct petal snapshots display/pending Gravity immediately, matching `bloomDisplayHarmony()` semantics
- core chooses/advances one voice-leading plan from display harmony
- later core petals keep source UNFURL targets and late-bind Gravity/tuning near their normal native bind lead

A finite manual queue supports four concurrent core snapshots / twenty pending petals. It is not a musical scheduler. Core random draws come from the one shared M3c RNG; no private UI RNG exists.

Main loop publishes one packed 16-bit Bloom request under a short IRQ guard. Audio callback owns the manual queue and `ApsisBloomDsp` mutation.

## KEEP / CHAIN

KEEP uses the existing eight snapshot slots:
- key selects/recalls a filled slot; empty slot selection remains selection
- SAVE captures using source selected/first-empty/final-slot behavior
- contextual REC erases the selected slot and removes CHAIN references through existing native structures

CHAIN uses the existing up-to-32 cells:
- KEEP-slot keys append valid memory slots
- SELECT chooses cell
- PRESETS changes selected cell memory
- K1 cycles, K2 fine pulses, K3 chance, K4 transpose
- ALGORITHM cycles morph values
- REC duplicates selected cell
- EDIT+REC deletes selected cell
- SEL+PLAY starts/stops CHAIN through existing runtime transport

Active-cell deletion preserves the source rule that the new cell occupying the active position is applied immediately through the existing runtime cell-apply path.

All project mutations mark the existing P7 journal dirty; flash save remains coalesced, transport-deferred and main-loop only.

## MOTHER / SPACE / DRIFT-TUNING

MOTHER remains the source mixer/state horizon rather than a second scene system. Live mixer controls remain live; structural edits use the existing one-shot `->1` pending/commit mechanism.

SPACE is the one shared Cathedral, reached by SEL+FX. It edits the existing shared `ApsisSpaceState`; there are never per-source reverbs.

DRIFT/TUNING edits the existing shared Drift and Temper state. Structural Tuning edits pass through the same MOTHER-aware apply bridge.

## Target ownership / safety

- physical decoding occurs in main loop
- existing state/runtime APIs own structural mutations
- no control code runs a private scheduler
- direct String/Bloom DSP mutation occurs only from the audio callback after narrow mailboxes
- project flash erase/program remains unreachable from `app_audio()`
- visuals still read the same shared state and retain the P8 post-8192-sample takeover
- MASTER remains platform output gain

## Phase-9 verification

Host tests and ASAN/UBSAN cover global routing, CLOCK/GRAVITY, 3x9 STRING mapping, source parameter bounds, SPIRAL ordered editing, BLOOM performance/parameter contracts, KEEP/CHAIN, MOTHER, SPACE, DRIFT/TUNING, MOTHER-aware apply, unified dispatch, project-dirty coalescing, direct String audio and direct Bloom audio.

Generated target gates additionally verify pinned BTN/ENC ids, activity-gated input consumption, persistence separation, main/audio ownership and exact idle audio/runtime regression.

Technical target head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e` preserves:
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue 28/512
- overflow 0
- late flags 0
- trace `811c9dc5`
- POOL 309,532 B

See `docs/reference/P9_FINAL_CHECKPOINT.md` for final resource and matrix evidence.

## Deferred to Phase 10

- Clock-quantized String Motion capture
- Classic/Dual Motion ownership and frozen pitch context
- overdub/manual override
- String Attractor editor
- Spiral Attractor editor
- selected Gravity-point editor

## Do not regress

One shared APSIS state graph; one M3c scheduler/runtime; HOME->MOTHER; one shared SPACE/CATHEDRAL; exact frozen idle PCM/runtime; historical M4f 95,804-B bypass; journaled persistence outside active audio; MOTHER pending semantics; no physical-hardware claims until a real FM-1 exists.
