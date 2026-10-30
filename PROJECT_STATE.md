# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 7 — persistent KEEP / CHAIN storage is technically complete on PR #17 and awaiting the final guarded merge. Phase 8 — native 240×240 visual surfaces is next.**

Current Phase-7 technical candidate:
- head `e290813f5e67affc7404dcb9fc7af63d2c671a8c`
- PR #17
- P7 storage host/fake-NOR/ASAN-UBSAN gates: **PASS**
- P7 target gates: **PASS all gates**

Phase 6 remains merged as `cd321e9f7999d0568d280d89e7e3574c3da66054` via PR #16.

Physical FM-1 hardware remains unavailable. Nothing is promoted to physical-hardware verification.

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behaviour/state/timing/safety authority:
- Study 126 SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime/audio

M3c remains the only musical scheduler/runtime timeline.

Host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- heap 512 events
- canonical queue 352/512; extreme 357/512
- Spiral slots 59/64; Bloom 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Historical M4/M5 pinned emulator baseline:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648**
- late trace `811c9dc5`

Integrated P6/P7 boot-to-stop scenario:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`

Source DSP and final output remain 44.1 kHz. The generated FM-1 Space/Cathedral renderer remains the frozen quarter-rate 11.025 kHz target adaptation with one shared room and five source sends. Long room storage remains in dedicated pre-`.pool` NOLOAD memory.

## Phase 7 frozen project contract

Persisted:
- all 8 KEEP snapshots + selected KEEP slot
- up to all 32 CHAIN cells + chain count + editor-selected cell
- 13 CHAIN behavioural/default/scope settings

Not persisted:
- CHAIN runtime/transport counters and armed/playing state
- shared scheduler/runtime state
- DSP histories/voices

Wire format v1:
- magic `APS7`
- explicit little-endian bytes
- 24-byte header
- 816-byte payload
- 840-byte complete record
- CRC-32/IEEE for payload and header
- wrap-safe 32-bit generation number

Journal:
- sector A `0x000F4000`
- sector B `0x000F5000`
- each owns one 4 KiB erase sector
- payload programmed and verified first
- 24-byte commit header programmed last
- boot selects newest fully valid generation; corrupt/torn newest falls back to older valid generation

Audio safety:
- boot NOR prefetch runs before `fm1_audio_init()` / ALNK0 DMA startup
- record decode runs later from RAM after APSIS defaults/KEEP/CHAIN reset
- save request is a flag only
- erase/program is serviced only from main loop with transport stopped and audio quiesced
- no flash write/erase path is reachable from `app_audio()`

## Frozen Phase-7 target evidence

Real JieLi target, audible:
- product `FM-1_972`
- image **110,848 / 581,564 B**
- RAM **88,628 / 98,304 B**
- RAM headroom **9,676 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package bytes 610,110
- package SHA-256 `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`

Pinned emulator, audible:
- PCM exact P6 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

P7 room-return-disabled regression:
- image 106,192 B
- RAM 88,628 B
- POOL 309,532 B
- package SHA-256 `36524423cc5744988abaa49970d8134ada531c514517104dbb9ac50050b2ee29`
- PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

## Phase-7 regression lesson

Reading project NOR from `app_main()` after FM-1 audio/DMA had already started changed only frames 1804..2047 of the first active block by at most 2 Q15 steps; runtime telemetry and all audio after frame 2048 were already identical. We did **not** accept this as a new baseline. Splitting boot into pre-audio NOR prefetch plus later RAM-only decode restored the full P6/Phase-5 hashes exactly.

## Exact next action — Phase 8

After the guarded Phase-7 merge:
1. freeze a 240×240 visual layout contract for all APSIS surfaces;
2. build native reusable graphics primitives with bounded redraw cost;
3. implement MOTHER / GRAVITY / STRING / CLOCK / SPIRAL / BLOOM / DRIFT-TUNING / CHAIN / KEEP / SPACE views from the Study-126 identity;
4. retain the shared state graph as the only UI truth — no visual shadow state;
5. compare native screens against the locked browser/source identity before Phase 9 control mapping.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless re-approved by evidence
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- integrated P6/P7 scenario retains cursor 123,392 with zero late/overflow flags
- P7 audible PCM remains exact P6; P7 dry-only remains exact Phase 5
- project persistence stays versioned/journaled/CRC-protected and never blocks active audio
- physical FM-1 and real ISR/storage power-cut behaviour remain unverified