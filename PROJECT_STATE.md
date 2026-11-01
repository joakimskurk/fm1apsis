# PROJECT_STATE

Last updated: 2026-11-01

## Current milestone

**Phase 9 — physical interaction parity is complete and merged as `bbfab322557b6760425503268b550b789f796c23` via PR #19. Phase 10 — String Motion and advanced interactive parity is current.**

Phase-9 freeze:
- technical firmware head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`
- final documentation/CI head `dc288b6424b957ad382997f62cd6924b4146f675`
- merge commit `bbfab322557b6760425503268b550b789f796c23`
- target image 132,264 B; RAM 89,492 B; POOL 309,532 B
- package `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`
- final exact-head matrix: core #504, staged #493, P6-final #96, P7-storage #83, P7-target #76, P8-visuals #86, P8-target #61, P9-controls #84, P9-H4b #8, P9-target #41 — **all PASS**

Phase 8 remains merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` via PR #18.
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

Integrated P6/P7/P8/P9 idle boot-to-stop scenario:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

Frozen Phase-5 dry PCM:
- `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Source DSP and final output remain 44.1 kHz. The generated FM-1 Space/Cathedral renderer remains the frozen quarter-rate 11.025 kHz target adaptation with one shared room and five source sends. Long room storage remains in dedicated pre-`.pool` NOLOAD memory.

## Phase 7 frozen project contract

Persisted:
- all 8 KEEP snapshots + selected KEEP slot
- up to all 32 CHAIN cells + chain count + editor-selected cell
- 13 CHAIN behavioural/default/scope settings

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
- later decode is RAM-only
- save request is a flag/dirty intent only
- erase/program is serviced from main loop with transport stopped/audio quiesced
- no flash write/erase path is reachable from `app_audio()`

## Phase 8 frozen visual contract

Ten native 240×240 surfaces remain deterministic:
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

Target adapter:
- reuses Dinghy `cv_px`; no second framebuffer and no POOL growth
- 8-row native strips gated by HAL `fm1_lcd_done()`
- display work remains outside `app_audio()`
- frozen Phase-7 legacy boot draw remains until runtime cursor sample **8192**, then native rendering takes over
- all ten surfaces remain target-addressable

## Phase 9 frozen interaction contract

Pinned panel vocabulary:
- 14 buttons
- 7 relative/detent encoders
- 27 note keys
- MASTER stays platform volume
- no encoder-push exists

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
- SEL+FX -> SPACE/CATHEDRAL

HOME is absolute. PLAY alone uses the existing shared runtime transport; SEL+PLAY is CHAIN transport.

Controls act on existing shared state/runtime structures only. Relative encoder deltas edit from the current parameter value, so no absolute pickup/soft-takeover shadow state is needed.

Key interaction freezes:
- CLOCK edits existing main/inner/snap/group structures.
- GRAVITY degree requests use `apsis_runtime_request_gravity()`.
- STRING uses three overlapping 9-key Study-126 source-keyboard banks across the 27 keys.
- SPIRAL edits ordered raw point ids `0..35` by route index; duplicates remain legal.
- BLOOM preserves distinct petal/core/grow gestures.
- KEEP/CHAIN use the existing 8-slot/32-cell structures.
- MOTHER structural edits retain `->1` pending/commit semantics.
- SPACE edits the one shared room.

## Phase 9 direct performance/audio ownership

Manual performance does not fabricate transport events or create a private scheduler.

STRING:
- P9-only `apsis_string_manual_audio.*`
- source keyboard pluck level 0.080
- Classic reuses BODY+WIRE finite topology; Dual reuses selected side
- 30-ms / 1323-sample retrigger debounce
- main -> audio one-byte mailbox; audio callback owns finite transient mutation
- historical M4b/M4f files remain byte-for-byte frozen and are hash-guarded by the P9 generator

BLOOM:
- P9-only finite manual adapter over existing M4g binding/DSP engine
- direct petal scale 1.04
- grow scale `0.78 + 0.055*i`
- source grow thresholds retained
- direct petal hears display/pending Gravity immediately
- core freezes/advances one voice-leading plan and late-binds future petals at existing native lead timing
- four concurrent manual core snapshots / twenty pending petals
- core uses the one shared M3c RNG; no private UI RNG
- packed 16-bit main -> audio mailbox; short IRQ guard only around producer read/modify/write

P7 project writes remain deferred/coalesced and outside active audio.

## Phase 9 frozen target evidence

Technical head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- product `FM-1_972`
- image **132,264 / 581,564 B**
- RAM **89,492 / 98,304 B**
- RAM headroom **8,812 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package bytes 610,110
- package SHA-256 `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`

Pinned emulator:
- PCM exact P6/P7/P8 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`
- dynamic HOME -> MOTHER PASS

Historical M4f bypass remains exactly **95,804 B**; its historical cursor/PCM gate is not relaxed.

## Phase-9 regression lesson

The first direct String implementation placed manual-audio helpers inside the historical M4b String Rhythm source. That moved the frozen M4f bypass image from 95,804 to 95,820 B even though Phase 9 was not supposed to change M4f. The project rejected the new size rather than updating the baseline.

The final implementation restores the M4b `.c/.h/test` blobs exactly and moves manual String into a P9-only module. The P9 generator hard-checks the frozen blob hashes before target generation. This keeps Phase-9 performance work layered above historical audio authorities.

## Phase 10 current scope

Phase 10 owns advanced interaction deliberately deferred from Phase 9:
1. Clock-quantized String Motion capture contract;
2. Classic/Dual ownership and frozen pitch context;
3. overdub/manual override behavior;
4. String Attractor editor;
5. Spiral Attractor editor;
6. selected Gravity-point editor;
7. deterministic host/sanitizer contracts before target integration;
8. target/emulator regression preserving frozen Phase-9 idle audio/runtime.

Do not reopen the frozen Phase-9 mappings merely to make Phase-10 editors convenient. Phase 10 must layer advanced edit state onto the existing shared APSIS model and shared M3c timeline.

## Verification boundary

Phase 9 is verified through implementation, deterministic host tests, ASAN/UBSAN, real JieLi target build and pinned emulator idle regression. It is **not** physical-FM-1 verified.

Still unverified:
- physical control feel/ergonomics and switch/encoder behavior
- actual LEDs/display on hardware
- real ISR margin under active manual performance
- DAC/listening quality
- real NOR timing/power interruption
- worst-case combined control/UI/audio stress

## Exact next action

Begin **Phase 10A — String Motion contract** from merged `main`:
1. freeze Study-126 String Motion capture/playback/ownership semantics before target edits;
2. define the minimum native capture state needed outside the frozen `ApsisRuntime` layout;
3. host-test Clock-quantized capture, Classic/Dual ownership, overdub and manual override;
4. only then bind Phase-10 controls into the generated FM-1 target;
5. retain exact Phase-9 idle PCM `c4fa1895…`, cursor 123,392 and zero late/overflow flags as the target regression gate.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless re-approved by evidence
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- historical M4f bypass retains image 95,804 B
- integrated P6/P7/P8/P9 idle scenario retains cursor 123,392 with zero late/overflow flags
- audible idle PCM remains exact P6/P7/P8; dry-only remains exact Phase 5
- project persistence stays versioned/journaled/CRC-protected and never blocks active audio
- native visuals consume existing APSIS state; no UI shadow musical state
- native boot takeover remains after sample 8192 unless revalidated against exact PCM
- Phase-9 direct performance uses finite mailboxes/adapters; no private scheduler/RNG
- physical FM-1 and real ISR/storage/display/control behaviour remain unverified
