# PROJECT_STATE

Last updated: 2026-11-02

## Current milestone

**Phase 12 — physical FM-1 hardware validation and release.**

Phase 11 is complete and merged to `main` as `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c` via PR #21. The final accepted pre-hardware production baseline is P11W; P11Z closed the software/real-target/pinned-emulator matrix.

Active branch: `p12-hardware-validation`

Active PR: #23 — `Phase 12: physical FM-1 hardware validation and release` (draft)

Physical FM-1 hardware is not currently available. Phase 12 is therefore prepared but blocked on verification level 6.

## Product/source authority

APSIS is a parity-first native port/adaptation of the existing HTML/browser instrument to M-VAVE FM-1.

- browser/HTML APSIS is the musical/product reference
- Study 126 behavior/state/timing/safety SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- Study 98 LF String acoustic exception SHA-256: `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- Dinghy: `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree: `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product: `FM-1_972`

Frozen contracts and documented repo exceptions override assumptions from chat.

## Current frozen production authority — P11W

- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B** — **10,780 B headroom**
- POOL **319,176 / 344,064 B** — **24,888 B headroom**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines
- current host `sizeof(ApsisRuntime) = 21,032 B`

Frozen idle emulator authority:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue 28/512
- overflow 0
- late flags 0
- trace `811c9dc5`

Frozen active Motion authority:
- image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- points 149/149
- record 220,672 → 441,088
- cycle 220,416 samples
- manual override 450,560
- resume 466,176
- cursor 481,024
- queue 30/512
- overflow 0
- late flags 0

Detailed Phase-11 authority: `docs/PHASE11_FINAL_CHECKPOINT.md`.

## Frozen architecture/invariants

- M3c remains the only musical scheduler/runtime timeline.
- Do not introduce module-private musical clocks or RNG authorities without explicit re-approval.
- One shared musical state; visual/control layers must not create shadow musical state.
- Space/Cathedral is one shared room with five source sends, not five independent reverbs.
- Native display is 240×240, reuses Dinghy canvas, uses 8-row DMA-gated strips and stays outside `app_audio()`.
- Physical vocabulary remains 14 buttons, 7 relative/detent encoders and 27 keys; MASTER remains platform output gain; no encoder push is assumed.
- KEEP/CHAIN persistence remains the frozen two-sector CRC journal at `0x000F4000` / `0x000F5000`.
- Frozen tests are not rebaselined merely to accommodate new code.
- New optimization after P11Z requires measured evidence or physical timing data.

## Phase 11 closure

Accepted production changes included:
- String Motion store **19,208 → 9,644 B**
- Bloom late slots **64 → 48** after exact legal max 44 proof
- Cathedral stereo-pair delay-read sharing
- FDN feedback-gain caching
- FDN line-read specialization
- exact sine endpoint cache with final A+delta representation

Rejected candidates remain rejected without new physical evidence:
- P11T selective narrow Q15
- P11Y fixed ±1 add/sub output mix

Final merge-candidate head passed the **18/18** active PR regression matrix. P11Z run `41403698227` passed all eight closure gates.

## Phase 12 gates

1. P12A — install and rescue
2. P12B — physical I/O smoke
3. P12C — functional musical parity
4. P12D — real-time ISR/callback timing
5. P12E — LCD/control/audio contention stress
6. P12F — runtime stack high-water
7. P12G — real NOR persistence and interruption
8. P12H — DAC/listening and perceptual tuning
9. P12I — release freeze

Detailed procedures and PASS/FAIL rules: `docs/PHASE12_HARDWARE_VALIDATION_PLAN.md`.

## Verified now

Verification levels 1–5 cover the relevant completed work: implementation, host tests, sanitizer coverage, real JieLi target builds and pinned-emulator smoke/regression gates.

The Phase-12 documentation head before this continuity pass (`0cf6a277613eb20689199800886384bf6f8c93c3`) passed all **18/18** active PR workflows.

## Explicitly unverified

Nothing is yet verification level 6. In particular:
- real install/rescue behavior
- physical controls, LEDs and display behavior
- real callback/ISR timing margin
- runtime stack high-water
- LCD/audio contention on hardware
- real NOR erase/program timing and power interruption
- DAC/listening quality and perceptual tuning

## Exact next action

**Do not make speculative production changes while hardware is unavailable.**

When a physical FM-1 is available, start with **P12A**: regenerate and verify the frozen P11W package, install it on the unit, prove repeated boot, prove rescue/back-to-official on the same unit, then reinstall APSIS. Only then proceed to later physical gates.
