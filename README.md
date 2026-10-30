# APSIS for M-VAVE FM-1

Native FM-1 port of **APSIS**, developed as a parity-first translation of the browser instrument to the physical M-VAVE FM-1.

## Start here

For a new ChatGPT chat or returning contributor, read these first in order:

1. `PROJECT_STATE.md` — exact current milestone and next action
2. `ROADMAP.md` — operational phase sequence
3. `DECISIONS.md` — durable architecture/product decisions
4. `TESTING.md` — what is actually verified
5. `CHANGELOG.md` — milestone history
6. `docs/reference/P6_FINAL_CHECKPOINT.md` — Phase-6 final technical evidence

The repository, not chat history, is the durable source of project state.

## Current status

- Phase 0 reference freeze: complete
- Phase 1 pinned FM-1 skeleton/build/emulator pipeline: complete
- Phase 2 pure musical/state core: complete and merged
- Phase 3 deterministic shared transport/runtime: complete and merged
- Phase 4 five dry source engines: complete and merged
- Phase 5 sounding-source structural integration: complete and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`
- Phase 6 shared Space/Cathedral: **implementation and technical gates complete; PR #16 is the guarded merge vehicle**
- Phase 7 persistent KEEP/CHAIN storage is next after Phase 6 merges

Physical FM-1 validation has not yet been performed. Real hardware ISR margin and listening quality therefore remain unverified.

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

Phase-5/M4g frozen default emulator baseline remains cursor **123,648**, queue 28/512, no overflow/late flags, trace `811c9dc5`, dry PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`.

## Phase 6 — shared Space/Cathedral

Phase 6 is now integrated into the generated FM-1 firmware path:

- five post-source/post-Mother sends feed one shared room
- selected compact modulated 8-line FDN
- large room memories live in dedicated pre-`.pool` NOLOAD storage
- host/reference room remains 44.1 kHz
- generated FM-1 Cathedral runs internally at **11.025 kHz** using 4:1 send averaging/wet hold, with its delay/filter/decay/modulation constants retimed to preserve real-time behaviour
- dry source engines and final output remain 44.1 kHz

Final technical target candidate:

- image **107,352 / 581,564 B**
- RAM **87,780 / 98,304 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- dry-only PCM remains exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- Phase-6 boot-to-stop cursor **123,392** for both audible and dry-only variants
- queue 28/512, overflow 0, late flags 0, late trace `811c9dc5`

The one-callback cursor difference from the older Phase-5 scenario is an explicit Phase-6 scenario baseline, not a scheduler rewrite. Both audible and room-return-disabled P6 variants land on the same cursor and retain healthy runtime telemetry.

## Important verification boundary

The firmware builds for the real JieLi target and passes the pinned FM-1 emulator. No physical FM-1 is currently available, so no claim is made yet about real DAC sound, physical controls, display behaviour under full load, or true ISR-cycle headroom.

The 240x240 native visual surfaces and physical control mapping remain later roadmap phases.