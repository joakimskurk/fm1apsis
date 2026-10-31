# PROJECT_STATE

Last updated: 2026-10-31

## Current milestone

**Phase 8 — native 240×240 visual surfaces is complete and merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` via PR #18. Phase 9 — physical interaction parity is current.**

Phase-8 freeze:
- technical source head `87575aa829f3fab6af43092b440d585745eecc25`
- final documentation/CI head `856c4e35ae1f041da8ccdc555f4c82b54f43a04a`
- merge commit `4d71d712b34a684fb8f74f6b98ed675a9d5e1457`
- exact-head gates: core #460, staged #449, P6-final #54, P7-storage #41, P7-target #34, P8-visuals #44 and P8-target #19 — **all PASS**

Phase 7 remains merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285` via PR #17.
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

Integrated P6/P7/P8 boot-to-stop scenario:
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
- payload programmed/verified first; commit header last
- boot selects newest fully valid generation; corrupt/torn newest falls back to older valid generation

Audio safety:
- boot NOR prefetch runs before `fm1_audio_init()` / ALNK0 DMA startup
- record decode runs later from RAM after defaults/KEEP/CHAIN reset
- save request is a flag only
- erase/program is serviced only from main loop with transport stopped and audio quiesced
- no flash write/erase path is reachable from `app_audio()`

## Phase 8 frozen visual contract

Native 240×240 surfaces:
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

Visual architecture:
- allocation-free integer raster with bounded clipping/primitives
- reads existing `ApsisState` / `ApsisRuntime` directly
- KEEP/CHAIN additionally observe the existing `ApsisKeepBank` / `ApsisChainSequence` read-only
- no visual layer owns musical state
- full-frame output is deterministic and equivalent to strip rendering in host tests
- normal and ASAN/UBSAN visual tests pass

Target adapter:
- reuses Dinghy `cv_px`; no second framebuffer and no POOL growth
- native redraw uses 8-row strips
- strip starts only when prior LCD DMA is complete, via HAL helper `fm1_lcd_done()`
- display work remains outside `app_audio()`
- frozen Phase-7 legacy boot draw is retained until runtime cursor sample **8192**, then native rendering takes over
- all 10 surfaces are target-addressable
- SPACE deliberately has no physical button mapping until Phase 9

## Phase 8 frozen target evidence

Technical head `87575aa829f3fab6af43092b440d585745eecc25`:
- product `FM-1_972`
- image **118,040 / 581,564 B**
- RAM **88,644 / 98,304 B**
- RAM headroom **9,660 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package bytes 610,110
- package SHA-256 `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`

Pinned emulator, audible:
- PCM exact P6/P7 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`
- HOME returns to the live MOTHER surface; MOTHER is not required to be byte-identical at different runtime times

## Phase-8 regression lesson

The first native full-screen target integration changed PCM despite perfect runtime telemetry. Large LCD updates could block in `cv_begin()`/`lcd_sync()` on the previous 12 MHz LCD DMA. Moving to DMA-gated 8-row strips removed the blocking redraw, but repeated native rendering during boot still changed only frames 1795..4351; frames from 4352 onward were already bit-identical to a fresh Phase-7 reference.

We did **not** accept a new audio baseline. Retaining the exact Phase-7 legacy boot draw through runtime sample 8192 and only then switching to the native incremental renderer restored the complete frozen PCM hash while keeping all ten native surfaces available.

## Verification boundary

Phase 8 is verified through implementation, desktop deterministic tests, ASAN/UBSAN, real JieLi target build and pinned emulator. It is **not** physical-FM-1 verified.

The visual contract is source-derived and deterministic; screenshot/pixel-identical parity with the browser has not been claimed. Real LCD colour/contrast, redraw timing under physical IRQ load and control ergonomics remain unverified.

## Exact next action — Phase 9

1. freeze the FM-1 physical interaction/control contract before changing mappings;
2. map direct access for Clock, Gravity, String, Spiral, Bloom, KEEP, CHAIN, MOTHER and SPACE using the existing shared state graph;
3. preserve HOME -> MOTHER and avoid UI shadow musical state;
4. define encoder/pot pickup and edit semantics without mutating untouched parameters;
5. add deterministic input-routing tests before target integration;
6. validate that control polling/LED work does not change the frozen audio/runtime baseline.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless re-approved by evidence
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- integrated P6/P7/P8 scenario retains cursor 123,392 with zero late/overflow flags
- audible PCM remains exact P6/P7; dry-only remains exact Phase 5
- project persistence stays versioned/journaled/CRC-protected and never blocks active audio
- native visuals consume existing APSIS state; no UI shadow musical state
- native boot takeover remains after sample 8192 unless revalidated against exact PCM
- Phase 9 owns physical mapping; Phase 8 does not invent a SPACE button binding
- physical FM-1 and real ISR/storage/display behaviour remain unverified
