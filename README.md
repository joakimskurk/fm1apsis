# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Start here

For a new ChatGPT chat or returning contributor, read these first in order:

1. `PROJECT_STATE.md` — exact current milestone and next action
2. `ROADMAP.md` — operational phase sequence
3. `DECISIONS.md` — durable architecture/product decisions
4. `TESTING.md` — what is actually verified
5. `CHANGELOG.md` — milestone history
6. `docs/reference/P9_FINAL_CHECKPOINT.md` — Phase-9 physical-control technical evidence
7. `docs/reference/P9_INTERACTION_CONTRACT.md` — frozen FM-1 interaction contract
8. `docs/reference/P8_FINAL_CHECKPOINT.md` — Phase-8 native-visual technical evidence
9. `docs/reference/P7_FINAL_CHECKPOINT.md` — Phase-7 persistence evidence
10. `docs/reference/P7_STORAGE_CONTRACT.md` — frozen project-storage format

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
- Phase 8 native 240×240 visual surfaces: complete and merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` via PR #18
- Phase 9 physical interaction parity: complete and merged as `bbfab322557b6760425503268b550b789f796c23` via PR #19
- Phase 10 String Motion + advanced interactive parity: **current**

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

Historical M4/M5 emulator baseline remains cursor **123,648**. The integrated Phase-6/7/8/9 idle scenario remains separately frozen at cursor **123,392**, queue 28/512, overflow 0, late flags 0 and trace `811c9dc5`.

## Phase 8 — frozen native visuals

Phase 8 adds a read-only, allocation-free visual layer over the existing APSIS state graph. It does not create musical/UI shadow state.

Ten native surfaces are frozen and deterministic: MOTHER, GRAVITY, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, KEEP, CHAIN and SPACE/CATHEDRAL. KEEP and CHAIN read the existing banks/sequences directly; SPACE shows one shared Cathedral with five sends.

Target display integration:
- reuses Dinghy's existing canvas scratch; no second framebuffer
- native updates are 8-row incremental strips
- each strip starts only when the prior LCD DMA is complete through HAL helper `fm1_lcd_done()`
- display rendering never runs from `app_audio()`
- the exact frozen Phase-7 legacy boot draw is retained until runtime cursor sample 8192, then native rendering takes over

## Phase 9 — frozen physical interaction contract

The pinned panel is now routed onto the existing shared APSIS state/runtime model. There is no second UI musical model and no private scheduler.

Surface navigation:
- HOME -> MOTHER
- ENV -> STRING
- LFO -> DRIFT/TUNING
- SEQ -> CLOCK
- ARP -> SPIRAL
- FX -> BLOOM
- GLO -> GRAVITY
- EDIT -> CHAIN
- SAVE -> KEEP
- SEL+FX -> shared SPACE/CATHEDRAL

MASTER remains platform output gain. Pinned Dinghy exposes seven relative/detent encoders and no encoder-push input; parameter edits therefore do not require a hidden absolute pickup/soft-takeover state.

Phase 9 also binds direct performance safely:
- String key gestures use a P9-only manual adapter over the existing finite String transient pool and the source keyboard pluck level 0.080;
- Bloom petal/core/grow gestures use a P9-only finite adapter over the existing M4g Bloom binding/DSP engine;
- direct Bloom petal and grow retain distinct source gains;
- Bloom core uses the shared M3c RNG and late-binds later petals without creating a private musical clock;
- main loop publishes narrow intents; audio callback remains the sole owner of finite DSP mutation.

Structural edits remain MOTHER-aware, Gravity still uses the existing runtime request path, and project changes still use Phase-7 deferred/journaled persistence outside active audio.

Frozen technical Phase-9 head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- image **132,264 / 581,564 B**
- RAM **89,492 / 98,304 B**
- POOL **309,532 / 344,064 B**
- package SHA-256 `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`
- audible PCM remains bit-exact P6/P7/P8: `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

The final documentation/CI head `dc288b6424b957ad382997f62cd6924b4146f675` passed the full exact-head matrix: core #504, staged #493, P6-final #96, P7-storage #83, P7-target #76, P8-visuals #86, P8-target #61, P9-controls #84, P9-H4b #8 and P9-target #41. PR #19 then squash-merged as `bbfab322557b6760425503268b550b789f796c23`.

See `docs/reference/P9_FINAL_CHECKPOINT.md` and `docs/reference/P9_INTERACTION_CONTRACT.md` for the frozen technical contract.

## Phase 10 — current work

Phase 10 adds the advanced interaction layer that was deliberately excluded from Phase 9: Clock-quantized String Motion capture, Classic/Dual ownership and frozen pitch context, overdub/manual override, String/Spiral Attractor editors and selected Gravity-point editing. These features must remain layered over the existing shared APSIS state/runtime and preserve the frozen Phase-9 idle audio/runtime unless explicit evidence approves a change.

## Important verification boundary

The firmware builds for the real JieLi target and passes the pinned FM-1 emulator. A physical FM-1 is not currently available, so no claim is made yet about real DAC sound, true ISR-cycle headroom under active manual performance, physical UI ergonomics, physical LED/display behaviour or real power-cut storage robustness.
