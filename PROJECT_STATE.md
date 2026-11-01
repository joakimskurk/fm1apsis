# PROJECT_STATE

Last updated: 2026-11-01

## Current milestone

**Phase 11 — FM-1 optimisation is active. P11A baseline inventory, P11B Motion-storage proof, P11C compact lossless Motion storage, and P11D real-target memory ownership inventory are complete. P11E capacity/high-water stress inventory is next.**

Phase 10 merge authority:
- feature-complete technical head `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`
- exact-head documentation/CI gate `ce5857a52c6326011e5af7cd7f73819b420550fc`
- 22/22 required workflows PASS on that exact head
- PR #20 squash merge `378d2023df2afe59475be53c09cc001788586514`

Phase 11 evidence authority:
- P11A green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8`
- P11B green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c`
- P11C exact-head green run `41318100472` on `7ce6993065186aa66c89bc723ba3edfa95448596`
- P11D green technical run `41320007779` on `2e1912f5d0ff8987382bd7af434fc322679bf762`
- P11D evidence artifact SHA-256 `81df7625e1f8421d2d469083276b47f0cb77fae99e86a3a25367d19969b68f06`

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
- shared event heap 512 events
- canonical queue high-water 352/512; extreme 357/512
- Spiral slots 59/64; Bloom 44/64 in existing host stress
- random trace `78771ff5`
- late trace `0e6fc422`

Historical M4/M5 pinned emulator baseline:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

Integrated frozen idle baseline used by P6/P7/P8/P9/P10/P11:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856; RMS 144/144; stereo delta 126

Frozen Phase-5 dry PCM:
- `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Source DSP and final output remain 44.1 kHz. Target Space/Cathedral remains the proven quarter-rate 11.025-kHz adaptation with one shared room and five source sends. Long room storage remains in dedicated pre-`.pool` NOLOAD memory.

## Frozen persistence contract

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
- save request is dirty intent only
- erase/program runs from main loop with transport stopped/audio quiesced
- no flash write/erase path is reachable from `app_audio()`

## Frozen native visual/control contract

Ten deterministic 240×240 surfaces remain authoritative:
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

Display invariants:
- reuse Dinghy `cv_px`; no second framebuffer
- 8-row native strips gated by `fm1_lcd_done()`
- display work outside `app_audio()`
- native takeover after runtime sample 8192

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

Manual performance uses finite main→audio adapters/mailboxes only. No private scheduler or musical RNG is permitted.

## Phase 10 frozen advanced-interaction contract

Phase 10 added:
1. Clock-quantized String Motion capture;
2. Classic/Dual ownership, overdub and frozen pitch context;
3. manual override/resume on shared Clock timeline;
4. String Attractor editor over existing M4c authority;
5. Spiral Attractor editor over existing M4f authority;
6. selected Gravity-point editor over the existing 24-point grid.

No private scheduler, musical RNG or duplicate musical state was introduced.

String Motion frozen behavior:
- REC arms/cancels at next Clock step 1; SEL+REC toggles playback when a loop exists
- 1/2/4/8-cycle capture remains source-bounded
- Classic owns single/right lane; linked Dual owns BOTH; free Dual can own BODY/WIRE/BOTH
- free-Dual overdub preserves unowned lane
- captured pitch context remains frozen against later Gravity/Tuning edits
- held String performance temporarily overrides playback and releases back at current phase
- 1,200 points/lane and 32-Hz capture bound are source-locked

Frozen active Motion lifecycle:
- seen `ff`; final `14`
- 149 points left / 149 right
- record start 220,672; finish 441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176
- cursor 481,024
- queue high-water 30/512; overflow 0; late flags 0
- active WAV peak 1,171; RMS 173

## Frozen Phase-10 production baseline

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

Static/emulator proxies must be labelled as proxies. Do not infer physical ISR margin, runtime stack high-water, LCD contention or NOR timing from emulator wall time or linker reservations.

## P11A — baseline inventory complete

Green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8` proved:
- final Phase-10 production sizes exactly regenerate
- host `sizeof(ApsisRuntime) = 21,928 B`
- original MotionStore = 19,208 B
- complete P9→P10 POOL growth = MotionStore
- Attractor/Gravity editors add no POOL allocation
- frozen idle PCM/runtime remains exact

## P11B — Motion storage proof complete

