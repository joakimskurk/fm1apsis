# Phase 11L — production FDN feedback-gain cache validation

Status: **complete, exact-head validated and ready for promotion.**

P11L promotes the P11K-proved feedback-gain cache into production FDN state/reset/render and validates that actual production behavior remains exact.

## Production change

`ApsisSpaceFdn` now stores:
- eight exact `feedback_q15` values;
- the rounded `feedback_decay_ms` key that produced them;
- one validity byte.

`apsis_space_fdn_reset()` invalidates the cache. `apsis_space_fdn_render()` recomputes all eight gains only when the cache is invalid or the rounded `decay_ms` key changes. `space_feedback_gain()` arithmetic, FDN topology, damping, modulation, Hadamard mixing, input/output gains, line capacities and storage remain unchanged.

The production source diff from P11K is intentionally narrow:
- `apsis/apsis_space_dsp.c`: 9 additions / 4 deletions;
- `apsis/apsis_space_dsp.h`: 3 added cache-state fields;
- `apsis_space_room.c` remains exact P11I blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`.

## Validation authority

- production-validation head `4a08705815fffcbf9407193a21334764d8710d51`
- authoritative run `41351791632`
- all six validation gates PASS
- evidence artifact `p11l-fdn-feedback-cache-evidence`
- artifact ID `13101743282`
- artifact ZIP SHA-256 `819f932f701d2e46cf13b26a641d68130785f84cc026eec39347563903f24d72`
- artifact size 1,654,972 B

The historical validation workflow is frozen manual-only and pinned to the exact validated production head.

## Gate 1 — actual production equivalence

The actual production cache implementation was compared against a frozen pre-P11L always-recomputed render after every block. The test requires:
- bit-identical stereo output;
- equality of every pre-existing FDN state field;
- byte-identical full FDN delay storage;
- exact cache contents equal to `space_feedback_gain()` for the current decay key.

Coverage:
- reset and partial fill;
- 256-block steady run / 16,384 frames;
- exact room-style 4 s → 12 s smoothed sweep with 192 rounded-decay changes;
- 256-block adversarial 4 s/12 s alternation;
- all 8,001 legal integer decay keys, two blocks per key;
- 4,096 deterministic random/reset blocks / 92,927 frames.

Optimized build PASS and ASAN/UBSAN PASS.

## Gate 2 — complete historical host behavior

The complete frozen host and sanitizer suite remains green. Relevant frozen FDN references remain exact:
- default peak 217, hash `a190ca86`, late 2,646,000;
- decay4 1,455,300;
- decay12 3,507,773;
- dark hash `7d4c13e3`;
- bright hash `7e1a4d9e`.

## Gate 3 — real JieLi target

Actual production target exactly matches the P11K ephemeral projection:
- product `FM-1_972`
- image **140,440 / 581,564 B**
- ordinary RAM **87,492 / 98,304 B**
- RAM headroom **10,812 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`

Target FDN-render code shape also matches the P11K projection exactly:
- `apsis_space_fdn_render`: **1,118 B** text
- **379** static disassembly instruction lines

Versus P11I/P11J baseline this is intentionally:
- image **+48 B**;
- RAM **+16 B**;
- POOL **+0 B**;
- FDN-render code shape **+36 B / +26 static instruction lines**.

The trade is accepted because P11J/P11K showed that it removes expensive block-rate coefficient recomputation when rounded decay is stable, while preserving exact behavior.

## Gate 4 — exact idle emulator behavior

Frozen idle authority remains exact:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue high-water 28/512
- queue overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856
- RMS 144/144
- stereo delta 126

## Gate 5 — exact active Motion lifecycle

Active target:
- image **137,704 B**
- RAM **87,540 B**
- RAM headroom **10,764 B**
- POOL **319,176 B**
- POOL headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `54616e07ad5b579d33ec07732b5ec3daa824d71950ec8844805631251245374a`

Active Motion lifecycle remains exact:
- seen flags `ff`; final flags `14`
- 149 left / 149 right points
- record start 220,672
- record finish 441,088
- record cycle 220,416 samples
- manual override 450,560
- resume 466,176
- cursor 481,024
- queue high-water 30/512
- queue overflow 0
- late flags 0
- WAV peak 1,171
- WAV RMS 173

## Gate 6 — narrow production scope

The validation confirms that P11L modifies only the proved FDN feedback-cache state/reset/render path. Cathedral room source remains exact P11I authority.

## Workload interpretation

P11J/P11K established the dynamic workload tradeoff for a 256-FDN-block census:
- steady decay: 2,048 → 8 feedback-gain evaluations, eliminating **2,040 32-bit divisions + 6,120 wide multiplies**;
- one measured 4 s → 12 s smoothed sweep: 2,048 → 1,544 evaluations, eliminating **504 divisions + 1,512 wide multiplies**;
- adversarial every-block decay change: 2,048 → 2,048 evaluations, so coefficient-computation savings are zero and only the bounded cache-key check remains.

P11L therefore establishes the cache as the current accepted production baseline.

No physical FM-1 CPU-cycle, ISR-margin, stack-high-water or LCD/audio-contention improvement is claimed. Those remain hardware-validation questions.
