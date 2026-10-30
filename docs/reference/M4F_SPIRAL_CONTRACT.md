# M4f Spiral dry-engine contract

Status: **implemented and technically verified; final documentation-head gate pending before PR #12 merge.**

Behavioural/acoustic authority: locked Study 126 source. M3c remains the frozen native authority for Spiral Attractor planning, shared-timeline target samples and late pitch binding.

## 1. Scope

M4f makes the existing Spiral musical events audible. It does **not** redesign Spiral, add another sequencer, change Attractor randomness, alter route semantics, or add Space/Cathedral.

Required invariants:
- one shared Phase-3 transport/event heap;
- existing M3c Spiral occurrence and `APSIS_EVENT_SPIRAL_BIND` timing remain authoritative;
- no private Spiral clock or event queue;
- `ApsisRuntime` remains 21,928 B;
- audio state lives externally in fixed-capacity M4f structures;
- no dynamic allocation and no target `libm`;
- M4a–M4e audio/runtime references must recover exactly when the new Spiral DSP layer is omitted.

## 2. Existing source/native musical contract

Study 126 Spiral defaults:
- route empty by default;
- voice `THREAD`;
- note length `0.82`;
- register 0;
- follow Gravity ON;
- Attractor rate 2x Clock;
- Attractor length 16;
- lock .86;
- density .58;
- seed 911;
- source sound ON;
- source level .78.

The native state already represents THREAD / GLASS / REED / HALO.

M3c already freezes:
- route index;
- trigger position;
- Attractor energy;
- target sample;
- unique token;
- native late-bind sample at target−80 ms.

M3c late binding re-resolves the frozen route index against current Spiral/Gravity/Tuning state, including current register octave and tuning deviation. M4f consumes that result rather than recalculating musical pitch.

## 3. Occurrence duration and amplitude law

For each legal Attractor subdivision:

`subInterval = current swung Clock interval / attractorRate`

For a gated hit with energy `e` in [0.12,1]:

`rawDuration = max(0.11 s, subInterval * (0.48 + 0.36 * e))`

`duration = rawDuration * clamp(noteLength, 0.18, 1.35)`

Base amplitude:

`amp = 0.030 + 0.036 * e`

`spiral.level` is clamped [0,1.25] and multiplied into the prepared amplitude.

Energy, duration/noteLength and source level are planning-time acoustic data.

## 4. Drift-coupled main/shadow topology

At planning time:

`delta = DriftOn ? beatDelta(amount) * max(0.15, depth) * driftLevel : 0`

where:

`beatDelta(amount) = 0.05 + 3.95 * amount^1.65` Hz.

Width freezes as `clamp(drift.width,0,1)`.

When `delta > 0.001 Hz`:

Main leg:
- target = Spiral target;
- frequency = late-bound base frequency − frozen delta/2;
- amplitude = `amp * frozenSpiralLevel`;
- duration = full planned duration;
- pan = `−0.34 * frozenWidth`;
- THREAD input role = triangle.

Shadow leg:
- target = Spiral target + **8 ms**;
- frequency = late-bound base frequency + frozen delta/2;
- amplitude = `amp * 0.58 * frozenSpiralLevel`;
- duration = planned duration × **0.88**;
- pan = `+0.34 * frozenWidth`;
- THREAD input role = sine.

When `delta <= 0.001 Hz`, one centered triangle-role leg uses base frequency, amp×1.08, full duration and pan 0.

The +8 ms shadow is DSP-local and never becomes another shared scheduler event.

## 5. Voice topology

Each leg has one oscillator bank, shared low-pass, exponential amplitude envelope and pan.

### THREAD

Main:
- triangle 1x gain 1.000;
- sine 2x gain .085.

Shadow:
- sine 1x gain 1.000;
- sine 2x gain .055.

LP `clamp(freq*3.1,2500,8500)`, Q .28, attack 10 ms.

### GLASS

- sine 1x .82 detune 0;
- sine 2x .20 detune +2 cents;
- sine 3x .055 detune −3 cents.

LP `clamp(freq*4.2,3800,10500)`, Q .28, attack 6 ms.

### REED

- triangle 1x .82;
- sine 2x .15;
- sine 3x .045 detune +1 cent.

LP `clamp(freq*3.0,2100,6200)`, Q .62, attack 10 ms.

### HALO

- sine 1x .55 detune −5 cents;
- sine 1x .55 detune +5 cents;
- sine 2x .10.

LP `clamp(freq*3.5,2600,7600)`, Q .28, attack 22 ms. Tail is duration×1.12 and outer amplitude is additionally ×.82.

## 6. Envelope semantics

Study 126 envelope:
- gain .0001 at target;
- exponential ramp to `max(.0002,effectiveAmp)` at target+attack;
- exponential ramp to .0001 at target+tail;
- oscillators stop tail+50 ms.

Native M4f uses deterministic fixed-point envelope/filter/oscillator math regression-locked by the reference vectors below.

## 7. Planning-time versus late-bound data

Frozen at occurrence planning:
- whether source is on and therefore receives an audio snapshot;
- energy;
- note duration/noteLength;
- source level;
- Drift audible delta;
- Drift width;
- oscillator topology;
- attack/tail law;
- filter Q;
- main/shadow topology and +8 ms offset.

