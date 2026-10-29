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

Verified planner/runtime behavior:
- explicit LEFT/RIGHT/ALTERNATE/BOTH target, default ALTERNATE
- deterministic local seeded LCG; no shared draw on reset
- legal 0.5/1/2/4/8 rates on the shared Phase-3 timeline
- FILTER-only advances/mutates; SUSTAIN draws/schedules nothing
- active random order Attractor -> Spiral -> Clock ghost
- planning freezes surface/route/value/position/gate/base level
- `STRING_ATTRACTOR_BIND` is shared-heap, not a private scheduler
- external Attractor runtime preserves `sizeof(ApsisRuntime)=21,928 B`

Worst legal 60-second shared-timeline stress:
- shared heap 415/512
- pending sides 60/64
- completed-bind output bound 6 in deliberate 512-frame stress
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- zero pending/binding/queue/Spiral/Bloom overflow/starvation
- deterministic repeat; normal + ASAN/UBSAN agree

Acoustic/late-bind verification:
- left BODY: 720 ms, pan -0.30, attack 30 ms, no audible root shadow
- right WIRE: 420 ms, pan +0.30, ×0.92, Foundation root .05
- Classic: BODY ×.66 at target; WIRE ×.78 at +4 ms
- right `norm>.52` octave accent preserved
- all finite audio shares M4b's 80+32 pool

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
- product `FM-1_972`

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

Branch `m4d-drift`; PR #10.
Base: M4c merge `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.
Validated implementation head: `0b2a351b17adfa7556c217f24af8a08c1c708d2c`.

Status: **implementation verified through real target + emulator; merge pending exact final documentation-head gate**.

Normative reference:
- `docs/reference/M4D_DRIFT_CONTRACT.md`

### Source disposition

Study 126 still defines the identity-bearing Drift field through `beatDelta()` / `beatPair()` and auditions it through `probeBeat()`, but the transport path no longer instantiates that pair as an independent persistent `beat` source. Existing Drift state is also cross-coupled into already-frozen String/Spiral behavior.

The project port contract explicitly requires DRIFT as its own paired beating field around the Gravity root. M4d therefore repairs the missing persistent wiring while preserving the old cross-couplings for a separately scoped later decision.

### Host DSP verification

Native `ApsisDriftDsp`:
- two persistent sine voices around the live tuned Gravity root
- base delta `0.05 + 3.95 * amount^1.65` Hz
- audible separation scales by `max(0.15, depth)` and source level
- per-voice gain `.013 * level`
- historical `STEREO WIDTH` law `-0.34*width / +0.34*width`
- equal-power pan
- 25 ms pitch/gain/pan smoothing
- gain smoothing uses fractional internal precision
- fixed 44.1 kHz stereo Q15, allocation-free and no target `libm`
- no event, private queue, scheduler or RNG changes

`tests/test_drift_dsp.c` proves the amount endpoints, default symmetric pair, mute/depth law, Equal/Just/Pythagorean carrier movement, tuning strength, width center/full behavior and deterministic 25 ms smoothing.

Frozen 2-second default Drift reference:
- peak **488 Q15**
- hash **`0aec6bc2`**
- normal + ASAN/UBSAN agree

### Implementation-head gates

Core #210 on `0b2a351b17adfa7556c217f24af8a08c1c708d2c`:
- PASS normal suite
- PASS ASAN/UBSAN
- all frozen M3/M4a/M4b/M4c references unchanged

Staged #199 on the same head:
- Gate 1 source/core PASS
- Gate 2 unchanged Dinghy host/target PASS
- Gate 3 APSIS overlay/host PASS
- Gate 4 real JieLi target/package PASS
- Gate 5 default FM-1 emulator PASS
- Gate 5b dedicated Drift-DSP bypass PASS

Default M4d target:
- image **86,544 / 581,564 B**
- RAM **44,760 / 98,304 B**
- pool **30,720 / 344,064 B**
- package bytes 610,110 B
- package SHA `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`
- product `FM-1_972`

Cost versus M4c:
- +980 B app image
- +80 B RAM
- +0 B pool

Default Drift-on emulator:
- frames 124,928
- peak **433**
- RMS L/R **135/135**
- stereo delta **125**
- mean L/R **-3/-3**
- raw PCM **`539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`**
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

The PCM change is expected because Drift is ON by default; the frozen scheduling/runtime telemetry remains unchanged.

### Exact M4c bypass gate

Muting Drift state itself is not a clean isolation test because old String/Spiral code already consumes Drift state. Gate 5b instead rebuilds the same `0.1-dev` target with only `ApsisDriftDsp` omitted.

Bypass target/emulator:
- image **85,564 B**
- RAM **44,680 B**
- pool **30,720 B**
- package SHA **`c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`**
- raw PCM **`10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`**
- peak 317; RMS 101/100; stereo delta 63; mean -2/-2
- queue 28/512; overflow 0; late flags 0; late trace `811c9dc5`

Both the package SHA and PCM SHA exactly equal the frozen final M4c references. This proves the M4d target delta is isolated to the new persistent Drift DSP layer.

### M4d verification interpretation

Verified through level 5:
- source-derived Drift identity/laws under the explicit wiring-defect disposition
- fixed-point persistent DSP
- smoothing and stereo law
- target linkage/budgets
- default Drift-on emulator audio
- exact pre-M4d target/PCM preservation when the new Drift DSP layer is omitted
- unchanged shared runtime telemetry

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
