# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

## Baseline

Pinned Dinghy:
- commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`

APSIS identity:
- product `FM-1_972`

CI target budgets:
- app image: 581,564 B
- RAM `.data + .bss`: 98,304 B
- pool: 344,064 B
- sample rate: 44.1 kHz
- callback baseline: 256 frames

Physical FM-1 is not currently available. Nothing below is promoted to physical-hardware verification.

## Phase 1

Status: **verified through emulator**.

Verified:
- pinned source/upstream identity
- unchanged Dinghy host + real target
- APSIS overlay host build
- APSIS target packaging / `FM-1_972`
- FM-1 emulator boot/input/silent-audio smoke

## Phase 2 / M2a + M2b + M2c

Status: **verified through emulator and merged**.

Desktop suites:
- `tests/test_apsis_core.c`: PASS
- `tests/test_gravity_bloom.c`: PASS
- `tests/test_keep_chain_mother.c`: PASS
- ASAN: PASS
- UBSAN: PASS

Final M2c target evidence:
- image 56,700 B
- RAM 14,104 B
- pool 30,720 B
- package bytes 610,110 B
- package SHA-256 `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`
- emulator WAV peak 0 because Phase 2 is intentionally silent

## Phase 3 / M3a

Merged to `main`:
- PR #4
- merge `fe2911c43fd69e63839220482e50bb10c7a2e4c2`

Status: **verified through emulator and merged**.

Verified:
- shared sample-referenced transport
- whole-sample + Q32 fractional time
- source-derived Clock/swing/subdivision event grid
- Gravity boundary prediction
- long-running sample counter behavior
- fixed-capacity shared heap
- target single-translation-unit integration

Final M3a evidence:
- `APSIS core` #60: PASS
- staged validation #50: PASS
- image 56,680 B
- RAM 14,104 B
- pool 30,720 B
- package SHA-256 `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`
- emulator WAV peak 0

## Phase 3 / M3b

Merged to `main`:
- PR #5
- merge `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- implementation/evidence head `001d69a758369e2d14665ba2fd176c1fd116990b`
- documentation head `6a8a182cbd0559cc2164c78bf956fbba7e0ee919`

Status: **verified through emulator and merged**.

Verified runtime behavior:
- allocation-free `apsis_runtime.[ch]`
- live shared transport/heap in FM-1 overlay
- silent `app_audio()` timeline driver
- Gravity immediate/quantized pending behavior
- Mother horizon / next-1 commit and blocked-dispatcher retry
- Chain cell/chance/transpose/morph progression and frozen meter length
- Spiral attractor symbolic occurrences
- Bloom symbolic occurrences and voicing plans
- deterministic runtime random source
- target-safe arithmetic

Final M3b evidence:
- core #81 implementation head: PASS incl. ASAN/UBSAN
- staged #71 implementation head: PASS through real target/emulator
- core #84 documentation head: PASS
- staged #74 documentation head: PASS
- image 66,428 B
- RAM 22,552 B
- pool 30,720 B
- host runtime sizeof 8,456 B
- host queue high-water 14/256
- emulator live queue 7/256
- emulator queue overflow 0
- emulator cursor 123,648 low-32 samples
- package SHA-256 `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`
- emulator WAV peak 0

## Phase 3 / M3c

Branch `m3c-late-binding`, PR #6.
Base: M3b merge `a61a40dc83c9b52129f7d3f920da26a9a61837d9`.
Last code/test implementation head before status documentation: `05ddbe2c4bed26b91d9d5701aa0965aa2bba90d2`.

Status: **implementation complete; desktop/sanitizer and target-identical real-target/emulator evidence PASS; exact final documentation-head gate still required before merge**.

Normative references:
- `docs/reference/LATE_BINDING_CONTRACT.md`
- `docs/reference/M3C_REFERENCE_TRACE.md`

### M3c source-derived behavior covered

Spiral:
- occurrence route ordinal/trigger/energy identity frozen
- final sequence length and pitch reread at bind
- current register and global tuning reread at bind
- ordinary pending Gravity is not projected
- native bind lead 80 ms; source narrow-browser 110 ms retained as a test fact

Bloom:
- occurrence callback at target−45 ms
- occurrence freezes plan/register/random choice and unfurl geometry
- petal bind is `max(occurrence, voiceTarget−50 ms)` on native FM-1
- source narrow-browser 70 ms retained as a test fact
- ordinary pending Gravity may project only degree/ring under Study 126's +0.5 ms target rule
- Mother-held state is not projected
- every petal rereads target-aware Gravity and current tuning/depth
- frozen petal count is preserved across mid-unfurl DEPTH changes

Shared-state observations:
- normal Gravity commit target−18 ms
- Mother structural commit target−26 ms
- Chain discrete apply target−24 ms
- Chain continuous morph still begins at target
- all observations use one shared sample heap; no per-module clocks

