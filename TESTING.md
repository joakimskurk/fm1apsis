# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

## Toolchain / platform baseline

Pinned Dinghy:
- commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`

Target product identity:
- `FM-1_972`

Current target platform constraints used by CI:
- app image budget: 581,564 B
- RAM `.data + .bss` budget: 98,304 B
- pool budget: 344,064 B
- 44.1 kHz / 256-frame callback baseline

## Phase 1

Status: **verified through emulator**.

Verified:
- pinned source identity
- unchanged Dinghy host build
- unchanged Dinghy target build
- APSIS overlay host build
- APSIS target packaging / product identity
- FM-1 emulator boot/input/silent-audio smoke

Physical FM-1: **not tested**.

## Phase 2 / M2a + M2b + M2c

Status: **verified through emulator**.

Automated desktop suites:
- `tests/test_apsis_core.c` - PASS
- `tests/test_gravity_bloom.c` - PASS
- `tests/test_keep_chain_mother.c` - PASS

Sanitizers:
- ASAN - PASS
- UBSAN - PASS

M2c full staged validation on PR #3:
- source core and pinned upstream - PASS
- unchanged Dinghy host and target - PASS
- APSIS overlay and host shell - PASS
- APSIS real JieLi target package - PASS
- FM-1 emulator smoke - PASS

M2c target evidence:
- app image: 56,700 B / 581,564 B
- RAM `.data + .bss`: 14,104 B / 98,304 B
- RAM headroom: 84,200 B
- pool: 30,720 B / 344,064 B
- pool headroom: 313,344 B
- package: `apsis-0.1-dev.fwsc`
- package bytes: 610,110 B
- product: `FM-1_972`
- package SHA-256: `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`

Emulator M2c:
- boot/input smoke PASS
- WAV stereo channels: 2
- WAV frames: 124,928
- WAV peak: 0, expected because Phase 2 is intentionally silent

Physical FM-1: **not tested**.

## Phase 3 / M3a

Branch: `m3-transport-core`.
PR: #4.
Verified implementation commit: `17b073e4174d7d27144d64a455e4080b5b777ea3`.

Status: **verified through emulator**.

Implemented:
- sample-referenced shared transport
- fixed 256-event min-heap
- long-running whole-sample + Q32 fractional clock
- event generation and transport regression test source
- transport suite wired into root `Makefile`
- transport sources copied into the Dinghy overlay
- transport header/source included in `overlay/dinghy.c` single translation unit

New suite:
- `tests/test_transport.c`

Coverage includes:
- source-derived String timing-state defaults
- default 72 BPM / 1/8 interval at 44.1 kHz
- event ordering on Clock 1
- step-one resync precedence
- Gravity boundary flagging
- alternating swing pair with no pair drift
- String rhythm +26 ms offset
- pulse diagnostic suppression
- snap +4 ms
- inner/between at half interval
- deterministic ghost event
- Bloom cycle/rate/delay occurrence
- Gravity quantization division/boundary prediction
- long-running sample positions above 10 billion samples
- 12-pulse heap ordering stress with 4x Spiral and secondary events

### M3a desktop/sanitizer evidence

GitHub Actions `APSIS core` run #59 on implementation commit `17b073e4174d7d27144d64a455e4080b5b777ea3`:
- source-derived core regression: PASS
- `tests/test_transport.c`: PASS as part of the root test target
- ASAN: PASS
- UBSAN: PASS

### M3a staged target/emulator evidence

GitHub Actions `APSIS staged validation` run #49 on the same implementation commit:
- source core and pinned upstream: PASS
- unchanged Dinghy host and target: PASS
- APSIS overlay and host shell: PASS
- APSIS real JieLi target package: PASS
- FM-1 emulator smoke: PASS
- evidence artifact upload: PASS
- development firmware artifact upload: PASS

Target evidence:
- app image: 56,680 B / 581,564 B
- image delta vs M2c: **-20 B**
- RAM `.data + .bss`: 14,104 B / 98,304 B
- RAM delta vs M2c: **0 B**
- RAM headroom: 84,200 B
- pool: 30,720 B / 344,064 B
- pool delta vs M2c: **0 B**
- pool headroom: 313,344 B
- package: `apsis-0.1-dev.fwsc`
- package bytes: 610,110 B
- product: `FM-1_972`
- package SHA-256: `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`

Emulator M3a:
- boot/input/silent-audio contract: PASS
- WAV stereo channels: 2
- WAV frames: 124,928
- WAV peak: 0, expected because M3a is intentionally silent
- screen SHA-256 @ 500 ms: `0db73c83747db6ba5d8b6a5174d9d08260b6553be44c09c924e43088e3ddfed7`
- screen SHA-256 @ 1100 ms: `dc5891507c9e99893a1b61df0ddbeec8ff57c6c93f3c41b71864f09c71efd8fa`
- screen SHA-256 @ 2100 ms: `0db73c83747db6ba5d8b6a5174d9d08260b6553be44c09c924e43088e3ddfed7`

Current verification levels:
- implemented: YES
- committed: YES
- desktop/core regression: PASS
- sanitizer: PASS
- real target build with transport compiled into overlay: PASS
- emulator with transport compiled into overlay: PASS
- physical FM-1: NO

Resource interpretation: the target compiler now parses/compiles `apsis_transport.[ch]` inside the real single translation unit, which closes the M3a single-TU/target-compatibility gate. M3a deliberately does not yet instantiate or consume an `ApsisTransport` from `app_audio()`. Therefore the current 0 B RAM/pool delta is **not** a measurement of the eventual live 256-event queue. Record that footprint in M3b when the shared runtime transport is instantiated.

Physical FM-1: **not tested**.

## M3a merge gate

Completed:
1. transport sources added to `ci/apply_apsis.sh`
2. transport included in `overlay/dinghy.c`
3. core regression + sanitizer rerun and green
4. complete staged CI rerun and green
5. real JieLi target compiled the transport in the single translation unit
6. image/RAM/pool deltas and package SHA recorded
7. emulator smoke inspected and green
8. `PROJECT_STATE.md` / `TESTING.md` updated

Remaining:
9. ensure checks are green on the final documentation head, mark PR #4 ready, and squash-merge

## Release-level validation not yet performed

- physical FM-1 install
- OCT- rescue
- return to official firmware
- USB/MIDI interaction on device
- all keys/buttons/encoders/LEDs/MASTER
- real ISR worst-case timing
- five-source + 4x Spiral + Bloom + String Motion + Cathedral stress
- persistent KEEP/CHAIN reboot/recovery
- power interruption during flash write

Do not call any of these verified until actual hardware evidence exists.