Late-bound at existing M3c bind:
- route-derived pitch;
- Gravity interpretation;
- register octave;
- tuning mode/strength/deviation.

Source edge behaviour is preserved: oscillator topology/envelope/Q remain planning-time voice data, but the late rebind recalculates cutoff using the **current `spiral.voice` cutoff family** and late-bound frequency. A voice edit between planning and bind can therefore combine old topology/Q with new-family cutoff.

## 8. Native handoff architecture

Implemented pieces:
- `ApsisSpiralAudioPlan`: compact source-derived planning snapshot;
- `ApsisSpiralAudioRuntime`: external 64-slot token/snapshot bridge mirroring existing M3c slot lifecycle;
- `apsis_runtime_m4f`: wrapper around the frozen M4c/M3c coordinator;
- `ApsisSpiralDsp`: fixed 32-leg finite renderer.

The bridge does not draw randomness, advance the Attractor, create another occurrence, change queue order, alter M3c hashes, or calculate targets from BPM.

At the existing `APSIS_EVENT_SPIRAL_BIND`, M4f pairs the M3c late-binding result with the matching snapshot and instantiates finite DSP legs with exact remaining delay to target.

`ApsisRuntime` stays 21,928 B.

## 9. Capacity/stress result

Stress profile covers legal worst acoustic density including 180 BPM, 1/16, max swing, Spiral 4x, noteLength 1.35, HALO, audible Drift, all gates/max energy and source level 1.25.

Measured eight-second result:
- logical hits 384;
- logical high-water **15**;
- leg high-water **30/32**;
- oscillator high-water **90**;
- steals **0**;
- drops **0**;
- deterministic hash **`02be4bc5`**.

Pool capacity is therefore frozen at 32 legs.

## 10. Frozen host references

Normal and ASAN/UBSAN agree:
- THREAD peak/hash **1230 / `aef6c83e`**; 2 legs / 4 oscillators;
- GLASS **1157 / `9efa9bdc`**; 2 legs / 6 oscillators;
- REED **967 / `e67a3c15`**; 2 legs / 6 oscillators;
- HALO **1256 / `e7ed6161`**; 2 legs / 6 oscillators.

Bridge tests lock:
- planning-time data remains frozen after later state edits;
- existing M3c bind supplies late pitch/register/tuning;
- late current-voice cutoff family edge behaviour;
- no `ApsisRuntime` growth;
- no shared random/queue ordering changes.

All earlier M3/M4 host references remain unchanged.

## 11. Default-path callback invariant

The first integrated M4f target passed all functional gates but default emulator telemetry advanced only 123,392 samples rather than frozen 123,648. The source-default Spiral route is empty, so this was an unacceptable inactive-engine callback-budget regression.

Pinned Dinghy boot inspection showed BSS zeroing occurs before audio init/IRQ; BSS size was not the cause. The loss came from performing M4f wrapper/binding clears and empty 32-leg renderer scans every callback even when Spiral could not produce audio.

Accepted target policy:
- while Spiral has never had both sound and a non-empty route, use the exact frozen M4e runtime/audio path and perform no Spiral runtime/renderer work;
- latch Spiral audio active on first real route+sound activation;
- keep it latched afterward so pending binds/tails survive later route/mute edits;
- avoid renderer scanning before the first hit.

This restores default cursor **123,648**. `ci/emulator_smoke.sh` now asserts that exact value for the pinned three-second scenario.

## 12. Target/emulator evidence

Technically final implementation/CI head:
`11569371ea18bc8a5e06f28d09ddef90f9835e29`

Gates:
- core #260 PASS normal + ASAN/UBSAN + M4f planning/bridge/renderer suites;
- staged #249 PASS source/core, unchanged Dinghy, overlay, real JieLi target, default emulator, exact cursor assertion and Spiral-DSP bypass.

Default target:
- image **95,804 / 581,564 B**;
- RAM **58,056 / 98,304 B**;
- pool **30,720 / 344,064 B**;
- RAM headroom 40,248 B;
- package SHA **`77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`**.

Default emulator:
- source-default route is empty, therefore no audible Spiral voice is expected;
- PCM remains exact M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`;
- peak 856, RMS 144/144, stereo delta 125, mean −2/−2;
- queue 28/512, overflow 0, late flags 0;
- cursor **123,648**;
- late trace `811c9dc5`.

The default emulator proves target integration, baseline preservation and callback-budget invariance. Because the source-default route is empty, audible Spiral topology/timing is proven by deterministic host/full-chain tests plus real JieLi compile/link, not by an audible default target scenario. No physical listening claim is made.

### Exact M4e bypass

Staged Gate 5b omits only the M4f Spiral DSP/runtime consumer:
- image **90,684 B**;
- RAM **49,128 B**;
- pool **30,720 B**;
- package **`74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`**;
- PCM **`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`**;
- queue 28/512, overflow 0, late flags 0, cursor 123,648, trace `811c9dc5`.

This is exact frozen M4e target/PCM/runtime.

## 13. Deferred

M4f does not implement:
- Bloom audio;
- shared Space/Cathedral;
- final Mother dry/wet integration beyond frozen state behaviour;
- native Spiral UI/control mapping;
- removal/redesign of historical Drift-to-Spiral coupling;
- physical FM-1 validation;
- real ISR worst-case timing.

Next dry engine after M4f merge: **M4g / Bloom**.
