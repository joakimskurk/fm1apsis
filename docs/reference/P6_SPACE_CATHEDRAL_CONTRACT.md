# Phase 6 — Space / Cathedral contract

Status: **normative source/topology freeze before native room DSP selection**

Authority:
- Study 126 is behavioural/state/timing/safety authority.
- M4g dry engines and the merged Phase-5 sounding semantics remain frozen.
- Dinghy remains pinned at `d44df7db09b98ccad2f1233b8f83f5f09c04829e`.

Phase 6 may replace the browser's literal convolution implementation with a compact algorithmic room, because a multi-second Float32 stereo IR is not a practical FM-1 implementation. It may **not** replace the musical topology, parameter roles or source-send relationships with a generic reverb preset.

## 1. Source defaults

Study-126 shared Space defaults:

- Cathedral return: `0.48`
- decay: `8.4 s`
- predelay: `72 ms`
- damping: `6200 Hz`
- echo time: `362 ms`
- echo feedback: `0.18`

Per-source sends:

- String: `0.72`
- Drift: `0.20`
- Clock: `0.06`
- Spiral: `0.62`
- Bloom: `0.58`

These already match `ApsisState` and require no Phase-6 state migration.

## 2. One shared room, five audible sends

The browser explicitly creates one dry bus and one send per instrument and routes every send into **one shared room input**. Native APSIS must preserve that architecture.

The five audible source sends are:

1. String
2. Drift
3. Clock
4. Spiral
5. Bloom

Tuning/Gravity do not become separate room engines. No source receives its own private reverb.

Source output reaches the send after its source bus / Mother mix gain. Therefore a source mute or Mother mute must remove both its dry contribution and its room excitation; the room tail already in flight may continue naturally.

## 3. Browser topology to preserve

Conceptually, Study 126 is:

```text
five source buses
   |\
   | +--> dry scene --> dry gain 0.78 -------------------------------+
   |                                                               |
   +--> per-source send --> shared room input                       |
                              |                                     |
                              v                                     |
                         trim 0.72                                  |
                              |                                     |
                         room guard                                 |
                      HP -> LP -> compressor                        |
                       /        |            \                      |
                      /         |             \                     |
             early reflections   |        feedback echo             |
                      |          |             |                     |
                      |          +----> predelay                     |
                      |                    |                          |
                      |                  HP 190                       |
                      |                    |                          |
                      |            long Cathedral tail <-------------+
                      |                    |
                      |              damping LP
                      |                    |
                      |              wet return
                      |                    |
                      +------------------> master <-------------------+
```

The feedback echo is not a separate dry echo effect. Its delayed output feeds the same tail high-pass / Cathedral path.

## 4. Wet-input safety / room guard

All wet excitation passes through the source-defined room guard before early reflections, echo or long tail:

- trim gain `0.72`
- HP: `170 Hz`, Q `0.35`
- LP initial: `5200 Hz`, Q `0.22`
- compressor: threshold `-22 dB`, knee `16 dB`, ratio `3.2:1`, attack `1.5 ms`, release `110 ms`

Native implementation does not have to reproduce WebAudio's compressor implementation bit-for-bit, but it must preserve the **safety role**: broadband discontinuity energy must be bounded before entering a long feedback/diffuse tail.

The room guard is part of the final Phase-6 topology, not an optional mastering effect.

## 5. Early reflections

Study 126 uses four early delay taps after the room guard:

| delay | tap gain |
|---:|---:|
| 21 ms | 0.100 |
| 43 ms | 0.075 |
| 71 ms | 0.052 |
| 109 ms | 0.036 |

They sum into an early bus with gain `0.16`, then directly to master.

The native room must preserve this sparse early-reflection role. A prototype may share storage with another room delay structure if the audible timings/gains remain equivalent.

## 6. Long Cathedral tail — perceptual source target

The browser constructs a stereo synthetic convolution IR whose length is `sampleRate * decay`, with decay clamped to `4..12 s`.

Per channel it generates white noise and smooths it through three one-pole stages:

```text
lp  = 0.78 * lp  + 0.22 * white
lp2 = 0.68 * lp2 + 0.32 * lp
lp3 = 0.58 * lp3 + 0.42 * lp2
```

Then:

```text
env     = exp(-5.15 * t / decay)
late    = 1 - exp(-18 * t)
shimmer = 0.91 + 0.09 * sin(i * channel_factor * 0.0019)
ir      = lp3 * env * late * shimmer * 0.54
```

Channel factors are approximately `0.993` and `1.007`.

The IR also receives five direct taps:

| time | gain |
|---:|---:|
| 31 ms | 0.62 |
| 54 ms | 0.44 |
| 83 ms | 0.34 |
| 127 ms | 0.23 |
| 181 ms | 0.16 |

The right-channel tap gains use the small source-defined channel scaling `0.92 + 0.01*k`.

### Native interpretation

These formulas define the **perceptual target**, not a requirement to store or convolve the literal IR.

The selected FM-1 algorithm must reproduce the important identity:

- slow dense build rather than an immediate comb;
- smooth, dark diffuse tail;
- low metallic periodicity;
- slight stereo decorrelation / slow movement;
- 4..12 s perceived decay control;
- stable response to sharp Clock/String transients;
- no broadband crack amplification.

## 7. Predelay and tail filters

Source predelay range: `15..140 ms`, default `72 ms`.

Long-tail pre-filter:
- high-pass `190 Hz`, Q `0.45`.

Tail damping:
- default `6200 Hz`;
- tail LP Q `0.25`.

