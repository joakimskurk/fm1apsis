# M4g Bloom dry-engine contract

Status: **implemented and verified through real JieLi target + emulator on implementation head `6c54d1cd5ca354543237a14c257b3ec28ab424d1`; final documentation-head gate pending.**

Behavioural/acoustic authority: locked Study 126 source. M3c remains the frozen native authority for Bloom occurrence timing, voicing-plan selection, shared-heap petal scheduling and per-petal late pitch binding.

## 1. Scope

M4g makes the already-existing M3c Bloom events audible. It does **not** redesign Bloom harmony, add another sequencer, change voicing randomness, alter Gravity projection, or add Space/Cathedral.

Required invariants:
- one shared Phase-3 transport/event heap;
- existing `APSIS_EVENT_BLOOM_OCCURRENCE` and `APSIS_EVENT_BLOOM_BIND` remain authoritative;
- no private Bloom clock or event queue;
- `ApsisRuntime` remains 21,928 B;
- acoustic state lives externally in fixed-capacity M4g structures;
- no dynamic allocation and no target `libm`;
- M3c Bloom future-aware Gravity semantics remain unchanged;
- M4a–M4f references recover exactly when Bloom DSP/runtime consumption is omitted.

This slice covers transport-driven Bloom voices. Manual Bloom surface gestures remain for the later native interaction/UI phases; they must eventually use the same acoustic primitive.

## 2. Existing source/native musical contract

Study 126 Bloom defaults:
- cycles 2;
- rate 1x;
- trigger step 0;
- delay 60 ms;
- voice `VEIL`;
- depth FOLLOW;
- voicing FOLLOW;
- inversion FOLLOW;
- register 0;
- voicing spread .48;
- length 1.90 s;
- attack 18 ms;
- unfurl 420 ms;
- stereo width .54;
- brightness .72;
- tuning mode FOLLOW;
- tuning reference CHORD ROOT;
- tuning amount 1.0;
- source sound ON;
- source level .58.

Native state already represents VEIL / CHOIR / ORGAN / GLASS and the source defaults above.

M3c already owns:
- 45 ms Bloom occurrence lead;
- future-aware Gravity snapshot for the occurrence target;
- voicing-plan random walk and plan freeze;
- occurrence token and target sample;
- petal target samples across frozen `unfurl`;
- petal bind sample = `max(occurrence, voiceTarget−50 ms)` for the native desktop profile;
- per-petal target-aware Gravity/tuning late binding;
- preservation of every occurrence-frozen petal even if later DEPTH changes.

M4g consumes those results rather than recalculating harmony or pitch.

## 3. Three-stage source causality

Bloom has three distinct source times.

### A. Musical target planning

Phase-3 transport decides the Bloom musical target using Clock/cycles/rate/trigger-step/delay. M4g does not alter it.

### B. Occurrence callback, nominally target−45 ms

When Study 126 `playBloom()` runs, it freezes the occurrence acoustic geometry:
- voicing plan;
- petal count/order implied by that frozen plan;
- `length`;
- `unfurl`;
- `width`;
- Bloom source level;
- per-petal amplitude;
- per-petal pan;
- each petal target time;
- each petal duration.

The source formulas are:

`dur = clamp(length, 0.65, 6.0) s`

`unfurl = clamp(unfurl, 0, 3.6) s`

`width = clamp(width, 0, 1)`

`level = 0.010 * clamp(bloom.level, 0, 1.25)`

For `n` frozen petals, petal `i` uses:

`phase = n>1 ? i/(n−1) : 0`

`target_i = bloomTarget + phase * unfurl`

`duration_i = dur + 0.060 * i`

`amp_i = level * (1 − 0.065 * i)`

`pan_i = n>1 ? ((i/(n−1))*2 − 1) * width : 0`

Those acoustic values stay frozen even if Bloom level/length/unfurl/width are edited before a later petal binds.

### C. Per-petal bind, nominally target_i−50 ms but never before occurrence

At each source `scheduleBloomBoundVoice()` fire, Study 126:
- re-checks Bloom source ON;
- re-resolves the occurrence-frozen plan against target-aware Gravity;
- re-resolves tuning for that target;
- reads the **current** Bloom voice;
- reads the **current** Bloom attack;
- reads the **current** Bloom brightness;
- instantiates the finite voice using the already-frozen amp/pan/duration.

