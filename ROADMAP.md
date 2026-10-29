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

### M3a - shared sample-referenced event grid
- [x] implemented, target/emulator verified and merged

Merged at `fe2911c43fd69e63839220482e50bb10c7a2e4c2`.

### M3b - structural/runtime consumers
- [x] implemented, target/emulator verified and merged

Merged at `a61a40dc83c9b52129f7d3f920da26a9a61837d9`.

### M3c - late binding and long deterministic trace
- [x] exact Study-126 occurrence/bind contract
- [x] Spiral/Bloom late binding and shared pre-target state observations
- [x] one allocation-free shared scheduler, 512 entries proven by stress
- [x] deterministic replay and 60 s max-swing capacity stress
- [x] lifecycle parity
- [x] core + ASAN/UBSAN
- [x] real JieLi target
- [x] FM-1 emulator
- [x] final documentation-head gate
- [x] PR #6 squash-merged

Merged to `main` at `ec0dc0b350793572f19040efc938f00121128dc6`.
Final pre-merge head `4fa7d25920523b9e5d8591ae6185e1b5f910c083` passed core #123 and staged #113.

## Phase 4 - five dry source engines

Active branch: `m4-string-dsp`, based directly on the M3c merge.

### M4a - String persistent dry core

- [x] lock Study-98 acoustic / Study-126 behavioural DSP contract
- [~] pure fixed-point Foundation + Classic sustain renderer
- [ ] source-derived target tests
- [ ] smoothing / continuity / bounded-Q15 / long-render tests
- [ ] ASAN/UBSAN
- [ ] document native dry Dual strategy
- [ ] integrate proven String DSP into FM-1 single translation unit
- [ ] update emulator gate for controlled nonzero String audio
- [ ] real JieLi target resource/timing measurement

M4a starts with the persistent sustain only. Pluck/rhythm/Attractor transients are deliberately deferred until the sustain core is proven.

Normative String DSP references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`

### Remaining dry engines

- [ ] String transient/rhythm/Attractor follow-up
- [ ] Drift
- [ ] Clock
- [ ] Spiral
- [ ] Bloom

No shared Space during initial dry-engine work.

## Phase 5 - live structural behaviour / sounding-source integration

- [ ] pending Gravity with sounding sources
- [ ] Mother held edits with audio engine
- [ ] Chain changes with audio/event engine
- [ ] prove M3c last-moment Spiral/Bloom binding drives sounding voices unchanged

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

- [ ] ISR timing under real DSP load
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