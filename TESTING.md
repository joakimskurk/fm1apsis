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

Branch `m4f-spiral`; draft PR #12.
Base: merged M4e `db9d0b3ecbc7931be343cec7717103059d3c5160`.
Technically final implementation/CI head: `11569371ea18bc8a5e06f28d09ddef90f9835e29`.

Status: **implementation verified through real target + emulator; merge pending exact final documentation-head gate**.

Normative reference:
- `docs/reference/M4F_SPIRAL_CONTRACT.md`

### Host planning/bridge verification

Verified:
- source duration/energy/noteLength law
- source-level and Drift audible delta/width frozen at planning time
- THREAD / GLASS / REED / HALO topology
- two-leg main/shadow topology with +8 ms shadow local delay
- no new scheduler event or random draw
- external snapshot runtime keyed to existing M3c token/slot
- M3c target−80 ms late pitch binding remains authoritative
- planning-time topology/Q/envelope with current-voice late cutoff-family edge behaviour
- `sizeof(ApsisRuntime)` remains 21,928 B

### Frozen Spiral renderer references

Identical normal + ASAN/UBSAN:
- THREAD peak/hash **1230 / `aef6c83e`**, 2 legs / 4 oscillators
- GLASS **1157 / `9efa9bdc`**, 2 legs / 6 oscillators
- REED **967 / `e67a3c15`**, 2 legs / 6 oscillators
- HALO **1256 / `e7ed6161`**, 2 legs / 6 oscillators

Eight-second worst acoustic stress:
- hits 384
- logical high-water 15
- leg high-water **30/32**
- oscillator high-water 90
- steals 0
- drops 0
- hash **`02be4bc5`**

All prior M3/M4 host references remain unchanged.

### Idle callback regression and fix

The first fully integrated M4f target (`923e7555…`) passed all gates but default emulator telemetry advanced only 123,392 samples instead of frozen 123,648 — exactly one 256-frame callback less — even though default Spiral route is empty and PCM was unchanged.

Pinned Dinghy boot inspection proved `.bss` zeroing occurs before audio init/IRQ, so BSS size was not the cause. The issue was unnecessary per-callback M4f runtime/binding clear and 32-leg renderer scanning while Spiral could not produce audio.

Accepted fix (`7a01a890…`):
- route-empty default path uses exact frozen M4e runtime/audio path;
- Spiral layer latches on only when route is non-empty and sound is ON;
- once latched it stays active so pending binds/tails survive later route/mute edits;
- renderer is not scanned before the first hit.

Result: default cursor restored to **123,648** with unchanged Spiral DSP semantics.

`ci/emulator_smoke.sh` now asserts exact cursor 123,648. Assertion head `11569371…` passed core #260 and staged #249.

### Technically final M4f target/emulator

Target:
- image **95,804 / 581,564 B**
- RAM **58,056 / 98,304 B**
- pool **30,720 / 344,064 B**
- RAM headroom 40,248 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- product `FM-1_972`

Default emulator:
- source-default Spiral route is empty, so no audible Spiral voice is expected
- PCM remains exact M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; **cursor 123,648**; trace `811c9dc5`

The default emulator proves integration, boot, baseline audio/runtime preservation and callback-budget invariance. Because route is empty, audible Spiral is verified at host/full-chain level plus real target compile/link, not by a target listening scenario.

### Gate 5b — exact M4e bypass

With only Spiral DSP/runtime consumer omitted:
- image **90,684 B**
- RAM **49,128 B**
- pool **30,720 B**
- package `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

This is exact frozen M4e target/PCM/runtime.

### Exact implementation-head gates

- core #260 PASS
- staged #249 PASS all gates including real JieLi target, default emulator, exact cursor assertion and Spiral-DSP bypass

## Next verification

Freeze M4f continuity docs, run both workflows on that exact documentation head, then merge PR #12 if green. Physical FM-1 remains untested.
