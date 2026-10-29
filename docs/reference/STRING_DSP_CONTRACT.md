# STRING DSP CONTRACT

This is the normative source contract for Phase 4 String DSP on FM-1.

## Authority hierarchy

1. **Study 126 is behavioural authority.** State, Classic/Dual interaction, Gravity, String geometry, quantisation, transport, Motion/Attractor, routing and all later de-click/pulse-isolation fixes remain source truth.
2. **Study 98 is the String acoustic authority where that acoustic feature exists in Study 98.** Its persistent Classic/Foundation architecture and `classicStringNote()` transient character are the preferred timbral target.
3. **Features introduced after Study 98 use Study 126 as their acoustic authority.** Study 98 has no `surfaceMode`, independent left/right String coordinates or WIRE voice; therefore Dual BODY/WIRE cannot be copied from Study 98 and must retain the Study-126 acoustic roles.
4. Native DSP may use a different low-cost realization where WebAudio primitives are inappropriate, but it may not change the musical/state contract.

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

## Dual sustain — source-resolved Study 126 strategy

Study 98 predates Dual. It contains no `surfaceMode`, no independent `leftSplit`, and no WIRE voice. Native Dual therefore preserves the later Study-126 dry architecture rather than inventing a fake "old Dual".

Foundation remains the preferred Study-98 Foundation pair above. On top of it, Dual has exactly two persistent side voices:

### BODY / left side

- frequency = current Study-126 left String frequency;
- harmonic oscillator uses the Study-126 `stringWave` spectrum: real partial coefficients `[1, .31, .15, .085, .052, .030, .018]`, normalized;
- gain before source level = `0.020 * leftLevel`;
- pan = `-0.23`;
- low-pass target = 1550 Hz, Q 0.38;
- pitch smoothing follows String glide.

### WIRE / right side

- frequency = current Study-126 right String frequency;
- triangle oscillator;
- gain before source level = `0.0155 * rightLevel`;
- pan = `+0.23`;
- low-pass target = 3600 Hz, Q 0.48;
- pitch smoothing is Study-126's faster WIRE rule: no slower than about 45 ms when the main String glide is longer.

This keeps Dual's two independently pitched voices, side levels, anchors, Gravity-follow state and stereo identity intact. It does **not** change Classic back to the Study-126 BODY/WIRE layer; Classic remains the Study-98 sine/Drift pair above.

The persistent voice ceiling therefore remains four oscillators in either mode:
- two Foundation voices;
- plus two Classic sine voices **or** one BODY + one WIRE.

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

The following remain mandatory even while targeting the older Classic sound:

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
- Study-126 Dual BODY/WIRE pair;
- Study-126 pitch/state as input;
- Study-126 transport gate;
- smoothed pitch and gain;
- fixed 44.1 kHz stereo interleaved Q15 rendering;
- deterministic phase/state reset;
- no dynamic allocation;
- no `libm` dependency;
- no target-unavailable 64-bit division helper.

## M4a test requirements before app_audio integration

1. Source-derived target calculation for default Foundation, Classic and Dual voices.
2. Foundation root uses `scaleAt(frameDegree())` and honours `foundationOctave`.
3. Drift offsets are exactly `±0.55*d` for Foundation and `±0.5*d` for Classic.
4. Dual uses independent left/right native String targets, not a Classic centre-pitch shortcut.
5. Dual BODY/WIRE gain, waveform, filter and pan roles match Study 126.
6. `foundation=false` removes only Foundation voices.
7. `string.sound=false` or source level zero converges to silence without discontinuity.
8. Transport stop converges smoothly to silence; restart ramps back without resetting musical state.
9. String pitch changes glide rather than phase-reset/click.
10. Rendered Q15 samples stay bounded and deterministic.
11. Long render has no DC runaway/filter instability.
12. Host normal + ASAN/UBSAN pass before target integration.

## Deferred from M4a

- Classic pluck/root-shadow transient;
- String Rhythm transient modes;
- Attractor excitation;
- String bus dynamic filter modulation;
- Cathedral/Space send;
- final Mother/master routing integration;
- physical-hardware listening judgement.

The first audible FM-1 integration may begin only after the dry Classic **and** Dual sustain paths pass the above host/sanitizer contract.