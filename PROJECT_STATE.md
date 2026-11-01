# PROJECT_STATE

Last updated: 2026-11-01

## Current milestone

**Phase 11 — FM-1 optimisation is active on `p11-optimization`. Phase 10 is complete and merged. P11A baseline inventory, P11B Motion-storage proof, and P11C compact lossless Motion storage are complete at the strongest level currently possible without physical FM-1 hardware. P11D target RAM/linker ownership inventory is next.**

Phase 10 merge authority:
- feature-complete technical head `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`
- exact-head documentation/CI gate `ce5857a52c6326011e5af7cd7f73819b420550fc`
- 22/22 required workflows PASS on that exact head
- PR #20 squash merge `378d2023df2afe59475be53c09cc001788586514`

Phase 11 branch:
- `p11-optimization`
- P11A green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8`
- P11B green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c`
- first complete P11C green run `41308798678` on `8c31d63d04d48efeed9bcb3c28c0ec8286e00e0c`

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
- cursor 123,648
- late trace `811c9dc5`

Integrated frozen idle boot-to-stop baseline used by P6/P7/P8/P9/P10 and P11 inactive regression:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856; RMS 144/144; stereo delta 126

Frozen Phase-5 dry PCM:
- `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Source DSP and final output remain 44.1 kHz. Target Space/Cathedral remains the proven quarter-rate 11.025 kHz adaptation with one shared room and five source sends. Long room storage remains in dedicated pre-`.pool` NOLOAD memory.

## Frozen project persistence

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
- boot NOR prefetch runs before audio DMA startup
- later decode is RAM-only
- save request is only dirty intent
- erase/program is main-loop, transport-stopped/audio-quiesced work
- no flash write/erase path is reachable from `app_audio()`

## Frozen native visual contract

Ten deterministic 240×240 surfaces:
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
- reuses Dinghy `cv_px`; no second framebuffer
- 8-row native strips gated by `fm1_lcd_done()`
- display work remains outside `app_audio()`
- native takeover remains after runtime sample 8192
- all ten surfaces remain target-addressable

## Frozen physical-control contract

Pinned panel vocabulary:
- 14 buttons
- 7 relative/detent encoders
- 27 note keys
- MASTER remains platform volume
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

HOME is absolute. PLAY alone uses shared runtime transport; SEL+PLAY is CHAIN transport.

Important frozen gestures:
- CLOCK edits existing main/inner/snap/group structures
- GRAVITY degree requests use `apsis_runtime_request_gravity()`
- STRING uses three overlapping 9-key Study-126 source-keyboard banks across 27 keys
- SPIRAL route editing preserves ordered raw point ids `0..35`; duplicates remain legal
- BLOOM preserves petal/core/grow gestures
- KEEP/CHAIN use existing 8-slot/32-cell structures
- MOTHER structural edits retain `->1` pending/commit semantics
- SPACE edits the one shared room

Manual performance uses finite main→audio adapters/mailboxes only. It does not fabricate transport events or create a private scheduler/RNG.

## Phase 9 frozen target evidence

Technical head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- image 132,264 / 581,564 B
- RAM 89,492 / 98,304 B
- POOL 309,532 / 344,064 B
- package SHA-256 `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`

Historical M4f bypass remains exactly 95,804 B; its historical cursor/PCM gate is not relaxed.

## Phase 10 frozen advanced-interaction contract

Phase 10 added deliberately deferred interaction without reopening Phase-9 mappings:
1. Clock-quantized String Motion capture;
2. Classic/Dual ownership, overdub and frozen pitch context;
3. manual override/resume on the shared Clock timeline;
4. String Attractor editor over existing M4c authority;
5. Spiral Attractor editor over existing M4f state/runtime;
6. selected Gravity-point editor over the existing 24-point grid.

No private scheduler, musical RNG or duplicate musical state was introduced.

### String Motion frozen behavior

- REC arms/cancels at next Clock step 1; SEL+REC toggles playback when a loop exists
- 1/2/4/8-cycle capture remains source-bounded
- Classic owns single/right lane; linked Dual owns BOTH; free Dual can own BODY/WIRE/BOTH
- free-Dual overdub preserves unowned lane
- captured pitch context remains frozen against later Gravity/Tuning edits
- held String performance temporarily overrides playback and releases back at current phase
- shared M3c Clock remains sole timeline
- 1200 points/lane and 32-Hz capture bound are source-locked

Frozen P10 active lifecycle:
- seen `ff`; final `14`
- 149 points left / 149 right
- record start 220,672; finish 441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176
- cursor 481,024; queue high-water 30/512; overflow 0; late flags 0
- active WAV peak 1,171; RMS 173

### String/Spiral/Gravity editors

- String Attractor edits existing M4c gates/seed/regeneration authority only
- Spiral Attractor edits existing M4f attractor state/reset/runtime only; route state remains isolated
- Gravity Selected Point edits existing 24-point grid only; `SEL+degree` selects without audition, plain degree still follows normal request path

## Frozen Phase-10 production target

Final P10G production baseline:
- product `FM-1_972`
- image **140,152 / 581,564 B**
- RAM **89,684 / 98,304 B**; headroom 8,620 B
- POOL **328,740 / 344,064 B**; headroom 15,324 B
- package bytes 610,110
- package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- exact idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Phase 10 is JieLi-target/pinned-emulator verified and merged, **not physical-FM-1 verified**.

## Phase 11 method

Phase 11 is optimisation, not redesign.

Acceptance rule for every optimisation:
1. measured baseline;
2. explicit resource/timing target;
3. minimal patch;
4. before/after evidence on the same measurement path;
5. all relevant frozen musical/audio/control regressions green;
6. no baseline relaxation simply to make a patch pass.

Static/emulator proxies must be labelled as proxies. Do not infer physical ISR margin, stack high-water, LCD contention or NOR timing from emulator wall time.

## P11A — baseline inventory complete

Green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8` proved:
- final Phase-10 production sizes exactly regenerate
- `sizeof(ApsisRuntime) = 21,928 B`
- original `sizeof(ApsisStringMotionStore) = 19,208 B`
- Phase-9 → Phase-10 POOL delta is exactly 19,208 B
- Attractor/Gravity editors add no POOL allocation
- frozen idle PCM/runtime baseline remains exact

