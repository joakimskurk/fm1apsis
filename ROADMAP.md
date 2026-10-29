# ROADMAP

This is the short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`.

## Status legend

- [x] completed
- [~] active / partially implemented / merge pending
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

Status: **implementation complete; PR #10 awaiting final documentation-head double gate**.

Active branch: `m4d-drift`, based directly on M4c merge `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.
Validated implementation head: `0b2a351b17adfa7556c217f24af8a08c1c708d2c` — core #210 PASS, staged #199 PASS.

Normative reference:
- `docs/reference/M4D_DRIFT_CONTRACT.md`

- [x] source-resolve Study-126 missing persistent Drift wiring as an explicit parity disposition
- [x] restore identity-bearing two-sine field around live tuned Gravity root
- [x] source nonlinear 0.05..4.00 Hz delta law
- [x] depth floor 0.15 + source-level scaling of separation
- [x] `.013 * level` per-voice gain
- [x] historical `STEREO WIDTH` law ±0.34×width with equal-power pan
- [x] 25 ms pitch/gain/pan smoothing without low-gain integer shortcut
- [x] separate fixed persistent DSP state; no event/queue/random changes
- [x] deterministic host reference + normal/ASAN/UBSAN
- [x] live JieLi target integration
- [x] default Drift-on emulator reference
- [x] CI-only Drift-DSP bypass proves exact M4c target/package/PCM/runtime
- [ ] exact final documentation-head double gate and PR #10 squash-merge

Frozen host Drift reference: peak **488**, hash **`0aec6bc2`**.

Implementation-head default M4d target/emulator:
- image 86,544 / 581,564 B
- RAM 44,760 / 98,304 B
- pool 30,720 / 344,064 B
- package SHA `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`
- emulator peak 433; RMS 135/135; stereo delta 125; mean -3/-3
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- runtime queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Gate 5b bypass removes only `ApsisDriftDsp` while preserving all old Drift state/cross-couplings and the same target identity:
- image 85,564 B; RAM 44,680 B; pool 30,720 B
- package SHA exactly M4c `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`
- PCM exactly M4c `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- runtime telemetry exactly frozen M4c.

### Remaining dry engines

- [ ] Clock
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
- [ ] bit-identical reduction of proven-safe 64-bit String per-sample intermediates
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

1. Gate this exact M4d documentation-only final head with both `APSIS core` and `APSIS staged validation`.
2. If both are green, update PR #10 metadata with the exact final-head evidence without moving the head.
3. Mark PR #10 ready and squash-merge with an `expected_head_sha` guard.
4. Start the Clock dry-engine slice from the actual M4d merge commit.