Lifecycle:
- Stop cancels future scheduler/Spiral/Bloom/Chain/Mother shared-heap work
- Stop resets Spiral planner
- already-pending normal Gravity remains independent and can still commit
- restart begins a fresh shared transport at the current native cursor

State representation corrections found by M3c audit:
- Bloom unfurl is 0..3600 ms, default 420 ms; it is not normalized Q15
- `spiral_step` is 32-bit so long runs do not wrap at 256 gated notes

### M3c desktop + sanitizer evidence

Core workflow on implementation head:
- root musical/state suites: PASS
- `tests/test_late_binding.c`: PASS
- `tests/test_runtime.c`: PASS
- `tests/test_runtime_lifecycle.c`: PASS
- `tests/test_runtime_extreme.c`: PASS
- ASAN: PASS
- UBSAN: PASS

Canonical 30-second deterministic replay, seed `0x31415926`:
- host `sizeof(ApsisRuntime)`: **21,928 B**
- queue high-water: **352 / 512**
- Spiral slot high-water: **59 / 64**
- Bloom slot high-water: **44 / 64**
- random trace: **`78771ff5`**
- late-binding/state-observation trace: **`0e6fc422`**
- same-seed replay: identical
- different seed: changes at least one trace hash
- queue overflow: 0
- scheduler starvation: 0
- late-slot overflow: 0

Capacity decision evidence:
- the original 256-entry M3 queue **failed** legal M3c worst-case stress by setting `scheduler_starved`
- queue was therefore raised to 512 using measured evidence

60-second extreme burst:
- 180 BPM
- 1/4 division
- 35% swing
- maximal Clock secondary activity
- Spiral 4x, all gates
- Bloom 4x, ninth depth, 3.6 s unfurl
- 512-frame processing chunks

Extreme result, normal + ASAN/UBSAN:
- queue **357 / 512**
- Spiral slots **59 / 64**
- Bloom slots **44 / 64**
- queue overflow 0
- scheduler starvation 0
- late-slot overflow 0

### M3c real target evidence

A target-identical M3c implementation passed the complete staged validation. The only later change before the current documentation pass was host-only extreme-test wiring; target firmware source was unchanged.

Target result:
- image **71,608 B / 581,564 B**
- image headroom 509,956 B
- RAM `.data + .bss` **35,272 B / 98,304 B**
- RAM headroom **63,032 B**
- pool **30,720 B / 344,064 B**
- pool headroom **313,344 B**
- package `apsis-0.1-dev.fwsc`
- package bytes 610,110 B
- identity `FM-1_972`
- package SHA-256 **`854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`**

Deltas versus M3b:
- image: **+5,180 B**
- RAM: **+12,720 B**
- pool: **0 B**

The RAM increase is expected: the host runtime grew from 8,456 B to 21,928 B as the queue doubled and bounded late-bind slots/telemetry were added.

### M3c emulator evidence

FM-1 emulator contract: PASS.

Measured target-RAM/runtime telemetry:
- live queue high-water **28 / 512**
- queue overflow **0**
- late-slot/starvation flags **0**
- runtime cursor low 32 bits **123,648**
- late trace hash **`811c9dc5`**

Audio/UI smoke:
- stereo channels 2
- WAV frames 124,928
- WAV peak **0**, mandatory because M3c is intentionally silent
- screen @500 ms SHA `9773b7fe1b57d86cf8429c4bf7444ee21e395343d6d551ebafb08d901d6c5138`
- screen @1100 ms SHA `3bc9deae5050b2ead3f8b1310fc6fb1927d89c122ea71aef4a73a8b6434f5487`
- screen @2100 ms SHA `9773b7fe1b57d86cf8429c4bf7444ee21e395343d6d551ebafb08d901d6c5138`

### M3c verification levels now

- implemented: YES
- committed: YES
- source-derived regression: PASS
- deterministic long replay: PASS
- extreme capacity stress: PASS
- sanitizer: PASS
- real target build on target-identical firmware: PASS
- emulator on target-identical firmware: PASS
- physical FM-1: NO
- merged: NO

Before merge, run both workflows once more on the exact final documentation head. Do not begin Phase 4 DSP until that head is green and PR #6 is merged.

## Release-level validation not yet performed

- physical FM-1 install
- OCT- rescue
- return to official firmware
- USB/MIDI interaction on device
- all keys/buttons/encoders/LEDs/MASTER
- real ISR worst-case timing
- five-source + 4x Spiral + Bloom + String Motion + Cathedral stress with real DSP
- persistent KEEP/CHAIN reboot/recovery
- power interruption during flash write

Do not call any of these verified until actual hardware evidence exists.
