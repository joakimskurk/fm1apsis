# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed
- [~] active / implementation complete, merge pending
- [ ] not started

## Phase 0 - reference freeze

- [x] lock Study 126 behavioural source
- [x] behavioural map / parity matrix / port contract
- [x] verify attached current `index.html` is the same source modulo LF/CRLF
- [x] lock Study 98 as String acoustic reference only

## Phase 1 - pinned FM-1 skeleton

- [x] pinned Dinghy baseline and APSIS identity `FM-1_972`
- [x] host / real target / emulator validation pipeline

## Phase 2 - pure musical/state core

- [x] defaults/state, tuning, Gravity, String, Drift, Clock, Spiral, Bloom, KEEP, CHAIN, MOTHER

Merged through `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

## Phase 3 - deterministic transport/runtime

- [x] M3a shared sample-referenced event grid — merged `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- [x] M3b structural/runtime consumers — merged `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- [x] M3c late binding/long deterministic trace — merged `ec0dc0b350793572f19040efc938f00121128dc6`

M3c frozen runtime: 21,928 B; 512-event shared heap; no module-private clocks.

## Phase 4 - five dry source engines

### M4a - String persistent dry core

Merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7.

- [x] Study-98 Foundation + Classic sustain
- [x] Study-126 Dual BODY/WIRE preservation
- [x] fixed-point 44.1 kHz stereo Q15 renderer
- [x] deterministic host references + ASAN/UBSAN
- [x] target-safe single-TU build
- [x] live `app_audio()` integration
- [x] real JieLi target + FM-1 emulator audio

Frozen host: Classic 1477 / `b20610c1`; Dual 823/787 / `10d23cf4`.
Frozen PCM: `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`.

### M4b - String finite transients + shared Rhythm audio

Merged to `main` as `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8.

- [x] fixed 80 logical +32 fade-tail finite pool
- [x] Study-98 normal/pizz harmonic primitive
- [x] Study-126 BODY finite primitive and Classic composite
- [x] shared `APSIS_EVENT_STRING_RHYTHM`; no private scheduler
- [x] late-read pitch/surface/levels with frozen planned mode
- [x] callback-boundary proof: never early, <1 block late
- [x] target/emulator regression

Frozen finite refs: transient 1309 / `97142d3e`; BODY 947 / `c1168570`.

### M4c - String Attractor excitation

Merged to `main` as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9.
Final pre-merge head `fe53bf265882369e2c8c1e38899e9cc655301dd6` passed core #202 + staged #191.

- [x] seeded S&H planner with 0.5/1/2/4/8 rates
- [x] LEFT/RIGHT/ALTERNATE/BOTH planning-time routing
- [x] Attractor random order before Spiral/ghost
- [x] shared target−45 ms bind event; no private queue
- [x] 64 pending slots, six bind outputs; worst legal 60 s stress
- [x] late-read pitch/levels/brightness/Foundation
- [x] BODY/WIRE/Classic finite topology + right octave accent
- [x] exact target-sample pool delay
- [x] default OFF path bit-identical to M4b
- [x] real JieLi target + emulator

Frozen M4c stress: heap 415/512, pending 60/64, random `b7a9a395`, late `e777a771`.
Frozen target: image 85,564 B; RAM 44,680 B; package `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`.

### M4d - persistent Drift dry engine

