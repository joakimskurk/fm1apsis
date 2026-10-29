# M4E CLOCK DRY-ENGINE CONTRACT

Status: normative Phase-4 reference for the native Clock dry engine.

Behavioural/acoustic authority is Study 126 · String Pulse Isolation, source
lock SHA-256
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`,
together with the already-frozen Phase-3 transport contract.

This slice makes the existing Clock event topology audible. It must not create
a second clock, event queue, lookahead scheduler or random stream for musical
events.

## 1. Identity and existing shared timing

The project port contract defines CLOCK as the variable-length 3–24 pulse orbit
with separate main, between and snap layers plus grouping boundaries.

The Phase-3 shared transport already emits the source-equivalent event types:

- `APSIS_EVENT_CLOCK_MAIN` at the pulse target;
- `APSIS_EVENT_CLOCK_SNAP` at pulse +4 ms;
- `APSIS_EVENT_CLOCK_INNER` at half the current swung pulse interval;
- `APSIS_EVENT_CLOCK_GHOST` at the same half-interval target when eligible.

`CLOCK_MAIN` carries `APSIS_EVENT_FLAG_ACCENT` when the source condition is
true: the main step is enabled and the cycle step is step 1 / index 0 or a
configured grouping boundary.

M4e consumes those existing events. BPM, division, swing, cycle length,
step-one resync, masks and Ghost eligibility remain owned by the already-frozen
transport/runtime.

No M4e DSP code may independently derive note-on times from wall clock, BPM or
cycle position.

## 2. Current Study-126 trigger laws

For one browser `tick()` pulse:

```text
mainOn = mainSteps contains cycleStep
accent = mainOn && (cycleStep == 0 || bounds contains cycleStep)

if mainOn:
    perc(accent, pulseTime, ghost=false, levelScale=1)

if Clock sound is on:
    if snapSteps contains cycleStep:
        snap(pulseTime + 4 ms)
    if innerSteps contains cycleStep:
        innerPerc(pulseTime + 0.5 * swungInterval)
    if melodyStep % 3 == 2 && random < ghost:
        perc(false, pulseTime + 0.5 * swungInterval, ghost=true,
             levelScale=0.55)
```

`innerPerc()` is a normal non-accent, non-ghost percussion hit with an
additional `innerLevel` multiplier.

Inner and Ghost may coexist at the same half-interval target and are distinct
voices.

Study 34 explicitly made the outer Clock ring KICK-only: grouping boundaries
change velocity/accent; they do not add a separate low-pitched layer.

Study 125 explicitly removed pulse-stepping of the global dry scene gain.
Clock dynamics live only inside Clock voices. M4e must not modulate String,
Drift or a shared master gain on Clock pulses.

## 3. Clock state required by the dry engine

Study 126 exposes four kick timbres and continuous pitch scaling:

- STONE
- WOOD
- GLASS
- SOFT
- pitch 0.50× .. 2.00× in 0.01 steps

The pre-M4e native state still contains a Phase-2 acoustic placeholder:
`ApsisClockVoice` has only STONE and `clock.pitch` is an `int8_t` value of 1.
That representation is not sufficient for the source contract.

M4e therefore source-corrects the acoustic state without changing KEEP memory
semantics:

```text
voice = STONE | WOOD | GLASS | SOFT
pitch_centi = 50 .. 200       // 0.50× .. 2.00×, default 100
```

`pitch_centi` deliberately matches the source control resolution and fits in
one byte, so the Clock state does not need a floating-point pitch field.

Clock timbre, pitch, level, Ghost amount, between level, Snap level and Snap
tone remain live/global engine state. KEEP continues to store only Clock pulse
geometry (`n`, bounds, main/inner/snap masks), exactly as the existing port
contract specifies.

Current source defaults relevant to M4e:

```text
sound = on
level = 0.78
voice = STONE
pitch = 1.00×
innerLevel = 0.66
snapLevel = 0.62
snapTone = 4200 Hz
ghost = 0
```

Raw `DEFAULTS` has all 12 main steps enabled and inner/snap empty. Starter-scene
seeding is a higher-level scene operation and does not redefine the native
state default.

## 4. Main / Between / Ghost percussion primitive

Source level is clamped to 0..1.25. `levelScale` is non-negative.

Each hit selects one profile. Arrays are `[accent, normal, ghost]`:

```text
STONE
  waveform: sine
  start Hz: [118, 118, 168]
  end Hz:   [ 67,  67, 116]
  duration: 120 ms
  gain:     [0.084, 0.058, 0.018]

