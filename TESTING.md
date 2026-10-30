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
- app image 581,564 B
- RAM `.data + .bss` 98,304 B
- pool 344,064 B
- sample rate 44.1 kHz
- callback baseline 256 frames

Physical FM-1 is not currently available. Nothing below is promoted to physical-hardware verification.

## Phase 1

Status: **verified through emulator**.

Pinned source/upstream identity, unchanged Dinghy host/target, APSIS overlay/package identity and emulator boot/input smoke are verified.

## Phase 2 / M2a + M2b + M2c

Status: **verified through emulator and merged**.

Desktop source-derived state suites and ASAN/UBSAN pass.

Final M2c target:
- image 56,700 B
- RAM 14,104 B
- pool 30,720 B
- package SHA-256 `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`
- emulator WAV peak 0

## Phase 3 / M3a

Merged as `fe2911c43fd69e63839220482e50bb10c7a2e4c2` via PR #4.

Final evidence:
- core #60 PASS
- staged #50 PASS
- image 56,680 B
- RAM 14,104 B
- pool 30,720 B
- package SHA `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`

## Phase 3 / M3b

Merged as `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5.

Final evidence:
- final docs core #84 / staged #74 PASS
- image 66,428 B
- RAM 22,552 B
- pool 30,720 B
- host runtime 8,456 B
- emulator queue 7/256, overflow 0, cursor 123,648
- package SHA `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`

## Phase 3 / M3c

Merged as `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6.
Final pre-merge head `4fa7d25920523b9e5d8591ae6185e1b5f910c083`.

Status: **verified through emulator and merged**.

Normative references:
- `docs/reference/LATE_BINDING_CONTRACT.md`
- `docs/reference/M3C_REFERENCE_TRACE.md`

Frozen host capacity/determinism:
- `sizeof(ApsisRuntime)=21,928 B`
- canonical queue 352/512
- max-swing extreme queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random `78771ff5`
- late `0e6fc422`
- zero overflow/starvation/late-slot overflow
- normal + ASAN/UBSAN agree

Final gates:
- core #123 PASS
- staged #113 PASS

Final target:
- image 71,608 B
- RAM 35,272 B
- pool 30,720 B
- package SHA `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Final emulator:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`
- peak 0 because M3c is intentionally silent

## Phase 4 / M4a — persistent dry String

Merged as `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7.
Final docs head `afe81ae3984797bcb3b99153c96b493dfb8575ec`.

Status: **verified through target + emulator and merged**.

Host references:
- Classic peak/hash 1477 / `b20610c1`
- Dual L/R peak 823/787; hash `10d23cf4`

Final gates:
- core #141 PASS normal + ASAN/UBSAN
- staged #130 PASS

Final target:
- image 75,700 B
- RAM 35,464 B
- pool 30,720 B
- package SHA `eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`

Frozen emulator audio:
- channels 2
- frames 124,928
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

## Phase 4 / M4b — finite String + shared Rhythm

Merged as `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8.
Final docs head `264a32e8…`; final gates core #174 + staged #163 PASS.

Status: **verified through target + emulator and merged**.

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

Finite engine verification:
- Study-98 normal harmonics 1..4 and pizz 1..3
- fixed-point brightness attenuation and 3 ms staggering
- current 16 ms String onset safety
- triangle LP / sine bypass
- folded Gravity-root shadow where source uses it
- fixed global pool: 80 logical +32 fade-tail =112 physical
- no allocation, no `libm`

Frozen host references:
- transient peak/hash 1309 / `97142d3e`
- burst 80/112, 480 steals, 0 drops
- BODY peak/hash 947 / `c1168570`
- BODY energy L/R `4176243 / 2564882`
- Classic composite 80/108, 410 steals, 0 drops
- idle M4a hashes unchanged

