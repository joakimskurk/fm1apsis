# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

Physical FM-1 is not currently available. Nothing below is level 6.

## Pinned baseline

- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`
- target image budget 581,564 B
- RAM budget 98,304 B
- POOL budget 344,064 B
- source/output rate 44.1 kHz, 256-frame callback

## Frozen runtime/audio regressions

M3c host runtime:
- `sizeof(ApsisRuntime)=21,928 B`
- queue 352/512 canonical; 357/512 extreme
- Spiral slots 59/64; Bloom slots 44/64
- random `78771ff5`; late `0e6fc422`

Historical M4/M5 target scenario:
- cursor **123,648**
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Historical M4f Bloom-bypass:
- image **95,804 B**
- RAM 58,056 B
- POOL 30,720 B
- historical PCM/cursor gate remains frozen and must not move because of later-phase code

Integrated P6/P7/P8/P9 idle target scenario:
- cursor **123,392**
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Frozen Phase-5 dry PCM:
`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Frozen Phase-6/7/8/9 idle audible PCM:
`c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

## Phase 7 storage verification retained

`p7-storage.yml` verifies CRC-32/IEEE, explicit v1 encode/decode, transient-state exclusion, corruption rejection, interrupted saves, newest-generation selection, fallback, generation wraparound, fake-NOR save/reboot/recovery, commit failure handling and ASAN/UBSAN.

Target guards prove project prefetch before audio startup, RAM-only later decode, dedicated sectors at `0x000F4000` / `0x000F5000`, and no erase/program/save service reachable from `app_audio()`.

These are deterministic/fake-NOR and target-build tests, not physical power-cut verification.

## Phase 8 visual verification retained

`p8-visuals.yml` verifies normal and ASAN/UBSAN rendering for all ten surfaces.

Frozen visual references:
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

Generated-target guards retain 8-row DMA-gated rendering outside `app_audio()`, no second framebuffer, POOL 309,532 B and native takeover only after runtime sample 8192.

This is a source-derived deterministic visual contract, not a claim of screenshot-identical browser pixels.

## Phase 9 deterministic control verification

Phase-9 host workflows verify normal and ASAN/UBSAN behavior for:
- global panel/navigation and SEL modifiers
- CLOCK mask/group editing
- GRAVITY request/ring selection
- source 3×9 STRING keyboard mapping, octave-window overlap, source nudge and source parameter bounds
- ordered raw-point SPIRAL append/insert/move/delete behavior
- BLOOM petal/core/grow thresholds and parameter intents
- KEEP/CHAIN editing and transport intents
- MOTHER live mixing / structural sync
- one shared SPACE/CATHEDRAL controls
- DRIFT/TUNING controls
- MOTHER-aware structural apply/queue/commit bridge
- unified physical dispatcher
- project-dirty coalescing
- direct String main→audio mailbox and manual acoustic topology
- direct Bloom mailbox and manual petal/grow/core acoustic/late-binding topology

Generated-target checks verify pinned button/encoder ids and preserve the activity-gated idle path. Control code cannot create a module-private clock or own a separate musical state graph.

## Phase 9 direct performance verification

STRING:
- manual key path uses source keyboard level 0.080
- Classic BODY+WIRE and Dual side-only topology are deterministic host-tested
- 30-ms / 1323-sample debounce is tested
- one-byte producer/consumer handoff is tested
- historical M4b source/test blobs are hard-hash-guarded before target generation

BLOOM:
- direct petal and grow have separate source gain laws
- queued/pending Gravity display-harmony behavior is host-tested
- manual core advances one source voicing plan using supplied shared RNG draws
- core petals retain source UNFURL target geometry and bind at existing native Bloom lead timing
- four concurrent core snapshots / twenty pending petals are capacity-tested
- main-loop producer uses a packed 16-bit request; target wraps producer RMW in a short IRQ guard
- audio callback is sole owner of Bloom manual queue/DSP mutation

The finite manual Bloom queue owns gesture target times only; it is not a musical scheduler.

## Final technical Phase-9 candidate

Firmware source head: `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`.

Real JieLi target:
- product `FM-1_972`
- image **132,264 / 581,564 B**
- RAM **89,492 / 98,304 B**
- RAM headroom **8,812 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package 610,110 B
- package SHA-256 `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`

Pinned emulator, idle control scenario:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — **bit-exact P6/P7/P8**
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`
- dynamic HOME -> MOTHER PASS

## Technical-head Phase-9 matrix

On `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- APSIS core #503 — PASS
- APSIS staged validation #492 — PASS
- APSIS Phase 6 final checkpoint #95 — PASS
- APSIS Phase 7 storage #82 — PASS
- APSIS Phase 7 target persistence #75 — PASS
- APSIS Phase 8 visuals #85 — PASS
- APSIS Phase 8 target visuals #60 — PASS
- APSIS Phase 9 controls #82 — PASS
- APSIS Phase 9 H4b Bloom performance #6 — PASS
- APSIS Phase 9 target controls #40 — PASS

The final documentation-only head is validated separately before PR #19 merge so this file does not create a self-referential CI/run-number loop.

## Regression caught during Phase 9

The first H4a manual-String implementation added helper code to the historical M4b String Rhythm source. That changed the frozen M4f bypass target image from 95,804 to 95,820 B.

The project did not accept the new historical size. Manual String was moved into a P9-only module and the M4b `.c/.h/test` blobs were restored exactly. The P9 generator now hashes and rejects any later contamination of those frozen authorities. Staged Gate 5b is again PASS at exactly 95,804 B.

## Current verification boundary

Phase 9 is verified through level 5 for the tested idle target scenario: implementation, desktop deterministic tests, sanitizers, real JieLi target build and pinned emulator.

Still unverified until physical hardware exists:
- real control feel/ergonomics
- actual switch/encoder/LED/display behavior
- real ISR-cycle margin under active manual String/Bloom performance
- actual DAC/audio listening quality
- real LCD DMA/IRQ coexistence under worst-case interaction
- real NOR erase/program timing and power interruption
- worst-case combined UI/control/audio load

## Next verification — Phase 10

Add deterministic host/sanitizer contracts for String Motion capture/ownership/overdub and advanced String/Spiral Attractor + Gravity-point editors before target integration. Phase-10 target work must retain the frozen Phase-9 idle PCM/runtime and must not reopen Phase-9 mappings without explicit evidence.
