# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop/host build and deterministic tests
3. sanitizer test
4. real FM-1 JieLi target build
5. pinned emulator smoke/regression
6. physical FM-1 hardware

**Physical FM-1 hardware is not currently available. Nothing is level 6.**

## Pinned platform

- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`
- source/output rate 44.1 kHz
- callback 256 frames
- image budget 581,564 B
- ordinary RAM budget 98,304 B
- POOL budget 344,064 B

## Current production authority — Phase 11 P11W

Merged Phase 11: `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c` via PR #21.

Final merge-candidate head passed the **18/18** active PR regression matrix. P11Z run `41403698227` passed all eight closure gates.

Frozen target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**
- POOL **319,176 / 344,064 B**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- FDN render **1,094 B / 364** static instruction lines

Current host runtime:
- `sizeof(ApsisRuntime) = 21,032 B`
- shared event heap 512
- canonical queue high-water 352/512
- extreme queue high-water 357/512
- Spiral late slots 59/64
- Bloom late slots 44/48; exact legal maximum 44

Frozen idle emulator:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue 28/512
- overflow 0
- late flags 0
- trace `811c9dc5`

Frozen active String Motion:
- image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- 149/149 points
- record 220,672 → 441,088
- cycle 220,416 samples
- override 450,560
- resume 466,176
- cursor 481,024
- queue 30/512
- overflow 0
- late flags 0

Full Phase-11 evidence: `docs/PHASE11_FINAL_CHECKPOINT.md` and the phase-specific evidence documents under `docs/`.

## Historical regression anchors retained

These remain intentional historical anchors and must not be confused with current production sizes:

- historical pre-P11F `sizeof(ApsisRuntime) = 21,928 B`
- historical M4/M5 emulator cursor 123,648, queue 28/512, overflow 0, late 0, trace `811c9dc5`
- frozen Phase-5 dry PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- historical M4f Bloom-bypass target image 95,804 B

Historical gates remain preserved where they protect earlier behavior; a later optimization must not silently rebaseline them.

## Phase 12 testing state

Phase 12 is hardware-led. Detailed procedure and PASS/FAIL definitions live in `docs/PHASE12_HARDWARE_VALIDATION_PLAN.md`.

The documentation-only Phase-12 head `0cf6a277613eb20689199800886384bf6f8c93c3` passed all **18/18** active PR workflows before the continuity-documentation update.

Required physical gates:
- P12A install/rescue
- P12B controls/LED/display/audio-I/O smoke
- P12C full musical parity
- P12D real callback/ISR timing
- P12E LCD/control/audio contention
- P12F runtime stack high-water
- P12G real NOR persistence/interruption
- P12H DAC/listening/perceptual tuning
- P12I release freeze

## Explicitly unverified

Do not infer any of these from static sizes, emulator behavior or target-build success:

- physical install/rescue reliability
- real `app_audio()` timing margin
- runtime stack high-water
- physical LCD DMA/audio contention
- physical controls/LED/display ergonomics
- real NOR erase/program timing and power interruption
- DAC/listening quality

Any new Phase-12 branch head must rerun the relevant regression matrix before being treated as release evidence.
