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

## Phase 3 / M3a current branch

Active branch: `m3-transport-core`.
Draft PR: #4.

Implemented:
- sample-referenced shared transport
- fixed 256-event min-heap
- long-running whole-sample + Q32 fractional clock
- event generation and transport regression test source
- new transport suite wired into root `Makefile`

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

### Current M3a verification state

GitHub Actions `APSIS core` run #49 on commit `338d8fb863fab31951f2077382c4e692ef395f6e`:
- source-derived core regression: PASS
- `tests/test_transport.c`: PASS as part of the root test target
- ASAN/UBSAN: PASS

Subsequent commits at handoff are documentation-only and do not alter the transport implementation.

Current verification levels:
- implemented: YES
- committed: YES
- desktop/core regression: PASS
- sanitizer: PASS
- real target build **with transport compiled into overlay**: NO
- emulator **with transport compiled into overlay**: NO
- physical FM-1: NO

Important: the staged Phase-1 workflow can still build the old overlay successfully before transport is included. Such a pass is a regression check for the existing shell, **not** target verification of `apsis_transport.[ch]`.

Reason target/emulator are not yet valid for M3a: `apsis_transport.[ch]` is not yet copied by `ci/apply_apsis.sh` or included in `overlay/dinghy.c`.

## Required validation before M3a merge

1. add transport sources to `ci/apply_apsis.sh`
2. include transport in `overlay/dinghy.c`
3. rerun core regression + sanitizer
4. rerun complete staged CI
5. confirm real JieLi target compiles the transport in the single translation unit
6. record image/RAM/pool deltas and package SHA
7. inspect emulator smoke
8. update `PROJECT_STATE.md` and this file
9. only then mark PR #4 ready and merge

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