WOOD
  waveform: triangle -> low-pass 2600 Hz, Q 0.35
  start Hz: [245, 245, 350]
  end Hz:   [160, 160, 245]
  duration: 75 ms
  gain:     [0.070, 0.050, 0.014]

GLASS
  waveform: sine
  start Hz: [760, 760, 1080]
  end Hz:   [520, 520,  790]
  duration: 280 ms
  gain:     [0.060, 0.043, 0.012]

SOFT
  waveform: sine
  start Hz: [88, 88, 132]
  end Hz:   [58, 58,  92]
  duration: 170 ms
  gain:     [0.056, 0.041, 0.011]
```

For the selected profile/index:

```text
startHz = profile.startHz[index] * pitch
endHz   = profile.endHz[index] * pitch
peakGain = profile.gain[index] * sourceLevel * levelScale
pitchRampDuration = min(90 ms, profile.duration * 0.65)
```

Pitch uses an exponential frequency ramp from start to end.

Gain envelope:

- starts at the source floor `0.0001`;
- linear attack to `max(0.0002, peakGain)` in 2.5 ms;
- exponential decay back to `0.0001` at profile duration;
- oscillator lifetime extends 35 ms beyond profile duration.

Native fixed-point may treat the `0.0001` terminal floor as silence after the
source-equivalent release has completed; it must not hard-discontinue an
otherwise audible envelope.

### Main accent

For STONE, WOOD and SOFT, accent selects the accent gain index but the source
start/end pitch arrays happen to equal the normal pitch values.

GLASS accent additionally creates a second sine:

```text
frequency = accentStartHz * pitch * 1.503
gain      = 0.018 * sourceLevel
attack    = 2.5 ms linear from floor
release   = exponential to floor at 420 ms
lifetime  = 450 ms
```

That second oscillator is part of GLASS accent identity and is required.

### Between

`CLOCK_INNER` uses the normal profile index and:

```text
levelScale = clamp(innerLevel, 0, 1.25)
```

### Ghost

`CLOCK_GHOST` uses the ghost profile index and:

```text
levelScale = 0.55
```

The shared transport already owns whether a Ghost event exists. M4e does not
draw another Ghost random value.

## 5. Snap primitive

Snap is a four-part finite voice at the existing `CLOCK_SNAP` target.

Source state:

```text
toneHz = clamp(snapTone, 1800, 8500)
lvl = sourceLevel * clamp(snapLevel, 0, 1.25)
```

The source UI changes Snap tone in 100-Hz increments. Native M4e may quantise
to that same 100-Hz control resolution.

### Three noise bursts

The browser creates one 160-ms white-noise buffer and reuses it. Each burst
starts from the beginning of that same buffer, then runs:

```text
white noise
 -> high-pass 1100 Hz
 -> band-pass toneHz, Q 1.15
 -> gain envelope
 -> stereo pan