Shared Rhythm verification:
- uses existing `APSIS_EVENT_STRING_RHYTHM`
- target remains Clock pulse +26 ms
- BOW/PIZZ/MOTION freezes in event metadata
- pitch/surface/levels/brightness/Motion harmony late-read at bind
- passive observer preserves 21,928-byte M3c runtime
- first reference target 1147 becomes audible at 1280: 133 samples late, never early

Target #159:
- image 81,752 B
- RAM 42,648 B
- pool 30,720 B
- package SHA `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`

Default emulator:
- Rhythm OFF
- raw PCM remains exact M4a baseline `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- peak 317; RMS 101/100; stereo delta 63; mean -2/-2
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

## Phase 4 / M4c — String Attractor excitation

Merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9.
Final pre-merge head `fe53bf265882369e2c8c1e38899e9cc655301dd6`; final gates core #202 + staged #191 PASS.

Status: **verified through target + emulator and merged**.

Normative reference:
- `docs/reference/M4C_STRING_ATTRACTOR_CONTRACT.md`

Worst legal 60-second shared-timeline stress:
- shared heap 415/512
- pending sides 60/64
- completed-bind output bound 6 in deliberate 512-frame stress
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- zero pending/binding/queue/Spiral/Bloom overflow/starvation
- deterministic repeat; normal + ASAN/UBSAN agree

Full shared-timeline handoff:
- target 36,750
- bind callback ends 34,816
- remaining exact pool delay 1,934 samples
- first voice-age sample is target sample itself

Final M4c target:
- image 85,564 / 581,564 B
- RAM 44,680 / 98,304 B
- pool 30,720 / 344,064 B
- package SHA `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`

Final M4c emulator:
- Attractor OFF
- frames 124,928
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

## Phase 4 / M4d — persistent dry Drift

Merged as `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10.
Final pre-merge documentation head `82106c33…`; final gates core #214 + staged #203 PASS.

Status: **verified through target + emulator and merged**.

Normative reference:
- `docs/reference/M4D_DRIFT_CONTRACT.md`

Frozen host reference:
- peak 488
- hash `0aec6bc2`
- normal + ASAN/UBSAN agree

Final M4d target:
- image 86,544 / 581,564 B
- RAM 44,760 / 98,304 B
- pool 30,720 / 344,064 B
- package bytes 610,110 B
- package SHA `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`

Default Drift-on emulator:
- frames 124,928
- peak 433
- RMS L/R 135/135
- stereo delta 125
- mean L/R -3/-3
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

Dedicated Drift-DSP bypass returned exactly to final M4c target/package/PCM/runtime.

## Phase 4 / M4e — Clock dry engine

Branch `m4e-clock`; PR #11.
Base: M4d merge `d7e39e2481c33628f402bed51d761b8be8304bb3`.
Validated implementation head: `285aacb5309fd9abaa51814e4ea6a09a3e480a10`.

Status: **implementation verified through real target + emulator; merge pending exact final documentation-head gate**.

Normative reference:
- `docs/reference/M4E_CLOCK_CONTRACT.md`

### Host/state verification

Verified source-correct acoustic state:
- STONE / WOOD / GLASS / SOFT
- `pitch_centi=50..200`, default 100
- unchanged KEEP Clock-memory semantics (geometry only)

Verified shared timing handoff:
- existing `CLOCK_MAIN`, `CLOCK_SNAP`, `CLOCK_INNER`, `CLOCK_GHOST` events only
- no private Clock scheduler/queue
- passive block observer does not mutate runtime or draw musical RNG
- pulse-0 bootstrap only fills deterministic first-block Clock events before initial shared scheduler fill
- per-event sample delay preserves the original shared target sample inside the callback

Verified acoustic topology:
- all four source kick profiles
- GLASS accent overtone
- INNER normal profile × current inner level
- GHOST ghost profile ×0.55 with no second random draw
- Snap three deterministic filtered/panned noise bursts + pitched square edge
- Clock-local Snap RNG; shared musical RNG remains frozen

