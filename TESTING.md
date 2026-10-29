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

Validated implementation head `0749ac86e6162fab190f343730ebc9b133c13e47`:
- core #170 PASS including `test_string_rhythm_live`
- staged #159 PASS all five gates

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

Branch `m4c-string-attractor`; PR #9.
Base: M4b merge `c96c00debf9a67948f1e874e36b090999dacadb9`.
Validated implementation head: `e2171caac10eb402e97a6d566eff71a27544e3fc`.

Status: **implementation verified through real target + emulator; merge pending exact final documentation-head gate**.

Normative reference:
- `docs/reference/M4C_STRING_ATTRACTOR_CONTRACT.md`

### M4c1 planner/runtime verification

Verified source behavior:
- explicit LEFT/RIGHT/ALTERNATE/BOTH target, default ALTERNATE
- deterministic local seeded LCG; no shared draw on reset
- step-one resync resets cursor, not cells
- legal 0.5/1/2/4/8 rates
- 0.5 targets pulse + half-pulse on shared heap
- FILTER-only advances/mutates; SUSTAIN draws/schedules nothing
- active RNG order Attractor -> Spiral -> ghost
- inactive/default path consumes no new draw/event
- planning freezes surface/route/value/position/gate/base level
- `STRING_ATTRACTOR_BIND` appended to shared heap
- external Attractor runtime preserves `sizeof(ApsisRuntime)=21,928 B`

Runtime lifecycle:
- transport start can deterministically regenerate cells from seed
- mid-run seed/length reset does not cancel already-planned binds
- stop cancels pending Attractor binds
- target implementation keeps BSS-zero lazy initialization until first actual Attractor activation so the default startup/audio path remains frozen M4b

Worst legal 60-second shared-timeline stress:
- 180 BPM, 1/4, max swing
- rate 0.5, BOTH, all gates, maximal mutation load
- concurrent Rhythm, Spiral rate4, Bloom and Clock
- deliberate 512-frame blocks
- queue **415/512**
- pending sides **60/64**
- completed-bind output bound **6**; normal 256-frame startup <=4
- advances 1470
- mutations 1470
- binds 2884
- random `b7a9a395`
- late `e777a771`
- zero pending/binding/queue/Spiral/Bloom overflow/starvation
- deterministic repeat matches; normal + ASAN/UBSAN agree

### M4c2 acoustic/late-bind verification

Verified planning-vs-bind contract:
- route/value/position/base level stay frozen
- String sound/pitch/source level/side level/actual consumed brightness/Foundation late-read at bind
- mute at bind skips voice
- live pitch/side-level edits before bind alter sounding voice
- live surface/target edits do not reroute already-planned occurrence

Audible topology tests:
- left BODY: 720 ms, pan -0.30, attack 30 ms, current brightness−0.04, no audible root shadow
- right WIRE: 420 ms, pan +0.30, ×0.92, Foundation root .05, current base String brightness
- Classic: BODY ×.66 at target; WIRE ×.78 at +4 ms
- right frozen `norm>.52`: sine 2×, input ×.15 before WIRE ×.92, +6 ms, 176.4 ms, pan +.216, 16 ms onset
- Classic accent total offset +10 ms
- all finite audio shares M4b's 80+32 pool

`tests/test_string_attractor_audio.c` passes normal + sanitizer.

Full shared-timeline handoff `tests/test_string_attractor_live.c`:
- frozen target 36,750
- bind callback consumed in block ending 34,816
- remaining start delay 1,934 samples
- voice age remains 0 until target
- first age increment occurs on target sample
- planned LEFT remains LEFT after live state changes to Classic/RIGHT while pitch/level are late-read
- normal + ASAN/UBSAN pass

### Implementation-head gates

Core #198 on `e2171caac10eb402e97a6d566eff71a27544e3fc`:
- PASS normal suite
- PASS ASAN/UBSAN
- all frozen M3/M4a/M4b references unchanged

Staged #187 on the same head:
- Gate 1 source/core PASS
- Gate 2 unchanged Dinghy host/target PASS
- Gate 3 APSIS overlay/host PASS
- Gate 4 real JieLi target/package PASS
- Gate 5 FM-1 emulator smoke PASS

Target #187:
- image **85,564 / 581,564 B**
- RAM **44,680 / 98,304 B**
- pool **30,720 / 344,064 B**
- package bytes 610,110 B
- package SHA `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`
- product `FM-1_972`

Default emulator #187:
- Attractor OFF
- frames 124,928
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM **bit-identical** to frozen M4a/M4b:
  `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

### Rejected PCM regression and fix

The first fully integrated M4c target head `592cf49409f0d76fc98263882f28d04a3cec9209` passed target/emulator structure but changed the default raw PCM hash while Attractor was OFF. Peak/RMS/stereo values were unchanged, but bit identity was lost, so that head was rejected.

The accepted head `e2171caa…` keeps the external Attractor runtime BSS-zero and uses the exact frozen M4b observer/startup path until first actual Attractor activation. First active planning performs deterministic lazy cell sync. This restored the immutable PCM hash exactly; the baseline was not relaxed.

### M4c verification interpretation

Verified through level 5:
- planner/state semantics
- shared scheduling/random order
- capacity
- late binding
- target-delayed finite audio topology
- single-TU JieLi target linkage/budgets
- default emulator regression transparency

Not verified:
- physical FM-1 listening
- hardware controls/UI
- real ISR-cycle margin
- audible dynamic String filter S&H path

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
