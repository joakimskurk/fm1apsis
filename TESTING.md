# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

Physical FM-1 is not currently available. Nothing below is level 6.

## Pinned baseline

- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`
- target image budget 581,564 B
- RAM budget 98,304 B
- POOL budget 344,064 B
- source/output rate 44.1 kHz, 256-frame callback

## Frozen runtime/audio regressions

M3c host runtime:
- `sizeof(ApsisRuntime)=21,928 B`
- queue 352/512 canonical; 357/512 extreme
- Spiral slots 59/64; Bloom slots 44/64
- random `78771ff5`; late `0e6fc422`

Historical M4/M5 target scenario:
- cursor **123,648**
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Integrated P6/P7/P8 target scenario:
- cursor **123,392**
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Frozen Phase-5 dry PCM:
`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Frozen Phase-6/7/8 audible PCM:
`c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

## Phase 7 storage verification retained

`p7-storage.yml` verifies CRC-32/IEEE, explicit v1 encode/decode, transient-state exclusion, corruption rejection, interrupted saves, newest-generation selection, fallback, generation wraparound, fake-NOR save/reboot/recovery, commit failure handling and ASAN/UBSAN.

Target guards prove project prefetch before audio startup, RAM-only later decode, dedicated sectors at `0x000F4000` / `0x000F5000`, and no erase/program/save service reachable from `app_audio()`.

These are deterministic/fake-NOR and target-build tests, not physical power-cut verification.

## Phase 8 deterministic visual verification

`p8-visuals.yml` verifies normal and ASAN/UBSAN rendering for all ten surfaces.

Frozen 32-bit visual references:
- MOTHER `27373657`
- GRAVITY `55aff30b`
- STRING `00532f1d`
- DRIFT/TUNING `9ff4af95`
- CLOCK `903a5f21`
- SPIRAL `562a4043`
- BLOOM `7fcc0856`
- KEEP `df715375`
- CHAIN `709ed554`
- SPACE/CATHEDRAL `9605ed04`

The host visual gates additionally verify that tiled/strip rendering reconstructs the same deterministic full-frame output, including the production 30 × 8-row strip geometry.

The renderer is read-only with respect to musical state. KEEP/CHAIN rendering observes the existing `ApsisKeepBank` and `ApsisChainSequence`; SPACE renders one shared room with five sends.

This is a source-derived visual regression contract, not a claim of screenshot-identical browser pixels.

## Phase 8 target architecture verification

Generated-target guards prove:
- native display work stays outside `app_audio()`
- Dinghy's existing `cv_px` scratch is reused; no second framebuffer is added
- POOL remains exactly 309,532 B
- LCD completion register access remains inside HAL via `fm1_lcd_done()`
- native display updates are 8-row strips and do not start while the previous LCD DMA is active
- the exact Phase-7 legacy boot draw remains in use until runtime cursor sample 8192
- all ten native surfaces, including SPACE/CATHEDRAL, are addressable by the target adapter
- Phase 8 does not assign a physical SPACE button; that mapping belongs to Phase 9
- HOME still routes to MOTHER

## Final technical Phase-8 candidate

Firmware source head: `87575aa829f3fab6af43092b440d585745eecc25`.

Real JieLi target:
- product `FM-1_972`
- image **118,040 / 581,564 B**
- RAM **88,644 / 98,304 B**
- RAM headroom **9,660 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package 610,110 B
- package SHA-256 `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`

Pinned emulator, audible:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — **bit-exact P6/P7**
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- dynamic HOME -> MOTHER contract passes

## Regression caught during Phase 8

The first native target renderer changed PCM even though cursor, queue, overflow and late telemetry were all exact. Investigation showed large LCD updates could wait in Dinghy `cv_begin()`/`lcd_sync()` for previous 12 MHz LCD DMA completion.

An 8-row DMA-gated renderer removed the blocking update pattern, but a fresh P8-vs-P7 frame diff still showed audio differences only from frames 1795 through 4351; frame 4352 onward was bit-identical. The project again refused to relax its audio baseline.

The final adapter preserves the exact Phase-7 legacy boot draw through runtime sample 8192 and then enables the incremental native renderer. This restores the complete P6/P7 PCM hash while retaining native surfaces and no POOL growth.

## Current verification boundary

Phase 8 is verified through level 5 for the tested target scenario: implementation, desktop deterministic tests, sanitizers, real JieLi target build and pinned emulator.

Still unverified until physical hardware exists:
- real ISR-cycle margin
- actual DAC/audio listening quality
- real LCD DMA/IRQ coexistence and visible redraw behaviour
- physical colour/contrast/layout quality
- final physical controls/LED behaviour
- real NOR erase/program timing and power interruption
- worst-case combined UI/control/audio load

## Next verification

Run core + staged + P6-final + P7-storage + P7-target + P8-visuals + P8-target on the final documentation head, then guarded-merge PR #18.
