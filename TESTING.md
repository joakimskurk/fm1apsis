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

Integrated P6/P7 target scenario:
- cursor **123,392**
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Frozen Phase-5 dry PCM:
`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Frozen Phase-6 audible PCM:
`c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

## Phase 6 target evidence retained

Audible P6:
- image 107,352 B
- RAM 87,780 B
- POOL 309,532 B
- audible PCM exact hash above

Room-return-disabled P6:
- exact Phase-5 PCM
- same integrated cursor 123,392

The generated target Cathedral remains 11.025 kHz internally while source DSP, scheduler and output remain 44.1 kHz. Long Space storage remains in dedicated pre-`.pool` NOLOAD memory.

## Phase 7 desktop/storage verification

`p7-storage.yml` verifies:
- standard CRC-32/IEEE check vector
- explicit v1 payload encode/decode and canonical re-encode
- transient CHAIN runtime is not persisted
- payload/header/version corruption rejection
- erased second slot recovery
- payload-only interrupted save recovery
- partial-header interrupted save recovery
- newest valid generation selection
- later corruption fallback to older valid record
- generation wraparound
- fake FM-1 NOR save A -> save B -> reboot/load newest
- failed commit-header write keeps older project bootable
- corrupted payload readback aborts before commit header
- unavailable flash path is non-destructive
- normal plus ASAN/UBSAN execution

These are desktop/fake-NOR tests, not physical power-cut verification.

## Phase 7 target architecture verification

Project sectors:
- A `0x000F4000`
- B `0x000F5000`
- Dinghy settings `0x000FC000` is not used for APSIS projects

Static/generated-target guards prove:
- only the two project sectors are added to the pinned flash allow-list
- project NOR prefetch is called before `fm1_audio_init()`
- project decode after defaults is RAM-only
- `fl_erase4k`, `fl_write` and project-save service are not reachable from `app_audio()`
- save service requires stopped transport and explicitly quiesces/restarts audio

## Final technical Phase-7 candidate

Firmware source head: `e290813f5e67affc7404dcb9fc7af63d2c671a8c`.

Real JieLi target, audible:
- product `FM-1_972`
- image **110,848 / 581,564 B**
- RAM **88,628 / 98,304 B**
- RAM headroom **9,676 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package 610,110 B
- package SHA-256 `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`

Pinned emulator, audible:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — **bit-exact P6**
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`

P7 room-return-disabled target:
- image **106,192 B**
- RAM **88,628 B**
- POOL **309,532 B**
- package SHA-256 `36524423cc5744988abaa49970d8134ada531c514517104dbb9ac50050b2ee29`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b` — **exact Phase 5**
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

## Regression caught during P7

The first target integration read project NOR from `app_main()` after ALNK0 audio/DMA had started. Runtime telemetry remained exact, and P7/P6 audio was bit-identical after frame 2048, but 202 frames in the first active block differed by at most 2 Q15 steps. This was rejected as a regression. Moving read-only project prefetch before audio/DMA startup restored the complete P6 audible and Phase-5 dry hashes.

## Current verification boundary

Phase 7 is verified through level 5 for the empty-flash boot path: implementation, desktop tests, sanitizers, real JieLi target build and pinned emulator.

Persistence save/reload logic is verified with deterministic desktop fake-NOR tests and target compilation, but the current emulator gate does not emulate a user save/reboot sequence. Still unverified until physical hardware exists:
- real ISR-cycle margin
- actual DAC/audio listening quality
- real NOR erase/program timing
- physical power interruption during project writes
- full physical controls/LED/display behaviour
- worst-case combined UI/audio load

## Next verification

Run core + staged + P6-final + P7-storage + P7-target on the final documentation head, then guarded-merge PR #17.