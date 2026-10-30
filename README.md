# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Start here

For a new ChatGPT chat or returning contributor, read these first in order:

1. `PROJECT_STATE.md` — exact current milestone and next action
2. `ROADMAP.md` — operational phase sequence
3. `DECISIONS.md` — durable architecture/product decisions
4. `TESTING.md` — what is actually verified
5. `CHANGELOG.md` — milestone history
6. `docs/reference/P7_FINAL_CHECKPOINT.md` — Phase-7 final technical evidence
7. `docs/reference/P7_STORAGE_CONTRACT.md` — frozen project-storage format

The repository, not chat history, is the durable source of project state.

## Current status

- Phase 0 reference freeze: complete
- Phase 1 pinned FM-1 skeleton/build/emulator pipeline: complete
- Phase 2 pure musical/state core: complete and merged
- Phase 3 deterministic shared transport/runtime: complete and merged
- Phase 4 five dry source engines: complete and merged
- Phase 5 sounding-source structural integration: complete and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`
- Phase 6 shared Space/Cathedral: complete and merged as `cd321e9f7999d0568d280d89e7e3574c3da66054`
- Phase 7 persistent KEEP/CHAIN storage: complete and merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285`
- Phase 8 native 240×240 visual surfaces: **current phase**

Physical FM-1 validation has not yet been performed. Real hardware ISR margin, real NOR power-interruption behaviour, DAC/listening quality and physical controls/display behaviour therefore remain unverified.

## Core rule

APSIS must remain **APSIS**. This is not a generic synth/groovebox using APSIS names.

Study 126 remains behavioural/state/timing/safety authority:
`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

Study 98 is only the narrow String acoustic reference:
`43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

The source instrument is one shared musical state seen through STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, GRAVITY, CHAIN, KEEP and MOTHER. Space/Cathedral is one shared room with five source sends, not five independent reverbs.

## Pinned platform/runtime

- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`
- source/audio rate 44.1 kHz, 256-frame callbacks
- `sizeof(ApsisRuntime)=21,928 B`
- one 512-event shared heap; no module-private musical clocks

Historical M4/M5 emulator baseline remains cursor **123,648**. The integrated Phase-6/7 scenario remains separately frozen at cursor **123,392**, queue 28/512, overflow 0, late flags 0 and trace `811c9dc5`.

## Phase 7 — persistent KEEP / CHAIN project storage

Phase 7 added persistence without changing the frozen musical/audio engine:

- explicit little-endian v1 project record; native C struct padding is never persisted
- 8 KEEP slots + KEEP selection
- 32 CHAIN cells + count/editor selection + CHAIN behavioural settings
- transient CHAIN transport/runtime state is deliberately excluded
- 24-byte CRC-protected header + 816-byte payload = 840-byte record
- two dedicated 4 KiB sectors at `0x000F4000` and `0x000F5000`
- payload-first / header-last alternating journal with generation counter and wrap-safe newest selection
- desktop corruption, interrupted-write, generation-wrap and fake-NOR reboot/recovery tests
- boot project record is prefetched from NOR **before FM-1 audio/DMA starts**, then decoded from RAM after APSIS defaults are installed
- save requests are deferred; erase/program runs only from the main loop with transport stopped and audio quiesced, never from `app_audio()`

Frozen Phase-7 technical candidate `e290813f5e67affc7404dcb9fc7af63d2c671a8c`, merged through PR #17 as `e9000b2b2f74d4fafe96df87f030cc55916a8285`:

- image **110,848 / 581,564 B**
- RAM **88,628 / 98,304 B**
- RAM headroom **9,676 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package SHA-256 `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`
- audible PCM remains bit-exact P6: `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- room-return-disabled PCM remains exact Phase 5: `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- both variants retain cursor 123,392, queue 28/512, no overflow/late flags and trace `811c9dc5`

The initial P7 target probe exposed a two-Q15-step perturbation confined to the first active audio block when NOR was read after audio startup. That was treated as a regression, not a new baseline. Moving read-only project prefetch before `fm1_audio_init()` restored the full frozen PCM hashes exactly.

## Important verification boundary

The firmware builds for the real JieLi target and passes the pinned FM-1 emulator. A physical FM-1 is not currently available, so no claim is made yet about real DAC sound, true ISR-cycle headroom, physical UI behaviour or real power-cut storage robustness.

The current implementation phase is **Phase 8 — native 240×240 visual surfaces**.