# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

Physical FM-1 is not currently available. Nothing below is promoted to level 6.

## Pinned baseline

- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- Dinghy tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS product `FM-1_972`
- target image budget 581,564 B
- RAM `.data + .bss` budget 98,304 B
- pool budget 344,064 B
- 44.1 kHz, 256-frame callback

## Phase 1 / Phase 2

Phase 1 skeleton and Phase 2 state core are verified through real target + emulator and merged.

Final M2c baseline:
- image 56,700 B
- RAM 14,104 B
- pool 30,720 B
- package `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`

## Phase 3

### M3a

Merged `fe2911c43fd69e63839220482e50bb10c7a2e4c2` via PR #4.
- core #60 PASS
- staged #50 PASS
- image 56,680 B; RAM 14,104 B; pool 30,720 B

### M3b

Merged `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5.
- final docs core #84 / staged #74 PASS
- image 66,428 B; RAM 22,552 B; pool 30,720 B
- emulator queue 7/256, overflow 0, cursor 123,648

### M3c

Merged `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6.

Frozen host determinism/capacity:
- `sizeof(ApsisRuntime)=21,928 B`
- queue 352/512 canonical; 357/512 extreme
- Spiral slots 59/64
- Bloom slots 44/64
- random `78771ff5`
- late `0e6fc422`
- normal + ASAN/UBSAN agree

Final target/emulator:
- image 71,608 B; RAM 35,272 B; pool 30,720 B
- package `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`
- peak 0 because M3c is intentionally silent

## Phase 4 / M4a — persistent String

Merged `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7.

Frozen host:
- Classic 1477 / `b20610c1`
- Dual L/R 823/787 / `10d23cf4`

Frozen emulator:
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512; cursor 123,648; late trace `811c9dc5`

## Phase 4 / M4b — finite String + shared Rhythm

Merged `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8.

Frozen host:
- finite partial engine 1309 / `97142d3e`
- 80 logical / 112 physical, 480 steals, 0 drops
- BODY 947 / `c1168570`
- Classic composite 80/108, 410 steals, 0 drops
- idle M4a hashes unchanged

Shared Rhythm:
- existing `STRING_RHYTHM` event only
- target Clock+26 ms
- planned mode frozen; pitch/surface/levels late-read
- first reference target 1147 becomes audible at 1280: 133 samples late, never early

Default Rhythm-OFF emulator remains exact M4a PCM.

## Phase 4 / M4c — String Attractor

Merged `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9.

Worst legal 60 s shared-timeline stress:
- heap 415/512
- pending 60/64
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- zero overflow/starvation

Full handoff reference:
- target 36,750
- bind-block end 34,816
- exact remaining voice delay 1,934 samples

Final target:
- image 85,564 B; RAM 44,680 B; pool 30,720 B
- package `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`

Default Attractor-OFF emulator remains exact M4a PCM and frozen M3c telemetry.

## Phase 4 / M4d — persistent Drift

Merged `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10.

Frozen host:
- peak 488
- hash `0aec6bc2`

Final target/emulator:
- image 86,544 B; RAM 44,760 B; pool 30,720 B
- package `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- queue 28/512; cursor 123,648; late trace `811c9dc5`

Dedicated Drift-DSP bypass returned exactly to final M4c target/package/PCM/runtime.

## Phase 4 / M4e — Clock

Merged `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11.
Final documentation head `ba3c05e4da692f8d4d3b67cc7b17a69ec560d8eb`; core #231 + staged #220 PASS.

Frozen host:
- STONE accent 2081 / `edcc4a81`
- GLASS accent 1880 / `0adf9829`
- SNAP 738 / `04bc514a`
- worst legal pool 23/32, 176 triggers, 0 drops

