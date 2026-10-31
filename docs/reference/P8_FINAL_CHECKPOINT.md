# Phase 8 Final Checkpoint — Native 240×240 Visual Surfaces

Date: 2026-10-31

Technical source head: `87575aa829f3fab6af43092b440d585745eecc25`

This checkpoint freezes the Phase-8 technical candidate before guarded PR #18 merge. It is an evidence record, not a claim of physical FM-1 validation.

## Scope

Phase 8 adds a native 240×240 visual system for APSIS while preserving the frozen musical, scheduler, DSP and persistence paths.

The visual layer is a read-only view of existing APSIS state:
- `ApsisState`
- `ApsisRuntime`
- `ApsisKeepBank` for KEEP
- `ApsisChainSequence` for CHAIN

It does not own a duplicate musical state model.

## Frozen native surfaces

| Surface | deterministic hash |
|---|---|
| MOTHER | `27373657` |
| GRAVITY | `55aff30b` |
| STRING | `00532f1d` |
| DRIFT / TUNING | `9ff4af95` |
| CLOCK | `903a5f21` |
| SPIRAL | `562a4043` |
| BLOOM | `7fcc0856` |
| KEEP | `df715375` |
| CHAIN | `709ed554` |
| SPACE / CATHEDRAL | `9605ed04` |

Normal and ASAN/UBSAN host visual suites pass. The tests also prove deterministic equivalence between full-frame rendering and tiled/strip rendering, including the production 30 × 8-row layout.

The contract is source-derived from the frozen APSIS reference/state semantics. No screenshot/pixel-identical browser claim is made.

## Surface semantics

- **MOTHER**: whole-system structural view derived from shared state/runtime.
- **GRAVITY**: active harmonic/gravity mapping.
- **STRING**: source identity with visible split anchors/central veil treatment.
- **DRIFT/TUNING**: paired detune/tuning field from existing Drift state.
- **CLOCK**: orbit/timeline geometry from existing Clock state.
- **SPIRAL**: route projected onto spiral geometry.
- **BLOOM**: probability/cluster representation of Bloom state.
- **KEEP**: eight real memory slots; selection and active CHAIN source read from existing banks.
- **CHAIN**: actual up-to-32-cell sequence with editor/play/progress distinctions.
- **SPACE/CATHEDRAL**: one shared Cathedral with five source sends; never five per-source reverbs.

## Target display adapter

The target adapter deliberately avoids a second framebuffer:
- reuses Dinghy `cv_px`
- no additional POOL allocation
- POOL remains 309,532 B
- renders full-screen transitions as 8-row strips
- starts a strip only when prior LCD DMA is complete
- reads LCD completion through HAL helper `fm1_lcd_done()`; UI source never directly accesses SPI registers
- display rendering is unreachable from `app_audio()`

All ten visual surfaces are target-addressable. SPACE/CATHEDRAL deliberately receives no physical FM-1 button gesture in Phase 8; physical access policy is Phase 9 work.

## Boot/audio coexistence regression and resolution

The first target implementation showed that a visually correct renderer can still perturb the frozen audio stream.

Large native display updates could wait in Dinghy `cv_begin()` / `lcd_sync()` for the previous 12 MHz LCD DMA. Moving to one HAL-gated 8-row strip per main-loop pass removed that blocking pattern, but the first native boot redraw still changed the exact PCM hash.

A fresh P8-vs-P7 WAV comparison localized the remaining difference to frames **1795..4351**. Frames **4352..124927** were already bit-identical; runtime cursor, queue, overflow and late telemetry were also exact.

The project did not accept a new PCM baseline. The final solution preserves the exact frozen Phase-7 legacy boot draw until the shared runtime cursor reaches sample **8192** (about 186 ms at 44.1 kHz). Native incremental rendering then takes over. This restores complete bit-exact P6/P7 audio while leaving native visuals active well before the tested 500 ms display snapshot.

## Frozen target evidence

Real JieLi target on technical head `87575aa829f3fab6af43092b440d585745eecc25`:
- product `FM-1_972`
- image **118,040 / 581,564 B**
- RAM **88,644 / 98,304 B**
- RAM headroom **9,660 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package bytes **610,110**
- package SHA-256 `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`

Pinned emulator audible scenario:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — exact P6/P7
- peak 856
- RMS 144 / 144
- stereo delta 126
- queue 28 / 512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- dynamic HOME -> MOTHER contract PASS

Tested display snapshots in the emulator remain dynamic; returning HOME is required to return to MOTHER, not to reproduce an earlier MOTHER bitmap byte-for-byte after musical runtime has advanced.

## Do-not-regress additions from Phase 8

- native visuals remain read-only consumers of existing APSIS state
- no second framebuffer unless separately measured/approved
- POOL baseline stays 309,532 B for the Phase-8 target
- display work remains outside `app_audio()`
- LCD completion register access stays behind HAL
- native boot takeover remains at/after runtime sample 8192 unless exact PCM is revalidated
- all ten surfaces remain addressable
- physical SPACE mapping is not silently invented before Phase 9
- integrated target scenario keeps exact P6/P7 PCM, cursor 123,392, queue 28/512 and zero overflow/late flags

## Verification boundary

Verified:
- deterministic host rendering
- ASAN/UBSAN visual tests
- real JieLi target compilation/link/resource accounting
- pinned FM-1 emulator boot/input/display/audio/runtime coexistence

Not verified:
- physical FM-1 LCD colour/contrast
- physical redraw timing under real IRQ/DMA load
- physical control ergonomics
- true ISR-cycle margin
- DAC/listening quality
- pixel-identical appearance against a browser screenshot

## Handoff to Phase 9

After PR #18 merges, Phase 9 must freeze the physical interaction contract before assigning controls. It should map FM-1 buttons/encoders/pots to the existing surfaces and shared APSIS state without creating UI shadow state, and must retain the exact audio/runtime regressions while control polling and LED feedback are active.
