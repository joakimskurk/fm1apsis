# M4f Spiral dry-engine contract

Status: normative source-resolution freeze before DSP implementation.

Behavioral/acoustic authority: locked Study 126 source. M3c remains the frozen native authority for Spiral Attractor planning, shared-timeline target samples and late pitch binding.

## 1. Scope

M4f makes the existing Spiral musical events audible. It does **not** redesign Spiral, add another sequencer, change Attractor randomness, alter route semantics, or add Space/Cathedral.

Required invariants:
- one shared Phase-3 transport/event heap;
- existing M3c Spiral occurrence and `APSIS_EVENT_SPIRAL_BIND` timing remain authoritative;
- no private Spiral clock or event queue;
- `ApsisRuntime` layout remains frozen unless evidence later proves an unavoidable need;
- audio state lives externally in a fixed-capacity M4f structure;
- no dynamic allocation and no target `libm`;
- M4a–M4e audio/runtime references must be recoverable exactly when the new Spiral DSP layer is omitted.

## 2. Existing source/native musical contract

Study 126 Spiral defaults:
- route empty by default;
- voice `THREAD`;
- note length `0.82`;
- register `0` (`LOW/MID/HIGH` = -1/0/+1 octave);
- follow Gravity ON;
- Attractor rate 2x Clock;
- Attractor length 16;
- lock .86;
- density .58;
- seed 911;
- source sound ON;
- source level .78.

The existing native state already represents all four source voice IDs:
- THREAD
- GLASS
- REED
- HALO

M3c already freezes the identity-bearing event data:
- route index;
- trigger position;
- Attractor energy;
- target sample;
- unique token;
- native late-bind sample at target -80 ms.

M3c late binding already re-resolves the frozen route index against current Spiral/Gravity/Tuning state and includes the **current register octave** and current tuning deviation. M4f must consume that result rather than recalculate pitch independently.

## 3. Occurrence duration and amplitude law

For each legal Attractor subdivision:

`subInterval = current swung Clock interval / attractorRate`

For a gated Spiral hit with energy `e` in [0.12, 1]:

`rawDuration = max(0.11 s, subInterval * (0.48 + 0.36 * e))`

Then:

`duration = rawDuration * clamp(noteLength, 0.18, 1.35)`

Base amplitude before source level:

`amp = 0.030 + 0.036 * e`

`spiral.level` is clamped to [0, 1.25] and multiplied into the prepared voice amplitude.

These are **planning-time** values in Study 126. Changing note length, energy-derived duration or Spiral level after a browser voice has been prepared does not rewrite that already-scheduled envelope.

## 4. Drift-coupled two-leg Spiral note

Study 126 deliberately couples Spiral acoustics to the audible Drift separation.

At planning time:

`delta = DriftOn ? beatDelta(amount) * max(0.15, depth) * driftLevel : 0`

where:

`beatDelta(amount) = 0.05 + 3.95 * amount^1.65` Hz.

Width is frozen as `clamp(drift.width, 0, 1)`.

When `delta > 0.001 Hz`, one logical Spiral hit prepares two legs:

### Main leg
- target = Spiral target sample;
- frequency = late-bound base frequency - frozen delta/2;
- amplitude = `amp * frozenSpiralLevel`;
- duration = full planned duration;
- pan = `-0.34 * frozenWidth`;
- THREAD input waveform role = triangle.

### Shadow leg
- target = Spiral target + **8 ms**;
- frequency = late-bound base frequency + frozen delta/2;
- amplitude = `amp * 0.58 * frozenSpiralLevel`;
- duration = planned duration * **0.88**;
- pan = `+0.34 * frozenWidth`;
- THREAD input waveform role = sine.

When `delta <= 0.001 Hz`, there is one centered leg:
- target = Spiral target;
- frequency = late-bound base frequency;
- amplitude = `amp * 1.08 * frozenSpiralLevel`;
- duration = full planned duration;
- pan = 0;
- THREAD input waveform role = triangle.

The Drift delta and width used by an occurrence are planning-time acoustic data. Later Drift edits do not retune an already-prepared browser Spiral note.

## 5. Voice topology

`prepareSpiralTone()` creates a voice-specific oscillator bank, one shared low-pass filter and one exponential amplitude envelope. Every oscillator in a leg feeds the same filter/envelope/pan.

### THREAD

Main leg bank:
- triangle 1x, gain 1.000, detune 0 cents;
- sine 2x, gain 0.085, detune 0.

Shadow leg bank:
- sine 1x, gain 1.000, detune 0;
- sine 2x, gain 0.055, detune 0.

Filter:
- low-pass cutoff `clamp(freq * 3.1, 2500, 8500) Hz`;
- Q .28.

Envelope:
- attack 10 ms;
- tail = planned leg duration.

### GLASS

Both legs use:
- sine 1x, gain .82, detune 0;
- sine 2x, gain .20, detune +2 cents;
- sine 3x, gain .055, detune -3 cents.