M4g preserves this split. Voice/attack/brightness are late acoustic data, not occurrence data.

## 4. Pitch and tuning authority

M4g does not implement a second Bloom pitch resolver.

The existing `ApsisBloomLateBinding` remains authoritative for:
- target-aware Gravity degree/ring;
- occurrence-frozen plan remapped onto the current base chord;
- current Bloom depth semantics;
- current tuning mode/reference/amount;
- current global Tuning strength where FOLLOW applies;
- final step and tuning deviation.

Study 126 Bloom is intentionally future-aware for ordinary quantized Gravity: `framePending` may be projected when its commit time is within the existing +0.5 ms tolerance of the requested petal target. Mother-held Gravity is not projected. M4g does not change this.

## 5. Voice topologies

Each petal is one finite outer voice with a shared low-pass filter, exponential amplitude envelope and optional stereo pan. All oscillator components in one petal share the same outer envelope/filter/pan.

### VEIL

For petal index 0:
- triangle 1x, gain 1.000.

For petal index >0:
- sine 1x, gain 1.000.

Filter:
- cutoff base = `1500 + brightness*7800`;
- final cutoff = `clamp(cutoffBase + max(30,freq)*0.45, 1000, 12000)` Hz;
- Q = `.30 + .24*brightness`.

Attack = current `bloom.attack` clamped 8…800 ms.
Gain scale = 1.00.
Sustain factor = .66.

### CHOIR

Oscillator bank:
- sine 1x gain .52, detune −6 cents;
- sine 1x gain .52, detune +6 cents;
- sine 2x gain .08, detune 0.

Filter:
- cutoff base = `1100 + brightness*5600`;
- final cutoff uses the same `+freq*0.45`, clamp 1000…12000 Hz;
- Q = `.30 + .24*brightness`.

Attack = `max(45 ms, baseAttack*1.8)`.
Gain scale = .78.
Sustain factor = .72.

### ORGAN

Oscillator bank:
- sine 1x gain .74;
- sine 2x gain .22;
- sine 3x gain .075.

Filter:
- cutoff base = `1500 + brightness*6500`;
- final cutoff same law/clamp;
- Q = .42.

Attack = `max(18 ms, baseAttack)`.
Gain scale = .82.
Sustain factor = .82.

### GLASS

Oscillator bank:
- sine 1x gain .82;
- sine 2x gain .18, detune +2 cents;
- sine 4x gain .035, detune −2 cents.

Filter:
- cutoff base = `2200 + brightness*8800`;
- final cutoff same law/clamp;
- Q = .24.

Attack = `max(6 ms, baseAttack*0.72)`.
Gain scale = .84.
Sustain factor = .66.

## 6. Envelope semantics

For each petal at its exact target sample `t`:
- outer gain starts at .0001;
- exponential ramp to `max(.0002, frozenAmp * currentVoiceGainScale)` at `t + attack`;
- exponential ramp to `max(.00012, frozenAmp * currentVoiceGainScale * sustain)` at
  `t + min(duration*0.48, attack + 0.42 s)`;
- exponential ramp to .0001 at `t + frozenDuration`;
- oscillators stop 60 ms after duration.

Native M4g uses deterministic fixed-point equivalents and regression-locks the resulting audio vectors.

## 7. Occurrence-frozen versus bind-late data

Frozen at occurrence:
- voicing plan;
- petal count/order;
- `length` and per-petal duration;
- `unfurl` and per-petal target;
- `width` and per-petal pan;
- Bloom source level and per-petal amplitude.

Late-bound at existing M3c petal bind:
- Gravity/tuning-derived pitch;
- current Bloom voice topology;
- current attack;
- current brightness;
- current voice-dependent filter/Q/gain-scale/sustain law;
- source ON/OFF eligibility.

This means a Bloom can begin as one acoustic voice family and later petals can use another if `bloom.voice` changes during a long unfurl. The source permits that and M4g does not freeze the voice at occurrence.

## 8. Native handoff architecture