Merged M4e target/emulator baseline:
- image 90,684 B; RAM 49,128 B; pool 30,720 B
- package `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

Clock-DSP bypass returns PCM/runtime and image/RAM/pool exactly to frozen M4d behaviour. Package SHA is not required to match M4d because M4e also corrects Clock acoustic state representation.

## Phase 4 / M4f — Spiral

Merged `734229f4d23626a176a8366de82aab2c58c48cc9` via PR #12.
Final documentation head `744559f89df733c4c167367efe30250b6dcdecdf`; core #261 + staged #250 PASS.

Frozen host normal + ASAN/UBSAN:
- THREAD 1230 / `aef6c83e`, 2 legs / 4 oscillators
- GLASS 1157 / `9efa9bdc`, 2 legs / 6 oscillators
- REED 967 / `e67a3c15`, 2 legs / 6 oscillators
- HALO 1256 / `e7ed6161`, 2 legs / 6 oscillators

Eight-second worst acoustic stress:
- hits 384
- logical high-water 15
- leg high-water 30/32
- oscillator high-water 90
- steals 0
- drops 0
- hash `02be4bc5`

Merged M4f target/emulator:
- image 95,804 B; RAM 58,056 B; pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- default source route is empty, so PCM remains M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor exactly 123,648; trace `811c9dc5`

M4f bypass is exact frozen M4e target/package/PCM/runtime.

## Phase 4 / M4g — Bloom

Branch `m4g-bloom`; draft PR #13.
Base: merged M4f `734229f4d23626a176a8366de82aab2c58c48cc9`.
Technically final implementation/CI head: `6c54d1cd5ca354543237a14c257b3ec28ab424d1`.

Status: **implementation verified through real target + emulator; final documentation-head gate and merge remain.**

Normative reference:
- `docs/reference/M4G_BLOOM_CONTRACT.md`

### Planner / bridge verification

Verified normal + ASAN/UBSAN:
- occurrence-time source-vector planner for frozen duration/unfurl/target offset/amp/pan
- external 64-slot snapshot runtime keyed to existing M3c Bloom slot/token
- no additional scheduler event or random draw
- existing M3c occurrence target−45 ms and per-petal causal target−50 ms bind remain authoritative
- per-petal bind consumes existing `ApsisBloomLateBinding`
- current Bloom voice/attack/brightness are late-read at bind
- frozen plan/petal count survives later DEPTH edits
- `sizeof(ApsisRuntime)` remains 21,928 B

### Frozen Bloom renderer references

Core #292 PASS normal + ASAN/UBSAN.

- VEIL peak/hash **175 / `6177470f`**
- CHOIR **143 / `4b2969f8`**
- ORGAN **122 / `d54a68d9`**
- GLASS **129 / `ca1c3127`**

Legal worst-case Bloom stress:
- triggers **186**
- petal high-water **125/136**
- oscillator high-water **375**
- drops **0**
- peak **266**
- deterministic hash **`5881bb36`**
- `sizeof(ApsisBloomDsp)=22,864 B`

Pool capacity is frozen at 136 petals, leaving 11 petal slots above the measured legal high-water.

All prior M3/M4 references remain unchanged in the same core run.

### Target integration / inactive-path gate

M4g is layered above the merged M4f target. It preserves the exact M4f callback path until an already-planned Bloom occurrence is due inside the current callback. Once activated, the Bloom layer stays live so pending petals and finite tails survive later state/mute edits.

Staged #281 PASS all gates:
- Gate 1 source core / pinned upstream
- Gate 2 unchanged Dinghy host + target
- Gate 3 actual M4g single-TU overlay + host shell
- Gate 4 real JieLi `FM-1_972` target package
- Gate 5 FM-1 emulator smoke
- Gate 5b exact M4f Bloom-DSP/runtime bypass

### Technically final M4g target

- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- RAM headroom **10,968 B**
- pool **30,720 / 344,064 B**
- pool headroom 313,344 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- product `FM-1_972`

### Default M4g emulator

The pinned three-second reference scenario does not reach a due Bloom occurrence, so the activation latch intentionally keeps the exact M4f audio/runtime path during this smoke.

- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856
- RMS 144/144
- stereo delta 125
- mean -2/-2
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648**
- late trace `811c9dc5`

This verifies real target integration, boot and baseline callback/audio/runtime preservation. Audible Bloom is verified by host source-vector, bridge, full shared-runtime handoff and finite-renderer tests plus real target compile/link. Physical listening is not verified.

### Gate 5b — exact M4f Bloom bypass

With only M4g Bloom DSP/runtime consumption omitted:
- image **95,804 B**
- RAM **58,056 B**
- pool **30,720 B**
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

This is exact frozen M4f target/package/PCM/runtime.

## Next verification

Freeze M4g continuity docs and `M4G_BLOOM_CONTRACT.md`, run both workflows on that exact documentation head, then merge PR #13 if green. After merge, begin Phase 5 sounding-source structural integration from the actual M4g merge commit. Physical FM-1 remains untested.
