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

M3c final pre-merge head `4fa7d25920523b9e5d8591ae6185e1b5f910c083` passed core #123 and staged #113.

## Phase 4 - five dry source engines

Active branch: `m4-string-dsp`, PR #7, based directly on the M3c merge.

### M4a - String persistent dry core

- [x] lock Study-98 acoustic / Study-126 behavioural DSP contract
- [x] resolve Dual authority: Study-126 BODY/WIRE because Study 98 predates Dual
- [x] fixed-point Study-98 Foundation pair
- [x] fixed-point Study-98 Classic sine/Drift pair
- [x] Study-126 Dual BODY/WIRE oscillator/filter/pan path
- [x] source-derived target vectors
- [x] smoothing / continuity / bounded-Q15 / long-render tests
- [x] deterministic Classic/Dual audio hashes
- [x] ASAN/UBSAN
- [x] target-safe single-TU namespacing
- [x] real JieLi compile/link gate
- [x] live String renderer in `app_audio()`
- [x] controlled non-zero stereo FM-1 emulator audio
- [x] preserve M3c queue/late-binding telemetry under audio load
- [x] record target image/RAM/pool and first audible WAV reference
- [~] exact final documentation-head core + staged gate
- [ ] squash-merge PR #7

First audible implementation head `839bc7b65695b7fa81a6336995537d374b8a3e1b` passed staged #126:
- target image 75,700 B
- RAM 35,464 B
- pool 30,720 B
- package SHA `eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`
- emulator peak 317, RMS L/R 101/100, stereo delta 63
- raw PCM SHA `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- M3c queue 28/512, overflow 0, late flags 0, late trace `811c9dc5`

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

M4a remains the persistent sustain only. The first reference audio is intentionally frozen before transient work.

### Next String slice after M4a merge

- [ ] Study-98 `classicStringNote()` transient engine
- [ ] optional Gravity-root shadow
- [ ] String Rhythm excitation/modes
- [ ] Attractor excitation
- [ ] preserve Study-126 transient teardown/pulse-isolation safety

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

Current M4a block boundary is explicit: structural changes may sound at most one 256-frame block late (~5.8 ms), never early.

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