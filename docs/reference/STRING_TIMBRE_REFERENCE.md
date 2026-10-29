# STRING Timbre Reference

This document defines a deliberately narrow exception to the normal APSIS source-lock rule.

## Authority hierarchy

### 1. Behavioural authority — locked Study 126

The FM-1 port continues to treat the frozen Study 126 browser source as authoritative for:

- String state and defaults
- Classic / Dual performance semantics
- BODY / WIRE coordinates and linking behaviour
- Gravity relationships and quantisation
- Motion Loop / Attractor behaviour
- Clock and transport semantics
- routing, source level and Cathedral send role
- safety / de-click / pulse-isolation fixes
- all cross-module interactions

Locked identity:

- logical source: `apsis.html`
- Study: `126 · String Pulse Isolation`
- CRLF source size: `606747` bytes
- lines: `5742`
- SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

The user-supplied `index.html` on 2026-10-29 is this exact source with LF line endings. See `SOURCE_LOCK.md`.

### 2. Acoustic authority for String only — Study 98

The user explicitly prefers the String sound of the attached older source.

Reference identity:

- attached file: `old_index.html`
- Study: `98 · Performance Architecture`
- raw LF size: `306687` bytes
- raw LF line count: `4335`
- SHA-256: `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Study 98 is **not** a replacement source for the port. It is only the target for the acoustic character of the future native String engine.

## Why the two versions sound materially different

Study 98 CLASSIC String and Study 126 String are not merely different parameter presets.

### Study 98 CLASSIC core

The older source deliberately restored the Study 20 sound DNA. Its persistent String layer contains:

- a low Gravity-root Foundation pair;
- a pair of sine voices placed symmetrically around the played String frequency using the Drift separation;
- relatively dark filtering on the persistent voices;
- the older `classicStringNote()` overtone/pluck transient path.

This produces one coherent, gently beating String identity even though multiple oscillators contribute to it.

### Study 126 redesign

The current locked source expanded String into a dual-instrument performance system. In its later Classic mode, BODY and WIRE are collapsed to one perceived pitch/controller, but the persistent sound itself is layered from different timbres:

- BODY uses the String periodic-wave / body voice;
- WIRE uses a brighter triangle-derived wire voice;
- both are layered at one pitch in Classic;
- Dual restores their independent positions, anchors, levels and Gravity-follow state.

That is an architectural timbre change. It explains why preferring the older String is not reducible to brightness/resonance tweaks.

## Native FM-1 String target

When Phase 4 begins, the native String engine should aim for the old acoustic identity while remaining inside the current APSIS behaviour model.

### Must preserve from Study 98

1. **Foundation floor**
   - persistent low Gravity-root layer;
   - separate from the performed String pitch;
   - restrained level and dark spectral balance.

2. **CLASSIC carrier identity**
   - persistent sine-based pair around the currently played String pitch;
   - pair separation follows the current/native Drift model rather than becoming a fixed chorus;
   - perceived as one String, not two unrelated synth voices.

3. **Transient DNA**
   - overtone/pluck behaviour derived from the older `classicStringNote()` concept;
   - harmonics decay with a musically useful roll-off rather than a bright generic pluck;
   - retain the old sense of a physical/string-like excitation rather than BODY/WIRE contrast as the defining sound.

### Must preserve from Study 126

1. **Current state and performance contract**
   - Dual is the source default;
   - Classic remains one perceived controller/string;
   - Dual retains BODY/WIRE independent positions and optional linking;
   - current anchor and Gravity-follow semantics remain authoritative.

2. **Current transport contract**
   - Clock-derived String events use the shared native scheduler;
   - Motion/Attractor timing remains source-derived;
   - no independent String clock.

3. **Current routing contract**
   - String source bus feeds both dry and one per-source send into the shared room;
   - source level/mute and Mother mixing remain live according to the locked model.

4. **Current safety fixes**
   - no hard-stepped global gain on each Clock pulse;
   - safe transient teardown / no audible oscillator hard-stop;
   - smoothed filter changes;
   - post-String guard/safety role where required by the native implementation;
   - broadband random-noise bow exciter stays removed unless a later measured design proves a safe, necessary equivalent.

## How to interpret Classic and Dual in the native engine

The acoustic reference does not require copying the browser oscillator graph literally.

A practical parity-first interpretation is:

- **Classic:** old Study 98 acoustic core is the primary target — Foundation + sine/drift-pair sustain + classic transient character.
- **Dual:** preserve Study 126's two independently playable String roles and state semantics. Their final native timbres may reuse/derive from the old acoustic core, but must remain perceptually distinguishable enough for the Dual performance concept.

The exact low-cost FM-1 DSP realization may differ from WebAudio as long as listening comparisons preserve the intended acoustic identity and resource use remains safe.

## What must not be imported from Study 98

Do not use the old file to override newer source truth for:

- default surface mode;
- String coordinate/state model;
- Gravity behaviour;
- Motion Loop semantics;
- Attractor semantics;
- Clock timing;
- MOTHER / KEEP / CHAIN behaviour;
- Bloom or Spiral behaviour;
- Cathedral topology;
- de-click / guard / pulse-isolation changes;
- any later bugfix represented by the locked Study 126 source.

## Phase 4 acceptance approach

Do not judge String parity from one oscillator waveform or one frequency response alone. Validate the native String in context.

Minimum comparison set when DSP work begins:

1. sustained String at centre / moderate Drift;
2. sustained String away from centre at several Gravity degrees;
3. Foundation ON/OFF comparison;
4. manual classic pluck at low, middle and high String positions;
5. String Rhythm / Clock excitation with the room send at zero first;
6. Gravity change while String sustains;
7. filter modulation/Attractor without pulse-synchronous clicks;
8. dry versus shared-room send;
9. Classic versus Dual perceptual role;
10. worst-case transient/voice-steal stress with no HF crack or discontinuity.

The primary subjective target for Classic is the user's preferred Study 98 String character; the primary behavioural target remains the locked Study 126 instrument.