Filter:
- low-pass cutoff `clamp(freq * 4.2, 3800, 10500) Hz`;
- Q .28.

Envelope:
- attack 6 ms;
- tail = planned leg duration.

### REED

Both legs use:
- triangle 1x, gain .82, detune 0;
- sine 2x, gain .15, detune 0;
- sine 3x, gain .045, detune +1 cent.

Filter:
- low-pass cutoff `clamp(freq * 3.0, 2100, 6200) Hz`;
- Q .62.

Envelope:
- attack 10 ms;
- tail = planned leg duration.

### HALO

Both legs use:
- sine 1x, gain .55, detune -5 cents;
- sine 1x, gain .55, detune +5 cents;
- sine 2x, gain .10, detune 0.

Filter:
- low-pass cutoff `clamp(freq * 3.5, 2600, 7600) Hz`;
- Q .28.

Envelope:
- attack 22 ms;
- tail = planned leg duration * **1.12**;
- outer leg amplitude is additionally multiplied by **.82** before the oscillator-bank gains.

## 6. Envelope semantics

The browser envelope for every prepared leg is:
- gain `.0001` at leg target;
- exponential ramp to `max(.0002, effectiveAmp)` at target + attack;
- exponential ramp back to `.0001` at target + tail;
- oscillators stop at tail +50 ms.

Native M4f may use fixed-point exponential or sufficiently close deterministic envelope mathematics, but the attack/tail/relative-level relationships above are part of the reference vector and must be regression-tested before optimisation.

## 7. Planning-time versus late-bound data

Study 126 deliberately separates prepared acoustic identity from late musical pitch.

Frozen when the occurrence is planned/prepared:
- whether the Spiral source was on and therefore receives an audio slot;
- Attractor energy;
- note duration and note-length multiplier;
- Spiral source level;
- Drift audible delta;
- Drift width;
- voice oscillator topology;
- attack and tail law;
- filter Q;
- main/shadow topology and +8 ms shadow offset.

Late-bound at the existing M3c bind point:
- route-derived pitch;
- Gravity-follow interpretation;
- register octave;
- tuning mode/strength/deviation.

Study 126 has one unusual edge behavior that M4f should preserve rather than silently normalize: oscillator topology/envelope/Q remain those of the **planning-time voice**, but the late rebind recalculates filter cutoff using the **current `spiral.voice` cutoff family** and the late-bound frequency. Thus a voice change between planning and bind may produce old topology/Q with new-family cutoff.

The frozen Drift ±delta/2 offset is applied around the late-bound base frequency.

## 8. M4f native handoff architecture

M4f should add an external fixed-capacity `ApsisSpiralAudioRuntime` rather than enlarging `ApsisRuntime`.

The M4f runtime wrapper may observe the same scheduling call that creates new M3c Spiral slots and capture a compact acoustic planning snapshot keyed by the existing Spiral token/target. It must not:
- draw randomness;
- advance the Attractor;
- schedule a second occurrence;
- change queue ordering;
- change M3c hashes;
- recalculate a target from BPM.

At the existing `APSIS_EVENT_SPIRAL_BIND`, M4f consumes the M3c late-binding result plus the matching external acoustic snapshot, instantiates fixed finite DSP voices with the remaining exact delay to target, and then lets the already-frozen runtime path continue unchanged.

The +8 ms shadow leg is an audio-relative delay inside the finite voice plan, not a new shared scheduler event.

## 9. Capacity/stress gate

Pool capacity is evidence-driven. Before freezing the M4f pool size, test at least:
- 180 BPM;
- 1/16 Clock division;
- maximum swing;
- Spiral Attractor 4x;
- legal maximum note length 1.35;
- HALO (longest 1.12x tail and three oscillators per leg);
- Drift audible so every logical hit uses both legs;
- all Attractor gates forced on with max energy;
- Spiral level at 1.25.

Record logical-hit high-water, physical oscillator/leg high-water, steals/drops and deterministic PCM hashes. Do not choose a pool ceiling by guesswork.

## 10. Regression gates

Before target integration:
- deterministic THREAD, GLASS, REED and HALO host references;
- main/shadow +8 ms proof;
- duration/energy/noteLength vectors;
- Drift delta/width coupling vectors;
- planning-time voice/level/delta freeze test;
- late Gravity/register/Tuning rebind test using the existing M3c result;
- current-voice late filter-cutoff edge test;
- legal worst-case finite-pool stress;
- normal and ASAN/UBSAN agreement;
- all prior M3/M4 reference tests unchanged.

Target/emulator gate:
- real JieLi build/package;
- default emulator smoke;
- CI-only Spiral DSP bypass that returns to frozen M4e PCM/runtime;
- no physical-hardware or ISR-cycle claim without device evidence.

## 11. Deferred

M4f does not implement:
- shared Space/Cathedral;
- final Mother dry/wet mix integration beyond already-frozen state behavior;
- native Spiral UI/control mapping;
- Bloom audio;
- removal/redesign of the historical Drift-to-Spiral acoustic coupling;
- hardware performance validation.