Implemented M4g pieces:
- `ApsisBloomAcousticPlan`: occurrence-time frozen duration/amp/pan/target-offset data;
- `ApsisBloomAudioRuntime`: external snapshot/bind bridge mirroring the existing 64 M3c Bloom slots/tokens;
- M4g runtime wrapper layered above M4f, capturing only existing Bloom occurrence/bind lifecycle data;
- `ApsisBloomDsp`: fixed-capacity finite renderer.

The bridge does not:
- draw randomness;
- choose another voicing plan;
- create another Bloom target;
- calculate BPM/Clock timing;
- change shared queue order;
- modify M3c random/late traces;
- grow `ApsisRuntime`.

At `APSIS_EVENT_BLOOM_OCCURRENCE`, M4g captures only source occurrence acoustic data for the already-created native Bloom slot.

At each existing `APSIS_EVENT_BLOOM_BIND`, M4g pairs that snapshot petal with the already-resolved `ApsisBloomLateBinding` and current voice/attack/brightness, then instantiates finite DSP with sample delay to the original petal target.

## 9. Capacity policy and frozen result

Capacity was measured rather than guessed.

Required stress profile:
- 180 BPM;
- 1/16 division;
- max swing;
- Bloom cycles 1;
- rate 4;
- depth 9th / five petals;
- unfurl 3.6 s;
- length 6 s;
- source level 1.25;
- worst three-oscillator voice family.

Measured normal + ASAN/UBSAN:
- triggers 186;
- petal high-water **125**;
- oscillator high-water **375**;
- drops 0;
- peak 266;
- deterministic hash `5881bb36`.

Frozen capacity: **136 petals**.

This leaves 11 petal slots above measured high-water, enough for more than two complete five-petal occurrences beyond the measured legal peak. Each petal holds at most three oscillators.

`sizeof(ApsisBloomDsp) = 22,864 B` on host.

## 10. Default-path requirement and implementation

M4f proved that even an inactive finite engine can steal one 256-frame callback from the pinned emulator if its wrapper/renderer is scanned unconditionally.

M4g therefore preserves the exact inactive/default path:
- the target uses the exact M4f callback path until an already-planned shared Bloom occurrence is due inside the current callback;
- no Bloom renderer scan occurs before Bloom has real work;
- once activated, M4g stays latched so pending petals/tails survive later mute/state edits.

The pinned three-second default emulator does not reach a due Bloom occurrence, so it remains bit-identical to M4f:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`;
- cursor 123,648;
- queue 28/512;
- late trace `811c9dc5`.

This is intentionally an inactive-path smoke. Audible Bloom is verified at host/full shared-runtime level plus real target compile/link, not by the pinned default listening scenario.

## 11. Bypass requirement and result

Staged validation includes a Bloom-DSP/runtime-consumer bypass that removes only M4g audio consumption while preserving merged M4f.

Verified exact M4f recovery:
- image 95,804 B;
- RAM 58,056 B;
- pool 30,720 B;
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`;
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`;
- queue 28/512;
- cursor 123,648;
- late trace `811c9dc5`.

## 12. Deferred

M4g does not implement:
- shared Space/Cathedral;
- final Mother dry/wet integration beyond frozen state behaviour;
- native Bloom surface/UI/manual gesture mapping;
- physical FM-1 validation;
- real ISR worst-case timing.

After M4g, all five dry source engines are present and Phase 5 sounding-source structural integration can begin.

## 13. Frozen implementation evidence

Technically final implementation/CI head:
- `6c54d1cd5ca354543237a14c257b3ec28ab424d1`

Core #292 PASS, including normal + ASAN/UBSAN for:
- occurrence acoustic planner;
- snapshot/bind bridge;
- shared-runtime handoff;
- finite renderer and legal capacity probe;
- all prior M3/M4 regressions.

Frozen voice vectors:
- VEIL peak/hash 175 / `6177470f`;
- CHOIR 143 / `4b2969f8`;
- ORGAN 122 / `d54a68d9`;
- GLASS 129 / `ca1c3127`.

Staged #281 PASS all gates.

Technically final M4g target:
- image 100,944 / 581,564 B;
- RAM 87,336 / 98,304 B;
- RAM headroom **10,968 B**;
- pool 30,720 / 344,064 B;
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`;
- product `FM-1_972`.

The small remaining static RAM headroom is now a hard constraint for later structural and Space work. No hardware/ISR timing conclusion may be drawn from target compile + emulator alone.