Study-126 UI advertises damping `2200..11000 Hz`, but the live `updateSpace()` path clamps the actual tail LP to `2200..6800 Hz` and derives room-guard LP as `clamp(damping * 0.84, 3800, 5600)`.

### Native disposition of this source inconsistency

Native Phase 6 uses the **stable live-audio semantics**:
- effective tail damping `2200..6800 Hz`;
- effective room-guard LP `3800..5600 Hz` from `damping * 0.84`.

The wider `2200..11000` value remains a documented source/UI range for later UI mapping. Native audio must not depend on whether the control has been touched since boot.

## 8. Cathedral return

Wet return range: `0..0.72`, default `0.48`.

Browser automation time constant is approximately `50 ms`. Native parameter smoothing should be click-safe and of the same order; exact WebAudio automation internals are not identity-bearing.

The wet return remains separate from the dry path.

## 9. Feedback echo

Study-126 source state/default:
- echo time default `362 ms`
- source/UI range `80..1200 ms`
- feedback default `0.18`
- source/UI/storage range `0..0.58`
- echo LP fixed `4200 Hz`, Q `0.22`

The live `updateSpace()` path clamps feedback to `0..0.48`, while initial construction may accept up to `0.58`.

### Native disposition

Native Phase 6 uses the stable live-audio range `0..0.48` for the feedback coefficient. The `0..0.58` browser control/storage range is retained as a documented source inconsistency for later UI compatibility.

Echo output feeds the long Cathedral path after its feedback delay; it is deliberately quiet and diffuse rather than an independent foreground delay effect.

## 10. Parameter-update behaviour

Study 126 smooths live room parameters rather than stepping them:

- wet mix: ~50 ms
- predelay: ~50 ms
- tail damping: ~80 ms
- room-guard LP: ~100 ms
- room trim: ~80 ms
- echo delay: ~40 ms
- echo feedback: ~40 ms
- echo LP: ~80 ms

Changing decay rebuilds the browser convolution IR after a debounced delay (default `180 ms`, minimum `80 ms`).

Native algorithmic decay does not need an IR rebuild, but its decay coefficient must update click-safely and must not truncate the existing room tail.

## 11. FM-1 memory placement is part of the contract

Current frozen post-M4g / post-Phase-5 target:

- `.data + .bss`: `87,336 / 98,304 B`
- ordinary static-RAM headroom: **10,968 B**
- Dinghy `.pool`: `30,720 / 344,064 B`
- pool headroom: **313,344 B**

Pinned Dinghy explicitly provides `.pool` for large zeroed buffers, e.g.:

```c
static int16_t delay[65536] __attribute__((section(".pool")));
```

Therefore:

- persistent room **control/filter state** should remain small ordinary RAM;
- long delay memories belong in `.pool`;
- Phase 6 must not spend the remaining ordinary RAM on large delay arrays.

Reference stereo int16 storage at 44.1 kHz:

- maximum 140 ms predelay: ~24.7 kB
- default 72 ms predelay: ~12.7 kB
- maximum 1.2 s echo delay: ~211.7 kB
- default 362 ms echo delay: ~63.9 kB

The worst-case source ranges can therefore fit only if room storage is designed deliberately around the pool budget.

## 12. Prototype candidates

Before production integration, implement and measure **both** candidates on host:

### Candidate A — compact modulated FDN

Target qualities:
- dense late field;
- long decay from feedback rather than literal multi-second IR storage;
- slight deterministic modulation / stereo decorrelation;
- low metallic periodicity;
- controllable damping in feedback path.

### Candidate B — nested allpass / Schroeder hybrid

Target qualities:
- multiple decorrelated comb/delay paths;
- nested diffusion before/after feedback sections;
- sufficient density for 4..12 s Cathedral tails;
- no obvious short repeating pattern.

Both candidates must use deterministic coefficients/noise/modulation for repeatable host comparison.

## 13. Prototype comparison gates

The comparison must report, at minimum:

- delay-memory bytes in `.pool`;
- ordinary state bytes;
- total multiply/add workload per stereo frame or measured host proxy;
- peak/RMS impulse response;
- early-energy timing;
- decay-energy slope / approximate RT60 at 4 s, 8.4 s and 12 s settings;
- left/right decorrelation;
- repeat/metallicity proxy;
- high-frequency damping response at low/default/high damping;
- transient burst stability;
- deterministic render hash.

No candidate may be selected solely because it is smaller.

## 14. Target-memory gate before audible integration

Before a room is called from `app_audio()`:

1. the candidate's exact delay arrays must be placed in `.pool` in a **silent target-validation build**;
2. real JieLi target must link;
3. target report must show ordinary `.data+.bss` remains safely below 98,304 B;
4. total pool must remain below 344,064 B with useful margin;
5. default M4g/Phase-5 PCM/runtime must remain bit-identical while the room renderer is not called.

Only after this gate may the selected room become audible.

## 15. Phase-6 non-goals

Not part of the initial room-selection slice:

- visual Space page;
- physical encoder/button mapping;
- persistent project storage;
- final hardware listening/tuning;
- unrelated per-source reverbs;
- changing M3c scheduler semantics;
- changing the five frozen dry-engine identities.

## Exit condition for the contract/prototype stage

Phase-6 production integration may start only when:

- this topology/source contract is frozen;
- both native candidates have deterministic host evidence;
- one candidate is selected with documented perceptual/resource rationale;
- silent real-target memory validation proves the selected storage layout fits FM-1 without consuming unsafe ordinary RAM.