Frozen host references, identical normal + ASAN/UBSAN:
- STONE accent peak/hash **2081 / `edcc4a81`**
- GLASS accent peak/hash **1880 / `0adf9829`**
- SNAP peak/hash **738 / `04bc514a`**

Legal worst-case Clock pool stress:
- 180 BPM, 1/16, max swing
- GLASS, pitch 2.00×
- main/inner/snap all enabled, all mains accented, all eligible Ghosts forced
- source/inner/snap levels 1.25, Snap 8500 Hz
- result **23/32 high-water, 176 triggers, 0 drops**

### JieLi target compatibility

The first target integration linked all host code but failed on missing pi32v2/libgcc `__divdi3`, emitted by exact 64-bit Clock coefficient arithmetic.

The accepted implementation supplies target-only `__divdi3` / `__udivdi3` through `apsis_target_div64.c` using shift/compare/subtract restoring division. Clock arithmetic and frozen host references were not weakened or changed to accommodate the linker.

### Implementation-head gates

Core #225 on `285aacb5309fd9abaa51814e4ea6a09a3e480a10`:
- PASS normal suite
- PASS ASAN/UBSAN
- frozen M3/M4a/M4b/M4c/M4d references unchanged

Staged #214 on the same head:
- Gate 1 source/core PASS
- Gate 2 unchanged Dinghy host/target PASS
- Gate 3 APSIS overlay/host PASS
- Gate 4 real JieLi target/package PASS
- Gate 5 default Clock-on FM-1 emulator PASS
- Gate 5b dedicated Clock-DSP bypass PASS

Default M4e target:
- image **90,684 / 581,564 B**
- RAM **49,128 / 98,304 B**
- pool **30,720 / 344,064 B**
- package bytes 610,110 B
- package SHA `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- product `FM-1_972`

Cost versus M4d:
- +4,140 B app image
- +4,368 B RAM
- +0 B pool

Default Clock-on emulator:
- frames 124,928
- peak **856**
- RMS L/R **144/144**
- stereo delta **125**
- mean L/R **-2/-2**
- raw PCM **`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`**
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

The PCM change is expected because Clock is ON and all 12 main pulses are enabled by default. Runtime telemetry remains frozen.

### Exact M4d PCM/runtime bypass gate

Gate 5b rebuilds the same target with only the new Clock DSP/consumer omitted. The source-correct Clock state remains.

Bypass target/emulator:
- image **86,544 B**
- RAM **44,760 B**
- pool **30,720 B**
- package SHA `3670d58da19a1ccad01a142ff328b6faf273901450b61b361dd436ad5041990f`
- raw PCM **`539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`**
- peak 433; RMS 135/135; stereo delta 125; mean -3/-3
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

Image/RAM/pool, PCM and runtime telemetry exactly return to frozen M4d values. Exact package SHA is intentionally not required because M4e source-corrects Clock acoustic state representation even in the DSP-bypass build.

### M4e verification interpretation

Verified through level 5:
- source-derived Clock acoustic topology
- shared-event timing consumption with no new scheduler
- deterministic finite pool and local Snap RNG
- fixed-point renderer + target compatibility shim
- real JieLi target linkage/budgets
- default Clock-on emulator audio
- exact pre-M4e M4d PCM/runtime preservation when Clock DSP is omitted

Not verified:
- physical FM-1 listening
- hardware controls/UI
- real ISR-cycle margin
- shared Space/Cathedral

Before merge, gate the exact final documentation head with both workflows. Do not create a later checkbox-only commit merely to record those final runs; record them in PR metadata and merge with an expected-head guard.

## Release-level validation not yet performed

- physical FM-1 install
- OCT- rescue / return to official firmware
- USB/MIDI interaction on device
- all keys/buttons/encoders/LEDs/MASTER
- real ISR worst-case timing
- five-source + 4x Spiral + Bloom + String Motion + Cathedral stress on physical DSP
- persistent KEEP/CHAIN reboot/recovery
- power interruption during flash write

Do not call any of these verified until actual hardware evidence exists.