Merged to `main` as `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10.
Final pre-merge documentation head `82106c33…` passed core #214 + staged #203.

Normative reference: `docs/reference/M4D_DRIFT_CONTRACT.md`.

- [x] source-resolve Study-126 missing persistent Drift wiring as explicit parity disposition
- [x] two-sine field around live tuned Gravity root
- [x] source nonlinear 0.05..4.00 Hz delta law
- [x] depth floor 0.15 + source-level scaling
- [x] `.013 * level` per-voice gain
- [x] historical width law ±0.34×width with equal-power pan
- [x] 25 ms pitch/gain/pan smoothing
- [x] separate fixed persistent DSP state; no event/queue/random changes
- [x] host normal/ASAN/UBSAN
- [x] real JieLi target + default emulator
- [x] DSP-bypass proof back to exact M4c package/PCM/runtime

Frozen host Drift: peak 488, hash `0aec6bc2`.
Frozen default M4d PCM: `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`.

### M4e - Clock dry engine

Status: **implementation complete; PR #11 awaiting final documentation-head double gate**.

Active branch: `m4e-clock`, based directly on M4d merge `d7e39e2481c33628f402bed51d761b8be8304bb3`.
Validated implementation head: `285aacb5309fd9abaa51814e4ea6a09a3e480a10` — core #225 PASS, staged #214 PASS.

Normative reference: `docs/reference/M4E_CLOCK_CONTRACT.md`.

- [x] source-correct Clock acoustic state to STONE / WOOD / GLASS / SOFT
- [x] source-correct continuous pitch representation to `pitch_centi=50..200`, default 100
- [x] consume existing shared MAIN / SNAP / INNER / GHOST events; no private scheduler
- [x] preserve target samples including pulse+4 ms Snap and half-interval Inner/Ghost
- [x] source-derived kick profiles and GLASS accent overtone
- [x] source-derived three-burst Snap + pitched digital edge
- [x] deterministic Clock-local Snap noise; zero shared musical RNG consumption
- [x] 32-slot fixed finite Clock pool; measured legal stress 23/32, 176 triggers, 0 drops
- [x] fixed-point 44.1 kHz stereo Q15, no target `libm`
- [x] target-only `__divdi3`/`__udivdi3` compatibility shim preserving exact integer DSP arithmetic
- [x] host references frozen and identical under ASAN/UBSAN
- [x] real JieLi target + default Clock-on emulator
- [x] CI-only Clock-DSP bypass preserves frozen M4d PCM/runtime
- [ ] exact final documentation-head double gate and PR #11 squash-merge

Frozen host Clock references:
- STONE accent: peak 2081 / `edcc4a81`
- GLASS accent: peak 1880 / `0adf9829`
- SNAP: peak 738 / `04bc514a`

Implementation-head default M4e target/emulator:
- image 90,684 / 581,564 B
- RAM 49,128 / 98,304 B
- pool 30,720 / 344,064 B
- package SHA `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- emulator peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- raw PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- runtime queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

Gate 5b removes only the new Clock DSP/consumer while retaining the source-correct Clock state:
- image 86,544 B; RAM 44,760 B; pool 30,720 B
- bypass package SHA `3670d58da19a1ccad01a142ff328b6faf273901450b61b361dd436ad5041990f`
- PCM exactly frozen M4d `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- runtime telemetry exactly frozen M4d
- package SHA is intentionally not required to equal M4d because M4e corrects Clock acoustic state representation even in the bypass build

### Remaining dry engines

- [ ] Spiral
- [ ] Bloom

No shared Space during initial dry-engine work.

## Phase 5 - live structural behaviour / sounding-source integration

- [ ] sample-exact state application to sounding sources
- [ ] pending Gravity with sounding sources
- [ ] Mother held edits with audio engine
- [ ] Chain changes with audio/event engine
- [ ] prove M3c last-moment Spiral/Bloom binding drives sounding voices unchanged

M4b Rhythm intentionally remains at most one 256-frame block late and never early. M4c Attractor can hit its frozen target exactly because its source already provides a pre-target bind lead.

## Phase 6 - shared Space / Cathedral

- [ ] prototype compact modulated FDN
- [ ] prototype nested allpass/Schroeder candidate
- [ ] compare diffusion/metallicity/decay/CPU/memory
- [ ] one shared room with five per-source sends

## Phase 7 - persistent KEEP / CHAIN storage

- [ ] 8-slot project persistence
- [ ] 32-cell chain persistence
- [ ] versioned CRC records
- [ ] two-slot/journal recovery
- [ ] power-loss/corruption tests

## Phase 8 - native visual surfaces

- [ ] MOTHER / GRAVITY / STRING / CLOCK / SPIRAL / BLOOM / DRIFT-TUNING / CHAIN / KEEP / SPACE

## Phase 9 - physical interaction parity

- [ ] direct FM-1 mappings for Clock, Gravity, String, Spiral, Bloom, KEEP, CHAIN, MOTHER

## Phase 10 - String Motion and advanced interactive parity

- [ ] Clock-quantized capture
- [ ] Classic/Dual ownership and frozen pitch context
- [ ] overdub/manual override
- [ ] String/Spiral attractor and Gravity point editors

## Phase 11 - FM-1 optimisation

- [ ] real ISR timing under DSP load
- [ ] bit-identical reduction of proven-safe 64-bit intermediates
- [ ] RAM/pool/stack headroom
- [ ] queue and voice-pool stress
- [ ] UI redraw load
- [ ] Cathedral tuning

## Phase 12 - hardware validation/release

- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

1. Update/freeze M4e continuity documentation from implementation head `285aacb5…` without changing DSP/runtime semantics.
2. Gate that exact final documentation head with both `APSIS core` and `APSIS staged validation`.
3. If both are green, update PR #11 metadata without moving the head, mark it ready and squash-merge with an `expected_head_sha` guard.
4. Start M4f / Spiral dry engine from the actual M4e merge commit.
