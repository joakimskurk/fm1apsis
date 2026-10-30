# Phase 7 final technical checkpoint

Date: 2026-10-30

Status: **complete and merged via PR #17 as `e9000b2b2f74d4fafe96df87f030cc55916a8285`**

Frozen firmware source candidate: `e290813f5e67affc7404dcb9fc7af63d2c671a8c`

Final documentation/CI head: `2fd054d90a49ff8e52544c092937dadc8e4097d9`

## Scope completed

Phase 7 adds persistent APSIS project memory without changing the frozen M3c musical/runtime/audio path:
- all 8 KEEP snapshots + selected slot;
- 32 CHAIN cells + count/editor selection + behavioural settings;
- explicit v1 little-endian wire format;
- CRC-protected header/payload;
- two-sector generation journal;
- header-last commit and older-valid fallback;
- dedicated FM-1 project sectors outside Dinghy settings;
- pre-audio boot prefetch;
- deferred save outside `app_audio()`;
- host corruption/torn-write tests, fake-NOR integration tests and sanitizers;
- real JieLi target and pinned-emulator regression gates.

## Record contract

- magic `APS7`
- version 1
- 24-byte header
- 816-byte payload
- 840-byte record
- CRC-32/IEEE
- sector A `0x000F4000`
- sector B `0x000F5000`
- 4 KiB per sector

Transient CHAIN playback counters/state, the M3c scheduler and DSP histories are intentionally not persisted.

## Power-loss model

The inactive sector is erased, payload is written and read back first, and only then is the 24-byte CRC-protected header programmed. Until that final header is valid, boot ignores the new sector. Newest valid generation wins; if it is corrupt, the older valid sector is retained.

Desktop/fake-NOR evidence covers erased sectors, payload-only interruption, partial header, failed header program, corrupt readback, later corruption fallback and generation wraparound. This is not a substitute for physical power-cut testing.

## Audio-safety model

Boot:
- project NOR reads happen before `fm1_audio_init()` and ALNK0 DMA;
- project state is later decoded from RAM after defaults/KEEP/CHAIN reset.

Save:
- UI/request path sets one pending byte only;
- service runs from `app_main()` only with transport stopped;
- audio is explicitly quiesced before NOR erase/program and restarted afterward;
- static CI rejects flash write/service calls in `app_audio()`.

## Regression found and fixed

The first target integration performed boot project reads after audio/DMA had started. That candidate retained exact runtime telemetry, and P6/P7 audio was bit-identical from frame 2048 onward, but frames 1804..2047 contained 202 changed frames with maximum absolute difference 2 Q15 steps.

The project did **not** adopt a new PCM baseline. Splitting boot into pre-audio NOR prefetch plus later RAM-only decode restored the complete frozen hashes exactly.

## Final real-target evidence

Audible P7:
- product `FM-1_972`
- image **110,848 / 581,564 B**
- image headroom **470,716 B**
- RAM **88,628 / 98,304 B**
- RAM headroom **9,676 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package **610,110 B**
- package SHA-256 `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`

Room-return-disabled P7:
- image **106,192 / 581,564 B**
- RAM **88,628 / 98,304 B**
- POOL **309,532 / 344,064 B**
- package SHA-256 `36524423cc5744988abaa49970d8134ada531c514517104dbb9ac50050b2ee29`

`ApsisRuntime` remains 21,928 B; the additional static RAM is the persistence staging/control storage, not a scheduler expansion.

## Final pinned-emulator evidence

Audible:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — exact P6
- frames 124,928
- peak 856
- RMS 144/144
- stereo delta 126
- queue high-water 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

Room-return-disabled:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b` — exact Phase 5
- frames 124,928
- peak 856
- RMS 144/144
- stereo delta 125
- queue high-water 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

## Final merge gate

On exact final documentation/CI head `2fd054d90a49ff8e52544c092937dadc8e4097d9`:
- APSIS core #436: PASS
- APSIS staged validation #425: PASS
- APSIS Phase 6 final checkpoint #32: PASS
- APSIS Phase 7 storage #19: PASS
- APSIS Phase 7 target persistence #12: PASS

PR #17 was marked ready and squash-merged with an expected-head lock, producing merge commit `e9000b2b2f74d4fafe96df87f030cc55916a8285`.

## Verification boundary

Level 5 is reached for implementation/build and empty-flash emulator behaviour. A physical FM-1 is not available. Do not claim:
- real NOR power-cut robustness;
- actual flash stall duration;
- real DAC/listening quality;
- physical control/display behaviour;
- true worst-case ISR-cycle margin.

## Handoff to Phase 8

Phase 8 starts from the frozen audio/runtime/persistence path and adds native 240×240 visuals only. The first work package is a visual/layout contract + bounded-cost graphics primitives. UI rendering must consume the existing shared APSIS state graph rather than creating shadow musical state.