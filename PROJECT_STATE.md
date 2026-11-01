# PROJECT_STATE

Last updated: 2026-11-01

## Current milestone

**Phase 10 — String Motion and advanced interactive parity is feature-complete on technical head `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`. PR #20 is at the final exact-head documentation/CI merge gate. Phase 11 — FM-1 optimisation is the next development phase after that gate.**

Phase-9 freeze remains authoritative:
- technical firmware head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`
- final documentation/CI head `dc288b6424b957ad382997f62cd6924b4146f675`
- merge commit `bbfab322557b6760425503268b550b789f796c23`
- target image 132,264 B; RAM 89,492 B; POOL 309,532 B
- package `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`

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

Integrated frozen idle boot-to-stop scenario used by P6/P7/P8/P9 and Phase-10 inactive-editor regression:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856; RMS 144/144; stereo delta 126

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

Historical M4f bypass remains exactly **95,804 B**; its historical cursor/PCM gate is not relaxed.

## Phase 10 completed scope

Phase 10 layers advanced interaction deliberately deferred from Phase 9 without reopening frozen Phase-9 mappings:
1. Clock-quantized String Motion capture;
2. Classic/Dual ownership, overdub and frozen pitch context;
3. manual override/resume on the shared Clock timeline;
4. String Attractor editor over existing M4c authority;
5. Spiral Attractor editor over existing M4f state/runtime;
6. selected Gravity-point editor over the existing 24-point grid;
7. host/sanitizer contracts before target integration;
8. real JieLi builds, exact frozen idle regression and active pinned-emulator lifecycles.

No Phase-10 editor introduces a private scheduler, private musical RNG or duplicate musical state.

## P10D3 — String Motion complete

Technical checkpoint head: `e750655141bb87e83949bb262426ab2ee42d734b`.

Contract:
- REC arms/cancels at the next Clock step 1; SEL+REC toggles playback when a loop exists
- 1/2/4/8-cycle capture remains source-bounded
- Classic owns the single/right lane; linked Dual owns BOTH; free Dual can own BODY/WIRE/BOTH
- overdub preserves the unowned side in free Dual
- captured pitch context is frozen against later Gravity changes
- held String performance temporarily overrides playback and releases back into the loop at current phase
- capture/playback observes shared M3c Clock only; no private scheduler
- compact main→audio mailbox; audio callback owns Motion runtime/store mutation
- two Motion lanes live in dedicated `.apsis_motion_pool` NOLOAD memory, exactly **19,208 B** total

Production P10D2 target:
- image **136,984 / 581,564 B**
- RAM **89,668 / 98,304 B**; headroom **8,636 B**
- POOL **328,740 / 344,064 B**; headroom **15,324 B**
- package SHA-256 `25f87a83b39b48e9c1f11826741d9bc4da45b3fe1783e5c4041020ffcd4bf55b`

Active P10D3 lifecycle:
- seen flags `ff`; final flags `14`
- Dual+LINK capture 149 points left / 149 right
- record start 220,672; finish 441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176
- cursor 481,024; queue high-water 30/512; overflow 0; late flags 0
- active WAV peak 1,171; RMS 173

## P10E — String Attractor editor complete

Editor semantics:
- ordinary Phase-9 MOTHER `SEL+ENV` mute gesture remains frozen
- enter STRING with ENV, then `SEL+ENV` enters/exits String Attractor edit context
- direct key gate toggles edit existing active attractor cells only
- REC reroll changes the existing bounded seed and regenerates through existing M4c authority
- existing renderer/state are reused; no new DSP/runtime authority

Production target:
- image **137,304 B**
- RAM **89,668 B**
- POOL **328,740 B**
- inactive editor preserves exact frozen PCM and cursor 123,392

Active emulator:
- seen flags `3f`
- gate `9535 -> 9534`
- seed `317 -> 8236`
- zero violations, zero Motion leakage, zero overflow, zero late flags

## P10F — Spiral Attractor editor complete

Editor semantics:
- ALGORITHM selects the existing Spiral ATTRACTOR page without replacing frozen P9 ROUTE behavior outside that page
- gate/length/rate/note-length/energy edits act on existing Spiral attractor state
- REC reroll calls existing `apsis_spiral_reset_attractor(..., 1)` authority
- route_count/hash remain isolated from attractor editing
- existing P8 Spiral attractor visual/runtime position are reused

Deterministic reset note:
- with default length 16 and seed reroll 911 -> 4825, source reset vector is **`0x002DA5FD`**
- final gate masks are not treated as static after transport runs because M3c legitimately evolves Spiral attractor gates when lock < 1.0

Production target:
- image **138,840 B**
- RAM **89,684 B**
- POOL **328,740 B**
- package SHA-256 begins `593ae412`
- inactive editor preserves exact frozen PCM and cursor 123,392

Active emulator:
- seen flags `3f`
- seed `911 -> 4825`
- route count `0 -> 0`, route hash unchanged
- queue high-water 28; overflow 0; late flags 0

## P10G — Gravity selected-point editor complete

Feature-complete technical head: `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`.

Editor semantics:
- ALGORITHM cycles Harmony / Timing / Selected Point
- on Selected Point, `SEL+degree` chooses one of 24 existing points without sending a normal Gravity audition/request
- K1 offset; K2 octave; K3 level; K4 brightness; SELECT attack; PRESET release; REC enabled
- only the `SEL+degree` note bits are filtered before the frozen P9 Gravity dispatcher
- plain degree keys still use normal `apsis_runtime_request_gravity()` behavior
- edits are live existing project state; no per-knob flash write or new persistence mechanism

Final stacked Phase-10 production target on P10G:
- product `FM-1_972`
- image **140,152 / 581,564 B**
- RAM **89,684 / 98,304 B**; headroom **8,620 B**
- POOL **328,740 / 344,064 B**; headroom **15,324 B**
- package bytes 610,110
- package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- exact frozen idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

P10G active telemetry target:
- image **140,712 B**
- RAM **89,764 B**
- POOL **328,740 B**
- package SHA-256 `21df63ad9bd785e8415ef91a9389e10888db67c983f98c11da51f4e266bc9ed9`

P10G active emulator:
- seen flags `7f`; violations 0
- selected ANCHOR degree 4 without audition
- offset 0 -> 1; octave 0 -> 1
- level q15 24903 -> 25559; brightness q15 7864 -> 7536
- attack 58 -> 66 ms; release 990 -> 1040 ms; enabled 1 -> 0
- other 23 points remain stable (`other_points_hash=23b8dbda`)
- later plain degree 6 produces the normal Gravity request
- cursor 176,384; queue high-water 29; overflow 0; late flags 0

## Phase 10 verification boundary

All four advanced-interaction layers are now verified at the strongest level currently possible without physical hardware:
- deterministic host contracts
- ASAN/UBSAN
- generated target ownership/guard checks
- real JieLi AC79 target builds
- exact inactive-editor regression against the same frozen Phase-9 PCM/runtime baseline
- active pinned-emulator control lifecycles

Phase 10 is **not physical-FM-1 verified**.

Still unverified and intentionally deferred to Phase 11/12:
- real ISR margin/timing under combined DSP + UI + control load
- physical control feel, debounce and encoder/switch behavior
- actual LEDs/LCD transfer behavior on hardware
- DAC/listening quality and Cathedral tuning on hardware
- real NOR timing/power interruption
- worst-case combined control/UI/audio stress on device

## Phase 11 entry rule

Phase 11 is optimisation, not redesign. Begin from measured evidence before changing arithmetic, memory ownership or scheduling. Preserve all Phase-10 musical/control behavior and frozen regression gates unless a change is explicitly re-approved by evidence.

Initial optimisation targets:
1. establish real/target-side ISR and callback cost measurements that can be obtained without physical hardware, while clearly separating emulator/toolchain proxies from hardware timing;
2. inventory RAM, POOL, stack and image headroom with the final Phase-10 stacked target;
3. identify proven-safe memory/arithmetic reductions, especially around Motion storage and UI redraw work;
4. stress shared queue/voice limits and redraw load without creating private schedulers or relaxing audio baselines;
5. leave physical Cathedral listening/tuning and true ISR margin claims for hardware validation.

## Exact next action

Run the **final exact-head CI matrix on this Phase-10 documentation head**. Merge PR #20 with squash only if every required core/staged/P6/P7/P8/P9/P10 host, target and active workflow is green on that same head. After merge, create the Phase-11 optimisation branch from `main` and start with measurement/budget inventory before making optimisation changes.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral remains the proven quarter-rate adaptation unless re-approved by evidence
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- historical M4/M5 gates retain cursor 123,648
- historical M4f bypass retains image 95,804 B
- integrated inactive-editor scenario retains cursor 123,392 with zero late/overflow flags
- audible idle PCM remains exact P6/P7/P8; dry-only remains exact Phase 5
- project persistence stays versioned/journaled/CRC-protected and never blocks active audio
- native visuals consume existing APSIS state; no UI shadow musical state
- native boot takeover remains after sample 8192 unless revalidated against exact PCM
- Phase-9 direct performance uses finite mailboxes/adapters; no private scheduler/RNG
- Phase-10 Motion store remains the bounded 19,208-B dedicated POOL allocation; no heap/private scheduler
- Phase-10 Attractor/Gravity editors keep existing DSP/runtime/state authorities; no duplicate musical state
- physical FM-1 and real ISR/storage/display/control behaviour remain unverified