```

Bursts:

```text
start offset   peak gain   decay end   pan
0 ms           0.078       55 ms       -0.18
13 ms          0.055       40 ms       +0.20
27 ms          0.032       28 ms       -0.05
```

Each gain envelope starts at `0.0001`, reaches
`max(0.0002, burstGain * lvl)` after 2 ms, then decays exponentially to the
floor at its listed end time. Browser buffer-source lifetime extends 20 ms
beyond that envelope.

Stereo panning follows equal-power `StereoPannerNode` semantics.

### Pitched digital edge

Snap also creates one square oscillator:

```text
startHz = toneHz * 0.31
endHz   = max(120 Hz, toneHz * 0.12)
pitch   = exponential ramp over 18 ms
peak    = 0.018 * lvl
attack  = 1.8 ms linear from floor
release = exponential to floor at 26 ms
stop    = 30 ms
pan     = centre
```

## 6. Snap noise random disposition

Study 126 fills the 160-ms Snap buffer with the global, unseeded
`Math.random()`. The same global RNG is also used elsewhere for musical
choices (String/Spiral Attractor, Ghost, Bloom and Chain). Therefore enabling
the first Snap can consume thousands of unrelated global random draws and
shift later musical outcomes. The exact resulting sequence is not stable
between browser sessions and is not a reproducible acoustic reference.

Native M4e treats this as random-engine leakage, not as musical Clock timing
semantics:

- Snap uses a Clock-local deterministic noise generator;
- it must not consume the frozen shared M3c/M4c musical RNG;
- every noise burst starts from the same fixed native noise realization,
  matching the browser's reuse of one buffer from offset zero;
- later Snap events reuse that same realization;
- the local noise seed/algorithm is frozen by the M4e audio reference test.

This is an explicit parity disposition. It preserves repeatable native builds,
keeps the already-frozen shared musical random trace unchanged, and preserves
the audible role/correlation of the source Snap noise buffer.

## 7. Event binding and live state

Clock events are already target-sample events on the shared heap. M4e may use a
small passive observer/consumer outside `ApsisRuntime`, analogous to M4b's
String Rhythm handoff, but it must not enlarge or reinterpret the frozen
21,928-byte runtime layout.

At event consumption, M4e reads current acoustic state needed to instantiate
the finite voice:

- Clock sound/mute;
- source level;
- voice;
- pitch;
- inner level for INNER;
- Snap level/tone for SNAP.

The event itself owns target time, type and accent flag. Ghost existence is
already frozen by transport scheduling.

Because browser source buses are independently muteable even after lookahead
has created oscillators, native mute must be able to silence scheduled Clock
voices without a click. M4e must never route Clock dynamics through a shared
scene/master gain.

## 8. Finite voice pool

M4e uses fixed-capacity finite state outside `ApsisRuntime`.

The pool must support the legal worst case without drops:

- 180 BPM;
- 1/16 division;
- maximum swing;
- minimum 3-pulse orbit with every pulse a grouping accent;
- every main, inner and snap step enabled;
- maximum legal Ghost probability;
- GLASS voice (longest primary tail + accent overtone);
- simultaneous Inner + Ghost at the half interval.

Capacity is measured and frozen by stress test before target integration.
Voice stealing is unacceptable for a legal Clock-only worst case; if a later
full-instrument stress requires a bounded steal policy it must be separately
measured and documented.

## 9. Native numerical policy

- 44.1 kHz stereo Q15 render;
- fixed-capacity/allocation-free;
- no target `libm`;
- deterministic oscillator phase and envelopes;
- source exponential pitch/envelope trajectories approximated by deterministic
  fixed-point laws with reference vectors;
- WOOD's 2600-Hz low-pass and Snap's 1100-Hz high-pass + variable band-pass
  must retain their audible filter roles; coefficient tables are allowed and
  preferred over target floating point;
- final saturation only at the additive stereo mix boundary.

M4e must not refactor frozen String or Drift DSP merely to share helpers. Small
Clock-local lookup tables are preferable to risking earlier reference hashes.

## 10. Required host evidence

Before target integration, tests must prove:

1. source-correct voice enum and 0.50×..2.00× pitch representation;
2. all four profile start/end frequencies, durations and gain indices;
3. accent flag selects the correct profile and GLASS extra oscillator;
4. INNER applies `innerLevel` and normal profile index;
5. GHOST applies ghost profile index and fixed ×0.55 level scale without a new
   RNG draw;
6. Snap target topology: three filtered/panned noise bursts + square edge with
   exact offsets/durations/gains;
7. local Snap-noise determinism and zero effect on shared runtime random trace;
8. deterministic reference renders for each kick timbre and Snap;
9. legal worst-case pool occupancy with zero drops;
10. all frozen M3c/M4a/M4b/M4c/M4d tests and references remain unchanged
    except for the intentional source-correction of Clock acoustic state fields.

## 11. Target / emulator gate

Clock is ON by default and the default main mask contains all 12 pulse steps.
Therefore M4e is expected to change default emulator PCM relative to M4d.

The accepted target must prove both:

- default Clock-on output boots/renders deterministically and receives a new
  PCM reference;
- a CI-only build that omits only the new Clock DSP/consumer preserves the
  pre-M4e M4d **audio/runtime behavior**.

Because M4e also corrects the previously-placeholder Clock acoustic state
representation (voice choices and pitch resolution), exact pre-M4e package SHA
identity is not required if the state bytes legitimately change. The bypass
must still preserve the frozen M4d PCM
`539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
and runtime queue/late trace unless source-correct state migration itself is
proven to alter them. Any such alteration must be treated as a regression,
not silently accepted.

Physical FM-1 listening and real ISR margin remain unverified until hardware
exists. Shared Space/Cathedral remains out of scope for M4e.
