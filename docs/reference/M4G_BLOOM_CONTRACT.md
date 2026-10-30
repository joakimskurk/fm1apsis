# M4g Bloom dry-engine contract

Status: **source-resolved before DSP implementation.**

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
- M4a–M4f references must recover exactly when Bloom DSP/runtime consumption is omitted.

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

M4g must preserve this split. Voice/attack/brightness are late acoustic data, not occurrence data.

## 4. Pitch and tuning authority

M4g does not implement a second Bloom pitch resolver.

The existing `ApsisBloomLateBinding` remains authoritative for:
- target-aware Gravity degree/ring;
- occurrence-frozen plan remapped onto the current base chord;
- current Bloom depth semantics;
- current tuning mode/reference/amount;
- current global Tuning strength where FOLLOW applies;
- final step and tuning deviation.

Study 126 Bloom is intentionally future-aware for ordinary quantized Gravity: `framePending` may be projected when its commit time is within the existing +0.5 ms tolerance of the requested petal target. Mother-held Gravity is not projected. M4g must not change this.

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

Attack = `max(6 ms, baseAttack*0.72)`; because source `baseAttack` is already clamped to at least 8 ms, the effective minimum remains 6 ms only for the product branch where it exceeds that bound.
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

This means a Bloom can begin as one acoustic voice family and later petals can use another if `bloom.voice` changes during a long unfurl. The source permits that and M4g must not freeze the voice at occurrence.

## 8. Native handoff architecture

Planned M4g pieces:
- `ApsisBloomAcousticSnapshot`: occurrence-time frozen duration/amp/pan/target data keyed to the existing M3c Bloom token/slot;
- `ApsisBloomAudioRuntime`: external snapshot/bind bridge mirroring the existing 64 M3c Bloom slots;
- M4g runtime wrapper layered above M4f, capturing only existing Bloom occurrence/bind lifecycle data;
- `ApsisBloomDsp`: fixed-capacity finite renderer.

The bridge must not:
- draw randomness;
- choose another voicing plan;
- create another Bloom target;
- calculate BPM/Clock timing;
- change shared queue order;
- modify M3c random/late traces;
- grow `ApsisRuntime`.

At `APSIS_EVENT_BLOOM_OCCURRENCE`, M4g captures only the source occurrence acoustic data for the already-created native Bloom slot.

At each existing `APSIS_EVENT_BLOOM_BIND`, M4g pairs that snapshot petal with the already-resolved `ApsisBloomLateBinding` and current voice/attack/brightness, then instantiates finite DSP with sample delay to the original petal target.

## 9. Capacity policy

Do **not** guess the Bloom DSP pool size.

Required stress profile before capacity is frozen:
- 180 BPM;
- 1/16 division;
- max swing;
- Bloom cycles 1;
- rate 4;
- depth 9th / five petals;
- unfurl 3.6 s;
- length 6 s;
- source level 1.25;
- voice CHOIR/ORGAN/GLASS worst oscillator count;
- concurrent existing String/Clock/Spiral/Drift load remains present for target/RAM validation.

Measure:
- logical petal high-water;
- oscillator high-water;
- steals/drops;
- deterministic output hash;
- target RAM/image delta.

Freeze the smallest capacity with explicit margin only after the legal stress test passes.

## 10. Default-path requirement

M4f already proved that even an inactive finite engine can steal one 256-frame callback from the pinned emulator if its wrapper/renderer is scanned unconditionally.

M4g must preserve the exact inactive/default path where practical:
- no Bloom renderer scan before Bloom has actually produced/queued audible finite state;
- once activated, keep the audio runtime alive long enough for pending petals/tails even if Bloom is later muted;
- the pinned emulator cursor must remain exactly 123,648 unless a deliberately audible default Bloom scenario changes the reference under an explicit gate.

Default Study 126 Bloom is ON but scheduler rules do not trigger immediately at startup (`bloom_cycle > 0` and cycle/rate/trigger constraints). Therefore target integration must distinguish source ON from actual Bloom-audio activation.

## 11. Bypass requirement

Staged validation must gain a Bloom-DSP/runtime-consumer bypass that removes only M4g audio consumption while preserving the merged M4f code/state/runtime.

The bypass must recover the frozen M4f target/audio/runtime baseline exactly wherever binary-layout state has not intentionally changed. Any unavoidable package-only difference must be explained and separately prove PCM/runtime equality.

## 12. Deferred

M4g does not implement:
- shared Space/Cathedral;
- final Mother dry/wet integration beyond frozen state behaviour;
- native Bloom surface/UI/manual gesture mapping;
- physical FM-1 validation;
- real ISR worst-case timing.

After M4g, all five dry source engines are present and Phase 5 sounding-source structural integration can begin.