Green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c` proved:
- every captured point pitch is exactly derivable from one frozen context/lane + stored split
- free-Dual overdub preserves independent lane contexts
- signed-16-bit pitch compression is not lossless
- source-locked 1,200-point/lane and 32-Hz bounds need not change

Projected MotionStore: 19,208 → 9,644 B. P11C measured that prediction exactly.

## P11C — compact lossless Motion store complete

Exact-head authority:
- head `7ce6993065186aa66c89bc723ba3edfa95448596`
- run `41318100472`: all 9/9 P11C gates PASS
- artifact includes production target/SHA, idle report/log, active target/SHA, active report/log/WAV

Storage model:
- Motion point 8 B -> 4 B (`phase_q16`, `split_q15`)
- one 20-B frozen pitch context per lane
- Motion lane 4,822 B
- Motion store **9,644 B**
- Motion runtime 88 B including bounded endpoint caches
- point capacity still 1,200/lane
- capture bound still 32 Hz
- playback output still signed 32-bit millisemintone pitch

Measured P11C production target:
- image **140,408 / 581,564 B**
- RAM **89,716 / 98,304 B**; headroom **8,588 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes 610,110
- package SHA-256 `e10c01525a808a4dea18d6b3f8777307bf4acf180c2cf958bebc4e1546b0d0be`

Delta vs frozen Phase 10:
- POOL **−9,564 B**
- POOL headroom **+9,564 B**
- image **+256 B**
- RAM **+32 B**

Frozen idle and active Motion lifecycle remain exactly unchanged.

## P11D — real-target memory ownership inventory complete

Technical authority:
- green run `41320007779`
- technical head `2e1912f5d0ff8987382bd7af434fc322679bf762`
- evidence artifact SHA-256 `81df7625e1f8421d2d469083276b47f0cb77fae99e86a3a25367d19969b68f06`

P11D changes no firmware memory size. It regenerates the exact P11C target and inventories real JieLi ELF/linker ownership.

### Ordinary RAM accounting

Real ELF sections:
- `.data` 12 B at `0x01c08000`
- `.bss` 89,700 B at `0x01c08010`
- 4-B alignment gap between them
- `.data` start → `.bss` end = **89,716 B**, exactly Dinghy's RAM metric

Merged sized-symbol intervals cover **89,658 B**. The remaining **58 B** is not free RAM; it can be alignment, linker-generated bytes or storage not represented by unique sized symbols.

Largest ordinary RAM symbols:
1. `bloom_dsp` **22,864 B**
2. `apsis_runtime` **21,128 B on target ABI**
3. `string_transient_pool` **7,188 B**
4. `bloom_audio_runtime` **6,424 B**
5. `spiral_audio_runtime` **4,792 B**
6. `clock_dsp` 4,364 B
7. `spiral_dsp` 4,124 B
8. `abuf` 4,096 B
9. `ua_ring` 2,048 B
10. `string_attractor_runtime` 2,028 B

The top five account for **62,396 B**, about **69.55%** of the ordinary RAM span.

Host `sizeof(ApsisRuntime) = 21,928 B` remains authoritative for the host ABI. The real target ELF symbol is 21,128 B. These are distinct ABI/layout measurements and must not be conflated.

### Special/dedicated RAM ownership

Largest special owners:
- `apsis_space_target_echo` 211,680 B in `.apsis_space_pool`
- `apsis_space_target_fdn` 38,340 B in `.apsis_space_pool`
- `cv_px` 30,720 B in `.pool`
- `apsis_space_target_history` 24,696 B in `.apsis_space_pool`
- `apsis_string_motion_store` **9,644 B in `.apsis_motion_pool`**
- Space input/source 2,048 B each

POOL reconciliation:
- `.apsis_motion_pool` 9,644 B
- `.apsis_space_pool` 278,812 B
- `.pool` 30,720 B
- total **319,176 B**, exactly P11C POOL metric

This independently confirms the compact MotionStore's real target ownership.

### Static configured stack ranges

Generated linker bounds:
- user stack `_ustack_lo.._ustack_top`: **24,320 B configured**
- 256-B guard before user stack
- system stack `_sstack_lo.._sstack_top`: **7,936 B configured**
- 256-B guard before system stack

These configured stack ranges live after the POOL region and before NOINIT. They are outside both the 96-KiB ordinary RAM metric and the 319,176-B POOL metric.

**P11D does not measure runtime stack high-water. No stack reduction is justified by P11D.**

### P11D decision

P11D identifies ownership, not overcapacity. Therefore do **not** shrink `bloom_dsp`, `apsis_runtime`, String/Bloom/Spiral pools or configured stacks simply because they are large.

The next phase must measure actual bounded-capacity usage first.

## P11E — exact next phase: capacity/high-water stress inventory

P11E is measurement only. It must not reduce capacity yet.

Primary targets:
1. shared M3c event heap / queue high-water under deterministic combined stress;
2. Bloom DSP/occurrence/voice capacity, including steals/drops where the implementation exposes them;
3. String transient logical/physical pool high-water, steals and drops;
4. Bloom manual/audio queue pressure;
5. Spiral audio/runtime slot pressure;
6. String Attractor pending/runtime pressure;
7. other large bounded owners only when safe counters or test-only telemetry can observe them without changing scheduling.

P11E should include both:
- realistic worst-case musical scenarios; and
- explicit saturation probes, clearly labelled artificial.

Acceptance rule:
- preserve production capacities and behavior;
- use existing counters where possible;
- add test-only telemetry where necessary;
- record high-water/steal/drop/overflow evidence reproducibly;
- do not claim physical CPU timing from emulator wall time;
- choose a P11F size reduction only if a convincing safety margin exists.

## Exact next action

Promote the completed P11D read-only instrumentation/docs to `p11-optimization`, then create a P11E staging branch from that head. Start by inventorying the existing capacity counters and struct limits in M3c, Bloom, String transient, Spiral and String Attractor before writing any new telemetry.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output remain 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless evidence explicitly re-approves a change
- host `ApsisRuntime` remains 21,928 B unless explicitly re-approved
- long Space storage remains in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- historical M4f bypass retains image 95,804 B
- integrated idle scenario retains cursor 123,392 with zero late/overflow flags
- audible idle PCM remains exact; dry-only remains exact Phase 5
- persistence remains journaled/CRC-protected and never blocks active audio
- native visuals consume existing state; no UI shadow musical state
- native boot takeover remains after sample 8192 unless exact PCM is revalidated
- direct performance remains finite/mailboxed; no private scheduler/RNG
- Motion remains source-bounded at 1,200 points/lane and 32 Hz capture
- P11C MotionStore remains 9,644 B unless later evidence explicitly re-approves a change
- P10 Attractor/Gravity editors retain existing DSP/runtime/state authorities
- no bounded pool is reduced without measured high-water/steal/drop evidence
- physical FM-1 ISR timing, runtime stack high-water, storage/display/control timing and listening quality remain unverified
