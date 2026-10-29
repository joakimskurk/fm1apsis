# M4B STRING TRANSIENT CONTRACT

This document is the normative source/implementation contract for the next dry String slice after M4a.

M4a is merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`. Its persistent reference audio in `M4A_REFERENCE_AUDIO.md` is frozen and must remain bit-identical whenever no transient is active.

## Authority hierarchy

1. **Study 126 remains behavioural/timing/safety authority.** It owns Classic/Dual performance topology, independent BODY/WIRE sides, String Rhythm/Attractor trigger behaviour, shared transport timing, pulse isolation and all later de-click fixes.
2. **Study 98 is acoustic authority for `classicStringNote()` and the older String transient character where that primitive exists.** Its harmonic stack, amplitude law, duration law, root shadow and pan relationship are the acoustic target.
3. **Study 124/125/126 safety changes override older engine mechanics.** In particular, String onset is safer, audible voices are never hard-stopped during stealing, Clock pulses do not hard-step the global scene gain, and the removed broadband random-noise String exciter stays removed.
4. Native FM-1 realization may replace WebAudio primitives with deterministic fixed-point DSP, but must not invent a private clock or silently change the musical topology.

## M4b staging

### M4b1 — pure finite transient engine

Build and prove the low-level source-derived transient primitive before connecting it to transport events:

- fixed-state / allocation-free transient voice pool;
- Study-98 normal Classic partial stack;
- Study-98 pizz partial stack;
- optional folded Gravity-root shadow;
- deterministic stereo Q15 render mixed with unchanged M4a sustain;
- Study-126 de-click onset and safe voice-steal teardown;
- no `libm`, allocation or target-unavailable 64-bit division;
- host normal + ASAN/UBSAN before live event integration.

M4b1 does **not** yet make `APSIS_EVENT_STRING_RHYTHM` audible.

### M4b2 — Study-126 performance topology on the shared timeline

After M4b1 is proven:

- consume the existing `APSIS_EVENT_STRING_RHYTHM`; do not schedule a second String clock;
- preserve Study-126 Classic composite and Dual side ownership;
- preserve Rhythm BOW/PIZZ/MOTION behaviour and side alternation where the current source defines it;
- preserve source state observation at the event boundary;
- prove M3c queue/hash/capacity semantics remain unchanged.

### Later gate — Attractor excitation

String Attractor is intentionally separate because Study 126 late-binds/re-reads String pitch near the target. Do not fold that into M4b1 simply because the same transient primitive is reused.

## Source-exact `classicStringNote()` acoustic primitive

Study 98:

`classicStringNote(freq, when=null, level=.065, dur=.92, pan=0, opts={})`

Input behaviour:

- base frequency is clamped to 35..2800 Hz;
- resonance is `max(.25, string.resonance)`; native state bounds may further constrain it;
- brightness is `max(.15, string.brightness)`; native state bounds may further constrain it;
- optional `rootGain` defaults to zero.

Normal partial set:

- harmonics `h = 1,2,3,4`;
- roll exponent `1.28 + (1 - brightness) * 0.58`;
- partial amplitude before the generic tone envelope is `level * scale * resonance / h^roll`;
- partial duration is `dur * (1 + hi*0.16)`, where `hi` is 0-based harmonic index;
- start staggering is `hi * 3 ms`;
- harmonics 1 and 2 use triangle; harmonics 3 and 4 use sine.

Pizz partial set:

- harmonics `h = 1,2,3`;
- roll exponent is fixed at `1.52`;
- partial amplitude uses the same `level * scale * resonance / h^roll` law;
- duration is `dur * (1 - hi*0.09)`;
- start staggering remains `hi * 3 ms`;
- harmonics 1 and 2 triangle, harmonic 3 sine.

Pan topology:

- fundamental uses the requested pan;
- every upper harmonic uses `pan * -0.45`;
- centered values remain centered.

## Generic tone behaviour inherited by the primitive

Study 98 `tone()` supplies the oscillator/filter/envelope mechanics:

- oscillator frequency is at least 30 Hz;
- triangle partials use a low-pass with cutoff `clamp(freq * 2.35, 1600, 16000 Hz)`, Q approximately 0.32;
- sine partials bypass that low-pass;
- original Study-98 envelope starts near zero, rises exponentially to target amplitude, then decays exponentially toward zero at the partial duration;
- oscillator lifetime extends about 40 ms beyond the nominal envelope end.

**Safety override from Study 124/126:** current Study 126 uses a **16 ms onset for String** instead of the older 10 ms generic onset. Native M4b uses the 16 ms String onset.

The native fixed-point envelope does not need to reproduce WebAudio's internal exponential implementation bit-for-bit, but its source-derived reference vectors must preserve onset, end time, monotonic decay, bounded output and no discontinuity.

## Gravity-root shadow

When `rootGain > 0`:

1. start from the current committed harmonic Gravity root frequency;
2. while root frequency is greater than `playedFrequency * 1.2`, divide it by two;
3. run the **same partial set and timing** at that folded root using `scale = rootGain`.

Study-98 source values that remain acoustic references:

- manual Classic pluck root shadow: `0.28` when Foundation is enabled;
- Study-98 pizz Rhythm root shadow: `0.18` when Foundation is enabled;
- zero when Foundation is disabled.

Later Study-126 side/composite topology uses smaller side-specific root contributions. Those values belong to M4b2 topology and do not rewrite the low-level Study-98 primitive.

## Study-126 performance topology facts for M4b2

### Manual / direct String transient

Current Study 126:

- manual debounce is 30 ms when no explicit scheduled time is supplied;
- Classic calls one composite transient at the current right/Classic String frequency;
- Dual calls the selected side at that side's current frequency and side level.

### Classic composite

Current Study 126 `stringCompositeTransient()`:

- left leg at `t`, level scale `0.66`;
- right leg at `t + 4 ms`, level scale `0.78`.

This composite topology is behavioural authority even though Study 98 predates it. M4b1 does not implement the full composite until the left/bowed acoustic mapping is explicitly source-resolved.

### Dual side roles

Current Study 126 `stringSideTransient()`:

- left/BODY: longer bowed-role transient, pan -0.30, source duration 1.18 s normally / 0.72 s for Attractor;
- right/WIRE: shorter pizz-role transient, pan +0.30, duration 0.66 s normally / 0.42 s for Attractor;
- right leg uses the Classic/pizz partial primitive;
- Attractor may add a short upper-octave sine on the right when its normalized energy is above 0.52.

The broadband random-noise bow exciter removed in Study 126 is **never** restored.

### String Rhythm event

The native Phase-3 transport already owns Rhythm timing.

When String Rhythm is enabled and its trigger condition hits, transport emits exactly one `APSIS_EVENT_STRING_RHYTHM` at **Clock pulse +26 ms**, carrying the shared pulse/step metadata. M4b must consume this existing event; it must not create a String-private scheduler.

Study-126 topology at that event:

- base level is `.052 * sourceLevel * rhythmLevel`;
- Dual side alternates left/right by Clock step parity;
- Classic currently uses the composite transient and returns regardless of BOW/PIZZ/MOTION mode;
- Dual PIZZ and BOW call the selected side with their source-defined level scalars;
- Dual MOTION uses the current Gravity chord note folded to the source range and the bowed-role path.

## Attractor late observation facts

Current Study 126 does not simply freeze String pitch when Attractor decides to fire:

- bind lead is 65 ms on narrow UI, 45 ms otherwise in the browser source;
- at fire time it re-reads current String frequency and side level;
- Classic routes to the composite transient; Dual routes to selected side(s);
- actual WebAudio start is guarded to at least current time +8 ms.

Native timing policy for Attractor must be specified/tested separately against the shared sample timeline before it becomes audible.

## De-click / bounded-pool safety

Study 124 is normative:

- transient voice stealing must **never hard-stop an audible waveform**;
- current browser logical ceiling is 80 tracked transient voices;
- on stealing, gain is held/cancelled then targeted toward near-zero with a 14 ms time constant;
- oscillator teardown happens 105 ms later (>7 time constants);
- String filter transitions are smoothed rather than hard-stepped;
- Study 125 removes pulse-wide scene-gain hard steps;
- Study 126 keeps broadband String noise disabled.

A fixed native pool cannot naively contain exactly 80 structs and immediately reuse a stolen slot: the browser removes the stolen voice from its tracking list while its 105 ms fade tail continues to exist. Therefore M4b must explicitly model a finite fade-tail reserve or another proven click-free equivalent.

The physical pool capacity is **not guessed in this contract**. M4b1 must measure/stress a proposed capacity and prove deterministic behaviour under legal trigger bursts. If a safety reserve is exhausted, the fallback must remain bounded and must not hard-stop an audible voice.

## Native numeric requirements

- sample rate 44.1 kHz;
- stereo interleaved Q15 output;
- partial start offsets are sample-referenced; 3 ms is converted deterministically to the nearest native sample;
- fixed-point harmonic attenuation must be validated against source-derived floating reference vectors over legal brightness/resonance ranges;
- no dynamic allocation;
- no `libm` on target;
- no non-power-of-two 64-bit division helper on JieLi target;
- all accumulators saturate/bound before writing Q15 output.

## M4b1 acceptance before live event integration

1. M4a sustain hashes remain exactly `b20610c1` (Classic) and `10d23cf4` (Dual) when transient pool is idle.
2. Normal 1..4 and pizz 1..3 harmonic identities, waveform roles and 3 ms staggering match source-derived vectors.
3. Harmonic amplitude ratios track the Study-98 exponent law within a documented tight tolerance across low/default/high brightness.
4. Per-partial durations match normal/pizz source formulas at sample resolution.
5. String onset is 16 ms and no first-sample discontinuity is introduced.
6. Triangle low-pass target follows `clamp(freq*2.35,1600,16000)`; sine partials do not receive that filter.
7. Root-shadow octave folding and scale are correct for at least below/equal/above played-pitch cases.
8. Pan topology matches fundamental vs upper-harmonic rule.
9. Repeated triggers are deterministic and bounded.
10. Pool stress proves click-free stealing/fade behaviour and exposes high-water/drop/steal telemetry.
11. Long render has no DC runaway or overflow.
12. Normal host + ASAN/UBSAN pass before transport/runtime integration.

## Deferred beyond M4b1

- full Study-126 Classic composite left/bowed realization;
- live `APSIS_EVENT_STRING_RHYTHM` consumption;
- Attractor late-bound excitation;
- dynamic String bus filter modulation;
- Cathedral/Space;
- physical-hardware listening judgement and real ISR-cycle measurement.