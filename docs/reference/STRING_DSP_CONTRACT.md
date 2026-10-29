# STRING DSP CONTRACT

This is the normative source contract for Phase 4 String DSP on FM-1.

## Authority hierarchy

1. **Study 126 is behavioural authority.** State, Classic/Dual interaction, Gravity, String geometry, quantisation, transport, Motion/Attractor, routing and all later de-click/pulse-isolation fixes remain source truth.
2. **Study 98 is the String acoustic authority only.** Its persistent Classic/Foundation architecture and `classicStringNote()` transient character are the timbral target requested for the native port.
3. Native DSP may use a different low-cost realization where WebAudio primitives are inappropriate, but it may not change the musical/state contract.

Locked identities are recorded in `SOURCE_LOCK.md` and `STRING_TIMBRE_REFERENCE.md`.

## Study 98 persistent acoustic core

The old persistent layer is transport-owned. Stop closes it; editing String/Gravity while stopped must not reopen it. Manual finite audition notes are separate.

### Foundation

Foundation follows the harmonic Gravity root `scaleAt(frameDegree())`, **not** a selected Gravity point's per-point offset/octave.

Before source level:

- base frequency = current Gravity root frequency × `2^foundationOctave`;
- default `foundationOctave = -1`;
- audible Drift separation `d` comes from the current Drift model;
- pair frequencies = `root - 0.55*d` and `root + 0.55*d`;
- each voice gain = `0.016 * foundationLevel`;
- default `foundationLevel = 0.52`;
- both voices are sine-based;
- source low-pass target is approximately 1050 Hz, Q 0.28;
- pitch follows quickly (browser target time constant about 12 ms);
- gain changes are smoothed (browser target time constant about 70 ms).

### Classic sustain

Classic is one perceived String built from a symmetric sine pair around the performed String pitch.

Before source level:

- centre frequency = current Study-126 String pitch;
- pair frequencies = `centre - d/2` and `centre + d/2`;
- each voice gain = `0.020`;
- both voices are sine-based;
- source low-pass target is approximately 1800 Hz, Q 0.35;
- pitch glide follows current String glide, with a practical lower bound around 25 ms in the browser engine;
- gain changes are smoothed around 70 ms.

This is the acoustic architecture intentionally preferred over Study 126 Classic's BODY+WIRE-at-one-pitch sustain redesign.

## Study 98 transient DNA for later M4 String slices

`classicStringNote()` is not part of the first M4a sustain implementation, but its contract is frozen now so later transient work cannot drift.

Normal transient:
- partials 1, 2, 3, 4;
- roll-off exponent = `1.28 + (1 - brightness) * 0.58`;
- partial amplitude = `level * resonance / h^roll`;
- partial duration grows with harmonic index: `dur * (1 + hi*0.16)`;
- partial starts are staggered by 3 ms per index;
- first two partials are triangle, higher partials sine.

Pizz transient:
- partials 1, 2, 3;
- roll-off exponent 1.52;
- partial duration = `dur * (1 - hi*0.09)`.

Optional root shadow:
- uses the current Gravity root;
- folds the root downward by octaves until it is no higher than `playedFrequency * 1.2`;
- Study 98 manual pluck uses root gain 0.28 when Foundation is enabled;
- Study 98 pizz String Rhythm uses root gain 0.18.

## Study 126 constraints that override old engine mechanics

The following remain mandatory even while targeting the older sound:

- default surface mode is Dual;
- Classic is one perceived String/controller;
- Dual keeps independent BODY/WIRE positions, anchors, levels and Gravity-follow state;
- String pitch comes from the current native Study-126 String/state model;
- Foundation follows current committed Gravity harmony;
- the shared Phase-3 sample timeline remains authoritative; no String-private clock;
- persistent sustain is transport-gated;
- manual audition can remain finite while transport is stopped;
- no hard gain steps on Clock pulses;
- no hard-stop of an audibly non-zero transient voice;
- filter/pitch/gain changes must be smoothed;
- pulse-isolation semantics remain testable;
- the removed broadband random-noise bow exciter is not restored;
- dry String is implemented before shared Cathedral/Space; Phase 6 owns the room.

## M4a scope — dry persistent String core

M4a deliberately does **not** attempt all String DSP at once.

Implement and prove a pure allocation-free native DSP core with:

- Study-98 Foundation pair;
- Study-98 Classic sine/Drift pair;
- Study-126 pitch/state as input;
- Study-126 transport gate;
- smoothed pitch and gain;
- fixed 44.1 kHz stereo interleaved Q15 rendering;
- deterministic phase/state reset;
- no dynamic allocation;
- no `libm` dependency;
- no target-unavailable 64-bit division helper.

M4a may expose Dual state to the pure core for tests, but live target integration is not accepted until Dual has a defined native dry strategy. Do not silently reinterpret Study 126 Dual just to make M4a produce sound sooner.

## M4a test requirements before app_audio integration

1. Source-derived target calculation for default Foundation and Classic voices.
2. Foundation root uses `scaleAt(frameDegree())` and honours `foundationOctave`.
3. Drift offsets are exactly `±0.55*d` for Foundation and `±0.5*d` for Classic.
4. `foundation=false` removes only Foundation voices.
5. `string.sound=false` or source level zero converges to silence without discontinuity.
6. Transport stop converges smoothly to silence; restart ramps back without resetting musical state.
7. String pitch changes glide rather than phase-reset/click.
8. Rendered Q15 samples stay bounded and deterministic.
9. Long render has no DC runaway/filter instability.
10. Host normal + ASAN/UBSAN pass before target integration.

## Deferred from M4a

- Classic pluck/root-shadow transient;
- String Rhythm transient modes;
- Attractor excitation;
- String bus dynamic filter modulation;
- fully defined Dual timbral realization;
- Cathedral/Space send;
- final source-level/Mother audio routing integration;
- physical-hardware listening judgement.

The first audible FM-1 integration should happen only after the dry core passes the above tests and the Dual strategy is explicitly documented.