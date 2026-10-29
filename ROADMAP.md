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

### M4a - String persistent dry core

- [x] Study-98 Foundation + Classic sustain
- [x] Study-126 Dual BODY/WIRE preservation
- [x] fixed-point 44.1 kHz stereo Q15 renderer
- [x] deterministic host references + ASAN/UBSAN
- [x] target-safe single-TU build
- [x] live `app_audio()` integration
- [x] real JieLi target + FM-1 emulator audio
- [x] preserve M3c timing/late-binding telemetry under audio load
- [x] exact final documentation-head core #141 + staged #130
- [x] squash-merge PR #7

Merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`.
Final validated pre-merge head: `afe81ae3984797bcb3b99153c96b493dfb8575ec`.

Frozen M4a evidence:
- Classic host peak/hash: 1477 / `b20610c1`
- Dual host L/R peak/hash: 823/787 / `10d23cf4`
- target image 75,700 B; RAM 35,464 B; pool 30,720 B
- package SHA `eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`
- emulator peak 317; RMS L/R 101/100; stereo delta 63
- raw PCM SHA `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- M3c queue 28/512, overflow 0, late flags 0, late trace `811c9dc5`

Normative M4a references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

### M4b - String finite transients + shared Rhythm audio

Status: **implementation complete; PR #8 merge pending final documentation-head gate**.

Active branch: `m4b-string-transients`, based directly on M4a merge `2ea217f09c143d30385f136e84e2b29d582987a0`.
Validated implementation head: `0749ac86e6162fab190f343730ebc9b133c13e47` — core #170 PASS, staged #159 PASS.

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

#### M4b1 - pure transient engine

- [x] audit Study-98 `classicStringNote()` + generic `tone()` mechanics
- [x] audit Study-126 Classic/Dual topology and Study-124/125/126 de-click overrides
- [x] lock source hierarchy and M4b transient contract
- [x] fixed-capacity transient pool with explicit fade-tail safety
- [x] Study-98 normal 1..4 harmonic primitive
- [x] Study-98 pizz 1..3 harmonic primitive
- [x] fixed-point brightness-dependent harmonic attenuation
- [x] 3 ms partial staggering at sample resolution
- [x] 16 ms Study-126 String onset safety override
- [x] triangle low-pass / sine bypass behavior
- [x] folded Gravity-root shadow
- [x] deterministic pan topology
- [x] pool high-water/steal/drop telemetry and legal-burst stress
- [x] preserve idle M4a Classic/Dual hashes bit-for-bit
- [x] normal host + ASAN/UBSAN

Frozen M4b1 evidence:
- transient peak/hash 1309 / `97142d3e`
- pool 80 logical + 32 fade-tail reserve = 112 physical slots
- legal burst 80/112, 480 steals, 0 drops
- BODY peak/hash 947 / `c1168570`
- Classic-composite stress 80/108, 410 steals, 0 drops
- `ApsisStringDsp` remains the M4a 196-byte layout; transient state is separate

#### M4b2 - existing String Rhythm event becomes audible

- [x] consume existing `APSIS_EVENT_STRING_RHYTHM`; no String-private clock/queue
- [x] preserve transport's pulse+26 ms event timing
- [x] freeze only planned BOW/PIZZ/MOTION mode in event metadata
- [x] late-read String pitch/surface/levels at bind
- [x] preserve Study-126 Classic composite topology
- [x] preserve Dual side alternation/levels and BOW/PIZZ/MOTION mapping
- [x] preserve M3c queue/hash/capacity semantics and 21,928-byte runtime
- [x] full transport -> observer -> trigger -> render timing test
- [x] real JieLi target + FM-1 emulator regression

Validated M4b2 target/emulator evidence from staged #159:
- target image 81,752 B / 581,564 B
- RAM 42,648 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`
- default Rhythm OFF keeps emulator bit-identical to M4a: raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- emulator peak 317; RMS L/R 101/100; stereo delta 63; mean L/R -2/-2
- M3c queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`
- callback-boundary proof: source target sample 1147 first becomes audible at sample 1280, 133 samples late and never early

#### Later String gate - Attractor excitation

- [ ] source-resolve bowed/composite acoustic mapping
- [ ] preserve Study-126 late pitch/side-level re-read
- [ ] native lead policy on shared sample timeline
- [ ] Attractor upper-octave accent rule
- [ ] target/emulator stress and reference freeze

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

Current M4a/M4b callback boundary remains explicit until Phase 5: events/state changes may sound at most one 256-frame block late (~5.8 ms), never early.

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

1. Gate this documentation-only final head with both `APSIS core` and `APSIS staged validation`.
2. Update PR #8 metadata with the exact final-head evidence without moving the head.
3. Mark PR #8 ready and squash-merge with an expected-head SHA guard.
4. Start the next String slice from the actual M4b merge commit; Attractor remains a separate late-bind gate and must not be silently folded into M4b.
