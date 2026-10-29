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

Gain smoothing is kept internally at fractional precision so low default gains
do not collapse the 25 ms time constant through integer minimum-step rounding.
Pan/width changes use the same 25 ms de-clicking policy because the browser
source defines no separate audible Drift-pan automation constant after the
wiring divergence.

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
- reuse the validated fixed String tuning/frequency conversion for the live
  Gravity root;
- saturation occurs only at the final additive stereo mix boundary.

## Frozen host evidence

`tests/test_drift_dsp.c` proves:

1. amount endpoints reproduce 0.05 Hz and 4.00 Hz base delta;
2. default state produces a symmetric tuned pair;
3. mute gives zero target gain and zero audible separation without hard-resetting
   oscillator state;
4. depth clamps to the source minimum 0.15;
5. Equal / Just / Pythagorean + tuning-strength changes move the carrier using
   current Gravity-root semantics;
6. width 0 collapses the pair exactly to centre and width 1 reaches the
   source-derived ±0.34 pan law with equal-power mapping;
7. 25 ms pitch/gain smoothing is deterministic and click-safe;
8. all frozen M3c/M4a/M4b/M4c tests remain unchanged.

Frozen standalone default Drift render, 2 s at 44.1 kHz:

- peak: **488 Q15**
- FNV hash: **`0aec6bc2`**
- normal and ASAN/UBSAN agree.

## Target / emulator verification

Unlike M4b/M4c, Drift is ON by default. Integrating M4d is therefore expected
to change the default emulator PCM.

Validated implementation head:
`0b2a351b17adfa7556c217f24af8a08c1c708d2c`.

Exact implementation-head gates:

- APSIS core #210: PASS normal + ASAN/UBSAN;
- staged validation #199: PASS source/core, unchanged Dinghy, APSIS overlay,
  real JieLi target, default FM-1 emulator, and the dedicated Drift-DSP bypass
  regression gate.

Default M4d target:

- image: **86,544 / 581,564 B**
- RAM: **44,760 / 98,304 B**
- pool: **30,720 / 344,064 B**
- package bytes: 610,110 B
- package SHA-256:
  **`394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`**
- product: `FM-1_972`.

Relative to M4c, the complete persistent Drift engine costs 980 B app image,
80 B RAM and zero additional pool memory.

Default Drift-on emulator reference:

- frames: 124,928
- peak: **433 Q15**
- RMS L/R: **135 / 135**
- stereo delta: **125**
- mean L/R: **-3 / -3**
- raw PCM SHA-256:
  **`539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`**
- M3c queue: 28/512
- overflow: 0
- late flags: 0
- cursor: 123,648
- late trace: `811c9dc5`.

The changed PCM is therefore the intended audible consequence of enabling the
identity-bearing Drift source, while runtime scheduling telemetry remains
frozen.

### Exact M4c bypass proof

A simple `state.drift.sound=0` is **not** a valid isolation test because the
already-frozen browser architecture also uses Drift state inside String/Spiral
couplings. Staged Gate 5b instead regenerates the target with only the new
`ApsisDriftDsp` layer omitted while leaving all pre-M4d state/runtime behavior
and the same `0.1-dev` identity untouched.

That CI-only bypass produces:

- image: **85,564 B**
- RAM: **44,680 B**
- pool: **30,720 B**
- package SHA-256:
  **`c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`**
- raw PCM SHA-256:
  **`10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`**
- peak 317; RMS 101/100; stereo delta 63; mean -2/-2
- queue 28/512; overflow 0; late flags 0; late trace `811c9dc5`.

Both the package SHA and PCM SHA are exactly the frozen final M4c references.
This proves the M4d target delta is isolated to the new persistent Drift DSP;
the previous String/runtime target remains bit-identical when that layer is
removed.

Physical FM-1 sound and real ISR margin remain unverified until hardware exists.
