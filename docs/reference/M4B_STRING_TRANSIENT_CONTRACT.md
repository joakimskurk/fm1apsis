# M4B STRING TRANSIENT CONTRACT

This document is the normative source/implementation contract for the finite dry String slice after M4a.

M4a is merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`. Its persistent reference audio in `M4A_REFERENCE_AUDIO.md` is frozen and must remain bit-identical whenever no transient is active.

## Authority hierarchy

1. **Study 126 remains behavioural/timing/safety authority.** It owns Classic/Dual performance topology, independent BODY/WIRE sides, String Rhythm/Attractor trigger behaviour, shared transport timing, pulse isolation and all later de-click fixes.
2. **Study 98 is acoustic authority for `classicStringNote()` and the older String transient character where that primitive exists.** Its harmonic stack, amplitude law, duration law, root shadow and pan relationship are the acoustic target.
3. **Study 124/125/126 safety changes override older engine mechanics.** In particular, String onset is safer, audible voices are never hard-stopped during stealing, Clock pulses do not hard-step the global scene gain, and the removed broadband random-noise String exciter stays removed.
4. Native FM-1 realization may replace WebAudio primitives with deterministic fixed-point DSP, but must not invent a private clock or silently change the musical topology.

## M4b staging and disposition

### M4b1 — pure finite transient engine

The low-level source-derived transient primitive was built and proven before live transport integration:

- fixed-state / allocation-free transient voice pool;
- Study-98 normal Classic partial stack;
- Study-98 pizz partial stack;
- optional folded Gravity-root shadow;
- deterministic stereo Q15 render mixed with unchanged M4a sustain;
- Study-126 de-click onset and safe voice-steal teardown;
- no `libm`, allocation or target-unavailable 64-bit division;
- host normal + ASAN/UBSAN before live event integration.

M4b1 is complete. Its frozen reference is peak/hash 1309 / `97142d3e`; the proven pool is 80 logical + 32 fade-tail reserve = 112 physical slots.

### M4b2 — Study-126 performance topology on the shared timeline

After M4b1 was green, M4b2 connected the existing shared event:

- consume `APSIS_EVENT_STRING_RHYTHM`; do not schedule a second String clock;
- preserve Study-126 Classic composite and Dual side ownership;
- preserve Rhythm BOW/PIZZ/MOTION behaviour and side alternation where the current source defines it;
- freeze planned Rhythm mode in event metadata while late-reading current sound/pitch/surface/levels at audio bind;
- prove M3c queue/hash/capacity semantics remain unchanged.

M4b2 is complete. Validated implementation head `0749ac86e6162fab190f343730ebc9b133c13e47` passed core #170 and staged #159.

### Later gate — Attractor excitation

String Attractor remains intentionally separate because Study 126 late-binds/re-reads String pitch near the target. Do not fold that into M4b simply because the same transient primitive is reused.

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

The native fixed-point envelope does not need to reproduce WebAudio's internal exponential implementation bit-for-bit, but its source-derived reference vectors preserve onset, end time, monotonic decay, bounded output and no discontinuity.

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

Manual physical-control integration is not part of M4b; these facts remain source constraints for the later control layer.

### Classic composite

Current Study 126 `stringCompositeTransient()`:

- left leg at `t`, level scale `0.66`;
- right leg at `t + 4 ms`, level scale `0.78`.

This composite topology is behavioural authority even though Study 98 predates it. M4b2 implements it using the source-resolved native BODY role plus the Study-98-derived WIRE/pizz primitive.

### Dual side roles

Current Study 126 `stringSideTransient()`:

- left/BODY: longer bowed-role transient, pan -0.30, source duration 1.18 s normally / 0.72 s for Attractor;
- right/WIRE: shorter pizz-role transient, pan +0.30, duration 0.66 s normally / 0.42 s for Attractor;
- right leg uses the Classic/pizz partial primitive;
- Attractor may add a short upper-octave sine on the right when its normalized energy is above 0.52.

The broadband random-noise bow exciter removed in Study 126 is **never** restored.

Native M4b BODY uses the same seven-harmonic BODY identity already accepted for M4a Dual sustain, with the Study-126 finite envelope/brightness/pan role. Frozen host BODY reference is peak/hash 947 / `c1168570`.

### String Rhythm event

The native Phase-3 transport owns Rhythm timing.

When String Rhythm is enabled and its trigger condition hits, transport emits exactly one `APSIS_EVENT_STRING_RHYTHM` at **Clock pulse +26 ms**, carrying the shared pulse/step metadata. M4b consumes this existing event; it does not create a String-private scheduler.

Study-126 topology at that event:

- base level is `.052 * sourceLevel * rhythmLevel`;
- Dual side alternates left/right by pulse parity;
- Classic currently uses the composite transient and returns regardless of BOW/PIZZ/MOTION mode;
- Dual PIZZ and BOW call the selected side with their source-defined level scalars;
- Dual MOTION uses the current Gravity chord note folded to the source range and the bowed-role path.

Planning/bind observation contract:

- Study 126 passes the Rhythm mode into `scheduleStringRhythmBound()` when the event is planned; native M4b stores that BOW/PIZZ/MOTION value in the event's otherwise-unused `sub_index`;
- at consumption, native M4b re-reads current String sound/level, Classic/Dual surface, String pitch, side levels and brightness;
- MOTION additionally reads the current Gravity chord at bind;
- M3c owns event consumption; a passive observation wrapper publishes the due event without adding state to `ApsisRuntime`.

## M4b callback-boundary timing policy

M4b intentionally preserves the M4a callback boundary rather than introducing the larger Phase-5 sample-splitting architecture.

Target audio order is:

1. render persistent M4a String;
2. render already-active finite M4b voices;
3. advance the shared M3c timeline for the callback while observing a due Rhythm event;
4. bind a consumed Rhythm event into the finite pool for the next callback.

Therefore a new Rhythm voice can be at most one 256-frame block late and can never sound before its target sample.

Deterministic full-chain proof:

- first Rhythm target at 44.1 kHz is sample 1147;
- callback 1024..1279 consumes it only after that callback's audio was rendered;
- first legal audible callback starts at sample 1280;
- delay is 133 samples, less than 256;
- no early audio is possible under this ordering.

Sample-exact structural/event-to-sounding-source application remains Phase 5.

## Attractor late observation facts

Current Study 126 does not simply freeze String pitch when Attractor decides to fire:

- bind lead is 65 ms on narrow UI, 45 ms otherwise in the browser source;
- at fire time it re-reads current String frequency and side level;
- Classic routes to the composite transient; Dual routes to selected side(s);
- actual WebAudio start is guarded to at least current time +8 ms;
- right Attractor may add the short upper-octave accent when normalized energy exceeds 0.52.

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

A fixed native pool cannot naively contain exactly 80 structs and immediately reuse a stolen slot: the browser removes the stolen voice from its tracking list while its 105 ms fade tail continues to exist.

M4b resolves this with 80 logical slots plus 32 explicit physical fade-tail reserve slots, for 112 physical slots total. The legal 30 ms burst test reaches logical 80 / physical 112 with 480 steals and 0 drops. Realistic Classic-composite stress reaches 80 / 108 with 410 steals and 0 drops. Exhaustion remains bounded and never authorizes a hard stop of an audible voice.

## Native numeric requirements

- sample rate 44.1 kHz;
- stereo interleaved Q15 output;
- partial start offsets are sample-referenced; 3 ms is converted deterministically to the nearest native sample;
- fixed-point harmonic attenuation is validated against source-derived floating reference vectors over legal brightness/resonance ranges;
- no dynamic allocation;
- no `libm` on target;
- no non-power-of-two 64-bit division helper on JieLi target;
- all accumulators saturate/bound before writing Q15 output.

## M4b1 acceptance — completed

1. PASS — M4a sustain hashes remain exactly `b20610c1` (Classic) and `10d23cf4` (Dual) when transient pool is idle.
2. PASS — normal 1..4 and pizz 1..3 harmonic identities, waveform roles and 3 ms staggering match source-derived vectors.
3. PASS — harmonic amplitude ratios track the Study-98 exponent law within the documented tight interpolation tolerance.
4. PASS — per-partial durations match normal/pizz source formulas at sample resolution.
5. PASS — String onset is 16 ms and no first-sample discontinuity is introduced.
6. PASS — triangle low-pass target follows `clamp(freq*2.35,1600,16000)`; sine partials bypass it.
7. PASS — root-shadow octave folding and scale are covered across pitch relationships.
8. PASS — pan topology matches fundamental vs upper-harmonic rule.
9. PASS — repeated triggers are deterministic and bounded.
10. PASS — pool stress proves bounded click-safe stealing and exposes high-water/drop/steal telemetry.
11. PASS — long render has no DC runaway or overflow.
12. PASS — normal host + ASAN/UBSAN passed before transport/runtime integration.

## M4b final verification disposition

Validated implementation/test head: `0749ac86e6162fab190f343730ebc9b133c13e47`.

- core #170: PASS including ASAN/UBSAN and full transport -> observer -> trigger -> render handoff;
- staged #159: PASS through real JieLi target and FM-1 emulator;
- target image 81,752 B / 581,564 B;
- target RAM 42,648 B / 98,304 B;
- target pool 30,720 B / 344,064 B;
- package SHA `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`;
- default Rhythm OFF keeps emulator PCM bit-identical to M4a: `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`;
- M3c live telemetry remains queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`.

## Deferred beyond M4b

- String Attractor late-bound excitation and upper-octave accent;
- dynamic String bus filter modulation;
- manual physical-control trigger integration;
- Cathedral/Space;
- physical-hardware listening judgement and real ISR-cycle measurement.