P11A explicitly did **not** claim:
- physical stack high-water
- physical `app_audio()` cycle/deadline margin
- hardware LCD DMA contention
- hardware NOR erase/program timing
- DAC/listening quality

## P11B — Motion storage proof complete

Green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c` proved before implementation:
- each captured point pitch is exactly derivable from one frozen context per lane + stored split
- free-Dual overdub can preserve independent contexts per lane
- signed-16-bit pitch compression is not lossless across the bounded String range
- source-locked 1200-point/lane ceiling and 32-Hz capture bound need not change

Projected store was 19,208 → 9,644 B, saving 9,564 B. P11C then measured that projection exactly on the real target.

## P11C — compact lossless Motion store complete

First complete green run: `41308798678` on `8c31d63d04d48efeed9bcb3c28c0ec8286e00e0c`.

Storage model:
- Motion point: 8 B -> 4 B (`phase_q16`, `split_q15`)
- frozen `ApsisStringMotionPitchContext`: 20 B once per lane
- Motion lane: 4,822 B
- Motion store: **9,644 B**
- Motion runtime: 88 B including bounded playback endpoint caches
- point capacity remains 1200/lane
- capture bound remains 32 Hz
- pitch remains 32-bit millisemintone at playback output

Playback implementation:
- one lane segment lookup now supplies both split and pitch interpolation position
- two derived endpoint pitches are cached per lane
- expensive quantization/tuning derivation happens when the sampled segment changes, not on every callback
- a new recording invalidates only the owned lane cache
- free-Dual overdub preserves the untouched lane/context bit-for-bit

Host gates:
- old Motion contract remains green
- compact playback matches a reference model across hold + continuous modes
- free-Dual overwrite/cache invalidation is covered
- ASAN/UBSAN green
- existing Motion DSP override green

### P11C measured production target

- product `FM-1_972`
- image **140,408 / 581,564 B**
- image headroom 441,156 B
- RAM **89,716 / 98,304 B**
- RAM headroom **8,588 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes 610,110
- package SHA-256 `e10c01525a808a4dea18d6b3f8777307bf4acf180c2cf958bebc4e1546b0d0be`

Delta vs frozen Phase 10:
- POOL **−9,564 B**
- POOL headroom **+9,564 B**
- image **+256 B**
- RAM **+32 B**

The complete measured POOL saving equals the P11B structural prediction byte-for-byte.

### P11C frozen idle regression

Still exactly:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue high-water 28/512
- overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856; RMS 144/144; stereo delta 126

### P11C active telemetry target

- image **137,672 B**
- RAM **89,764 B**
- POOL **319,176 B**
- package SHA-256 `d6e2d264d13d82c9cf93bce96f1d3b6666c6429cd5894fcf92db884b412bcb0e`

Active lifecycle remains exactly the Phase-10 reference:
- seen `ff`; final `14`
- left/right points 149/149
- record start 220,672
- record finish 441,088
- cycle 220,416 samples
- manual start 450,560
- manual resume 466,176
- cursor 481,024
- queue high-water 30/512
- overflow 0
- late flags 0
- active WAV peak 1,171; RMS 173

P11C therefore improves POOL headroom without changing the tested musical/runtime behavior.

## P11C evidence-retention correction

The first complete P11C run proved all gates, but the second `prepare_dinghy.sh` pass intentionally resets `work/` and `build/`; as a result the uploaded artifact retained active evidence but not the earlier production/idle JSON files. The workflow is being tightened to:
- store P11C evidence under `/tmp/p11c-evidence` across both target passes;
- assert exact production image/RAM/POOL/package hash;
- assert exact active image/RAM/POOL/package hash;
- upload production + idle + active evidence together.

This is CI evidence hygiene only; it does not change firmware.

## P11D — exact next optimisation step

P11D is **target memory ownership inventory**, not a RAM optimisation yet.

Goals:
1. build the current P11C production target with the real JieLi toolchain;
2. emit ELF/linker symbol inventory for `.data`, `.bss`, `.ram_text`, dedicated NOLOAD regions and largest static owners;
3. locate any statically configured stack reservation/bounds if the linker/SDK exposes them;
4. explicitly distinguish configured stack reservation from measured stack high-water;
5. use the inventory to choose the next RAM candidate only after ownership is clear.

Do not shrink a buffer, scheduler, voice pool or stack reservation merely because it is large. Any later change requires usage evidence and the normal Phase-11 acceptance rule.

## Exact next action

1. Run the tightened P11C workflow on the documentation/evidence-retention head and require all nine gates green on that exact SHA with a complete evidence artifact.
2. From that green head, create P11D memory-inventory instrumentation only.
3. Do not make a RAM optimisation until P11D identifies a concrete owner and safe target.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless re-approved by evidence
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- historical M4f bypass retains image 95,804 B
- integrated idle scenario retains cursor 123,392 with zero late/overflow flags
- audible idle PCM remains exact; dry-only remains exact Phase 5
- project persistence stays journaled/CRC-protected and never blocks active audio
- native visuals consume existing state; no UI shadow musical state
- native boot takeover remains after sample 8192 unless exact PCM is revalidated
- Phase-9 direct performance stays finite/mailboxed; no private scheduler/RNG
- Motion remains source-bounded at 1200 points/lane and 32 Hz capture
- P11C Motion store remains 9,644 B unless a later evidence-backed change is approved
- P10 Attractor/Gravity editors retain existing DSP/runtime/state authorities
- physical FM-1 and real ISR/storage/display/control behavior remain unverified
