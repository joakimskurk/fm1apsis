# M4D DRIFT DRY-ENGINE CONTRACT

Status: normative Phase-4 reference for the native Drift dry engine.

This contract resolves the current Study-126 source together with the already
frozen APSIS port contract. It exists because the browser source contains a
real wiring inconsistency: the identity-bearing Drift pair is defined and can
be auditioned, but is no longer wired as an independent persistent source while
transport runs.

## Authority and disposition

Behavioural source remains Study 126 · String Pulse Isolation, source lock
SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`.

The project-level `docs/PORT_CONTRACT.md` is already explicit that DRIFT is an
identity-bearing source: **the paired beating field around the Gravity root,
not a generic chorus/LFO macro**, and that Drift owns an independent dry bus
and Space send.

Current Study-126 browser facts:

- `beatDelta()` is `0.05 + 3.95 * amount^1.65` Hz.
- `beatPair()` places two frequencies symmetrically around
  `freqForStep(frameRootStep())`.
- audible separation is multiplied by `max(.15, depth)` and by the current
  Drift source level; mute makes the separation zero.
- `probeBeat()` realizes the pair as two sine oscillators.
- probe amplitude per oscillator is `.013 * sourceLevel('beat')`.
- probe pitch and gain use WebAudio `setTargetAtTime(..., .025)` smoothing.
- `freqForStep()` applies current Gravity scale-lock quantisation and current
  Equal / Just / Pythagorean tuning strength.
- the current transport path does **not** instantiate that pair as a persistent
  `beat` source. Instead `audibleBeatDelta()` is also used to spread String
  Foundation and Spiral voices.
- current `DRIFT WIDTH` is still stored as Drift state, but the active audio
  reference is in the Spiral preparation path rather than the `beat` probe.

This missing persistent source is treated as a browser wiring defect rather
than as permission to delete Drift. Native M4d repairs the wiring while keeping
the source-defined musical identity and laws below. This is an explicit parity
disposition, not a silent redesign.

## Native persistent voice

M4d adds exactly two persistent Drift oscillator slots, separate from the
String persistent engine and String finite-transient pool.

For current state:

```text
baseDeltaHz = 0.05 + 3.95 * amount^1.65
audibleDeltaHz = sound ? baseDeltaHz * max(0.15, depth) * level : 0
carrierHz = tuned current Gravity-root frequency
leftHz  = carrierHz - audibleDeltaHz / 2
rightHz = carrierHz + audibleDeltaHz / 2
```

Both oscillators are sine waves. They are continuous while transport is
running and Drift is audible. Stop silences the pair smoothly; restart does not
require a phase reset to establish musical identity.

The `.013` source probe gain is the acoustic level reference:

```text
voiceGain = 0.013 * level
```

Level therefore has the same dual consequence as the source code: it scales
both beat separation and oscillator amplitude.

## Width

Earlier source history labels `beat.width` explicitly **STEREO WIDTH** and uses
one concrete pan law on the Drift-separated pair:

```text
leftPan  = -0.34 * width
rightPan = +0.34 * width
```

Study 126 still exposes the same state as `DRIFT WIDTH`, but the surviving
transport wiring passes it into Spiral instead of the independent Drift pair.
M4d restores the width law to the identity-bearing Drift pair. Native panning
uses equal-power stereo semantics equivalent to WebAudio `StereoPannerNode`.

M4d does **not** remove the already-frozen Spiral behaviour here. Any later
cleanup of the historical Drift→Spiral coupling must be separately scoped and
regression-gated; this slice only makes Drift itself audible.

## Pitch / tuning ownership

Drift follows the live Gravity root continuously. The carrier is recomputed
from current:

- Gravity tonic;
- scale;
- degree;
- scale-lock state;
- Tuning mode;
- Tuning strength.

No Drift-private quantiser, late-binding queue, clock, or pitch memory is
allowed. This is a persistent current-state voice, not an event voice.

The root uses the same 130.81278265-Hz step-0 convention as the source and the
native String pitch engine.

## Smoothing

Source probe pitch and gain each use a 25 ms WebAudio time constant. Native M4d
therefore smooths both oscillator frequency and gain toward their new targets
with a 25 ms source-equivalent time constant.

Pan/width changes must also be de-clicked. They may use the same 25 ms native
smoothing because the browser source never defines a separate audible Drift
pan automation constant after the wiring divergence.

Mute/Stop must never hard-discontinue an audible oscillator.

## Transport and routing

- Drift is a persistent dry source, transport-gated like the other sustained
  Phase-4 source engines.
- Drift has no pulse events and adds nothing to the shared event heap.
- M3c/M4c scheduler/random/late-binding traces must remain unchanged.
- Drift state remains global/live except `amount`, which is already part of
  KEEP/CHAIN memory semantics.
- M4d is dry only. `send_q15` remains state for the later shared Space phase;
  no private reverb is introduced.
- Mother source mixing remains outside this slice unless already provided by
  the target architecture.

## Native numerical policy

- fixed 44.1 kHz stereo Q15 renderer;
- allocation-free;
- no `libm` on target;
- reuse the existing 257-point fixed Drift-delta table;
- tuning/frequency conversion must be deterministic fixed-point and must not
  alter frozen String output when Drift rendering is omitted/muted;
- saturation occurs only at the final additive stereo mix boundary.

## Required host evidence

Before target integration, tests must prove:

1. amount endpoints reproduce 0.05 Hz and 4.00 Hz base delta;
2. default state reproduces the expected symmetric tuned pair;
3. mute gives zero target gain and zero audible separation without destroying
   oscillator state;
4. depth clamps to the source minimum 0.15;
5. Equal / Just / Pythagorean + tuning-strength changes move the carrier using
   current Gravity-root semantics;
6. width 0 collapses the pair to centre and width 1 reaches ±0.34 pan with
   equal-power stereo mapping;
7. 25 ms pitch/gain smoothing is deterministic and click-safe;
8. a deterministic standalone Drift render has a frozen peak/hash;
9. all frozen M3c/M4a/M4b/M4c tests remain unchanged.

## Target / emulator gate

Unlike M4b/M4c, Drift is ON by default. Therefore integrating M4d is expected
to change the default emulator PCM. The old String-only raw PCM hash is a
regression reference for **String-only / Drift-muted** operation, not the new
full-default output.

The accepted M4d target must prove both:

- Drift-muted output retains the frozen prior String path; and
- default Drift-on output boots/renders deterministically and receives a new
  recorded emulator PCM reference.

Physical FM-1 sound and ISR margin remain unverified until hardware exists.
