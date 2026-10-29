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

- [x] Study-98 Foundation + Classic sustain
- [x] Study-126 Dual BODY/WIRE preservation
- [x] fixed-point 44.1 kHz stereo Q15 renderer
- [x] deterministic host references + ASAN/UBSAN
- [x] target-safe single-TU build
- [x] live `app_audio()` integration
- [x] real JieLi target + FM-1 emulator audio
- [x] preserve M3c telemetry under audio load
- [x] squash-merge PR #7

Merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Frozen M4a evidence:
- Classic host peak/hash 1477 / `b20610c1`
- Dual host L/R 823/787 / `10d23cf4`
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

### M4b - String finite transients + shared Rhythm audio

- [x] fixed 80 logical +32 fade-tail finite pool
- [x] Study-98 normal/pizz harmonic primitive
- [x] Study-126 BODY finite primitive and Classic composite
- [x] shared `APSIS_EVENT_STRING_RHYTHM` consumption; no private scheduler
- [x] freeze BOW/PIZZ/MOTION only; late-read pitch/surface/levels
- [x] callback-boundary proof: never early, <1 block late
- [x] normal + ASAN/UBSAN
- [x] real JieLi target + emulator regression
- [x] squash-merge PR #8

Merged to `main` as `c96c00debf9a67948f1e874e36b090999dacadb9`.
Final pre-merge documentation head `264a32e8…` passed core #174 + staged #163.

Frozen M4b evidence:
- transient peak/hash 1309 / `97142d3e`
- BODY peak/hash 947 / `c1168570`
- legal burst 80/112, 480 steals, 0 drops
- Classic composite 80/108, 410 steals, 0 drops
- target image 81,752 B; RAM 42,648 B; pool 30,720 B
- default Rhythm OFF remains exact M4a PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

### M4c - String Attractor excitation

Status: **implementation complete; PR #9 awaiting final documentation-head double gate**.

Active branch: `m4c-string-attractor`, based directly on M4b merge `c96c00debf9a67948f1e874e36b090999dacadb9`.
Validated implementation head: `e2171caac10eb402e97a6d566eff71a27544e3fc` — core #198 PASS, staged #187 PASS.

Normative reference:
- `docs/reference/M4C_STRING_ATTRACTOR_CONTRACT.md`

#### M4c1 - state + shared silent planner

- [x] explicit LEFT/RIGHT/ALTERNATE/BOTH target state; default ALTERNATE
- [x] source-seeded 16-cell-max S&H runtime
- [x] exact reset/resync/lock/mutation semantics
- [x] rates 0.5/1/2/4/8 on shared Phase-3 timeline
- [x] Attractor random draws before Spiral/ghost
- [x] FILTER-only advance; SUSTAIN no draw/event
- [x] appended shared `STRING_ATTRACTOR_BIND` event
- [x] external runtime preserves `sizeof(ApsisRuntime)=21,928 B`
- [x] fixed pending/binding capacities from measured legal stress
- [x] OFF/null path remains byte/hash-equivalent to frozen M3c/M4b

Frozen M4c1 stress evidence:
- shared heap 415/512
- pending sides 60/64
- binding output max 6 in deliberate 512-frame stress; normal 256-frame startup <=4
- advances/mutations/binds 1470 / 1470 / 2884
- random trace `b7a9a395`; late trace `e777a771`
- zero overflow/starvation
- normal + ASAN/UBSAN agree

#### M4c2 - target−45 ms late bind + finite excitation

- [x] freeze planning-time surface/route/value/position/base level
- [x] late-read sound/pitch/source-level/side-level/brightness/Foundation
- [x] BODY 720 ms / -0.30 / 30 ms attack / no audible root shadow
- [x] WIRE 420 ms / +0.30 / ×0.92 / Foundation root 0.05
- [x] preserve actual Study-126 WIRE brightness behavior (unused `opts.brightness` does not alter pizz)
- [x] Classic BODY + WIRE composite with +4 ms WIRE leg
- [x] right `norm>0.52` octave sine accent, including Classic +10 ms total offset
- [x] one shared 80+32 finite pool; no second audio pool
- [x] full plan -> bind -> pool-delay target proof
- [x] live target integration with frozen M4b default-path bypass until first Attractor activation
- [x] real JieLi target + emulator regression

Implementation-head evidence from core #198 / staged #187:
- full-chain target 36,750; bind-block end 34,816; remaining exact delay 1,934 samples
- target image 85,564 / 581,564 B
- RAM 44,680 / 98,304 B
- pool 30,720 / 344,064 B
- package SHA `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`
- emulator peak 317; RMS 101/100; stereo delta 63; mean -2/-2
- M3c queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`
- default Attractor OFF raw PCM restored exactly to `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

### Remaining dry engines

- [ ] Drift
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

1. Finish M4c documentation-only final head (`ROADMAP.md`, `PROJECT_STATE.md`, `TESTING.md`, M4c contract).
2. Gate that exact head with both `APSIS core` and `APSIS staged validation`.
3. Update PR #9 metadata with exact final-head evidence without moving the head.
4. Mark PR #9 ready and squash-merge with an expected-head SHA guard.
5. Start the next dry source engine from the actual M4c merge commit; current roadmap order is Drift unless a later explicit decision changes it.
