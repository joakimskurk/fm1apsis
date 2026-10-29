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
Merged to `main`: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`.

Status: **verified through emulator and merged**.

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

Final rebased PR head:
- `cb0319185a483ff2972aae104da6b2a86c202add`
- `APSIS core` run #60: PASS
- `APSIS staged validation` run #50: PASS through real target and emulator

M3a target evidence:
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

M3a verification levels:
- implemented: YES
- committed: YES
- desktop/core regression: PASS
- sanitizer: PASS
- real target build with transport compiled into overlay: PASS
- emulator with transport compiled into overlay: PASS
- physical FM-1: NO

Physical FM-1: **not tested**.

## Phase 3 / M3b

Branch: `m3b-runtime-consumers`.
PR: #5.
Final implementation/evidence head before final documentation: `001d69a758369e2d14665ba2fd176c1fd116990b`.

Status: **implementation verified through emulator; merge pending final documentation-head checks**.

Implemented:
- dedicated allocation-free `apsis_runtime.[ch]` coordinator
- live `ApsisTransport` + 256-entry event heap instantiated in the FM-1 overlay
- `app_audio()` used only as the sample-clock/timeline driver; output remains intentionally silent
- deterministic same-sample structural event consumption
- Gravity immediate vs quantized pending commit
- Mother horizon / next-1 commit on the shared timeline
- Mother dispatcher failure preserves pending state and can recover on a later Clock-1 retry
- Chain start/loop/cell/chance/transpose/morph runtime progression
- Chain meter-length snapshot parity with browser `startChain()`
- Spiral attractor progression and symbolic occurrence capture
- Bloom occurrence and voice-plan capture while final pitch binding remains deferred to M3c
- deterministic runtime random source
- target-safe Q32 arithmetic without target-unavailable `__udivdi3`
- passive four-word target telemetry probe for emulator queue evidence

New suite:
- `tests/test_runtime.c`

Desktop/runtime coverage includes:
- event consumption and same-sample priority
- quantized Gravity invisibility before commit and visibility at commit
- Mother commit before dependent Bloom observation
- Mother blocked-dispatcher retry/recovery without losing pending state
- Chain start sync, advance, loop and morph behaviour
- frozen Chain meter length after start
- Spiral symbolic occurrence progression
- Bloom occurrence/voicing-plan scheduling
- bounded queue stress with simultaneous Clock/Spiral/String/Bloom/secondary events

### M3b desktop/sanitizer evidence

GitHub Actions `APSIS core` run #81 on `001d69a758369e2d14665ba2fd176c1fd116990b`:
- all root regression suites: PASS
- `tests/test_runtime.c`: PASS
- host `sizeof(ApsisRuntime)`: 8,456 B
- host stress queue high-water: **14 / 256**
- queue overflow in stress test: NO
- ASAN: PASS
- UBSAN: PASS

### M3b staged target/emulator evidence

GitHub Actions `APSIS staged validation` run #71 on the same head:
- source core and pinned upstream: PASS
- unchanged Dinghy host and target: PASS
- APSIS overlay and host shell: PASS
- APSIS real JieLi target package: PASS
- FM-1 emulator boot/input/silent-audio/runtime-queue smoke: PASS
- evidence artifact upload: PASS
- development firmware artifact upload: PASS

Target evidence:
- app image: 66,428 B / 581,564 B
- image delta vs M3a: **+9,748 B**
- image headroom: 515,136 B
- RAM `.data + .bss`: 22,552 B / 98,304 B
- RAM delta vs M3a: **+8,448 B**
- RAM headroom: 75,752 B
- pool: 30,720 B / 344,064 B
- pool delta vs M3a: **0 B**
- pool headroom: 313,344 B
- package: `apsis-0.1-dev.fwsc`
- package bytes: 610,110 B
- product: `FM-1_972`
- package SHA-256: `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`

Resource interpretation:
- the +8,448 B target RAM delta closely matches the 8,456 B host runtime object; this is now a measurement of the **live** transport/runtime/heap footprint, unlike M3a's compile-only 0 B delta
- no extra pool allocation is used

Emulator M3b:
- boot/input/silent-audio/runtime-queue contract: PASS
- target-RAM live queue high-water: **7 / 256**
- target-RAM queue overflow flag: **0**
- runtime cursor low 32 bits after smoke: **123,648**, proving the callback-driven timeline advanced
- WAV stereo channels: 2
- WAV frames: 124,928
- WAV peak: 0, required because M3b remains intentionally silent
- screen SHA-256 @ 500 ms: `9773b7fe1b57d86cf8429c4bf7444ee21e395343d6d551ebafb08d901d6c5138`
- screen SHA-256 @ 1100 ms: `3bc9deae5050b2ead3f8b1310fc6fb1927d89c122ea71aef4a73a8b6434f5487`
- screen SHA-256 @ 2100 ms: `9773b7fe1b57d86cf8429c4bf7444ee21e395343d6d551ebafb08d901d6c5138`

M3b verification levels:
- implemented: YES
- committed: YES
- desktop/core regression: PASS
- sanitizer: PASS
- real target with live runtime instantiated: PASS
- emulator with live runtime instantiated: PASS
- live target-RAM queue high-water measured: PASS
- physical FM-1: NO

M3b deliberately does **not** implement DSP or final Spiral/Bloom pitch binding. Those remain blocked behind M3c.

Physical FM-1: **not tested**.

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
