# M3c deterministic timing / late-binding reference

Status: canonical pre-DSP Phase 3 reference.

Authority:
- behavioural/timing source: locked Study 126 `apsis.html`
- source SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- native implementation: branch `m3c-late-binding`
- this reference is intentionally silent; it is a timing/state trace, not an audio fingerprint

## Canonical replay

The canonical long host replay is the `tests/test_runtime.c` stress profile:

- sample rate: 44.1 kHz
- duration: 30 seconds
- deterministic seed: `0x31415926`
- Clock: 3 pulses, 180 BPM, 1/4 division
- Clock main/snap/inner paths maximally populated
- Clock ghost probability: 100%
- step-one resync: enabled for stress coverage
- String rhythm: every pulse
- Spiral: 4x attractor subdivision, three-note route, all gates on
- Bloom: every eligible cycle, 4x rate, ninth depth, 3.6 s unfurl
- one shared allocation-free event heap
- no DSP and no audio output

Reference result:

- `sizeof(ApsisRuntime)`: 21,928 B on the host build
- queue high-water: **352 / 512**
- Spiral late-bind slot high-water: **59 / 64**
- Bloom occurrence/bind slot high-water: **44 / 64**
- queue overflow: **0**
- scheduler starvation: **0**
- late-slot overflow: **0**
- random-decision trace hash: **`78771ff5`**
- late-binding/state-observation trace hash: **`0e6fc422`**

The test runs this seed twice and requires identical high-water values, counts and hashes. It also runs a different seed and requires at least one trace hash to differ.

## Extreme burst capacity replay

`tests/test_runtime_extreme.c` is a separate 60-second capacity proof. It adds the maximum source swing used by the Clock model and processes in 512-frame chunks to add scheduling-burst margin:

- 180 BPM
- 1/4 division
- 35% swing
- Spiral 4x, all gates
- Bloom 4x, ninth depth, 3.6 s unfurl
- maximum secondary Clock activity

Measured result, normal and ASAN/UBSAN:

- queue high-water: **357 / 512**
- Spiral slots: **59 / 64**
- Bloom slots: **44 / 64**
- queue overflow: **0**
- scheduler starvation: **0**
- late-slot overflow: **0**

The earlier 256-entry heap failed the legal worst-case profile by setting `scheduler_starved`. That failure is the evidence for the current 512-entry capacity.

## Source-observation timing represented by the trace

The trace includes explicit shared-timeline state-observation events rather than browser timer jitter:

- normal quantized Gravity shared-state mutation: target − 18 ms
- Mother structural commit callback: Clock target − 26 ms
- Chain discrete apply callback: Clock target − 24 ms; continuous morph begins at target
- Spiral final pitch observation: target − 80 ms on the FM-1 native profile
- Bloom occurrence observation: target − 45 ms
- Bloom petal final observation: `max(occurrence, voiceTarget − 50 ms)`

Spiral observes committed state only. Bloom may target-project ordinary pending Gravity degree/ring according to Study 126's `+0.5 ms` rule. Mother-held state is never projected before the real Mother commit event.

## Target / emulator evidence for the same M3c firmware

A real JieLi target build and FM-1 emulator run on the target-identical M3c implementation reported:

- app image: **71,608 B / 581,564 B**
- RAM `.data + .bss`: **35,272 B / 98,304 B**
- RAM headroom: **63,032 B**
- pool: **30,720 B / 344,064 B**
- package bytes: **610,110 B**
- product identity: `FM-1_972`
- package SHA-256: `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Emulator telemetry:

- live queue high-water: **28 / 512**
- queue overflow: **0**
- late-bind/starvation flags: **0**
- runtime cursor low 32 bits: **123,648**
- late trace hash after smoke: **`811c9dc5`**
- stereo channels: 2
- WAV frames: 124,928
- WAV peak: **0**

The zero peak is required: M3c is the final silent transport/state milestone. Phase 4 DSP remains blocked until M3c is merged and green.

## What this reference does not prove

It does not prove:
- physical FM-1 behavior or ISR timing
- future source DSP quality
- Cathedral/Space CPU cost
- physical controls/LEDs/display parity
- storage power-loss recovery

Those require later phases and physical hardware evidence.
