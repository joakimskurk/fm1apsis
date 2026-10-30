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
- POOL-region budget 344,064 B
- 44.1 kHz, 256-frame callback

## Frozen Phase-3 runtime

M3c merged `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6.

Host invariants:
- `sizeof(ApsisRuntime)=21,928 B`
- queue 352/512 canonical; 357/512 extreme
- Spiral slots 59/64
- Bloom slots 44/64
- random `78771ff5`
- late `0e6fc422`
- normal + ASAN/UBSAN agree

Pinned emulator invariants:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

## Phase 4 merged dry engines

- M4a String persistent: `2ea217f09c143d30385f136e84e2b29d582987a0`
  - Classic 1477 / `b20610c1`
  - Dual L/R 823/787 / `10d23cf4`
- M4b String finite/Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9`
  - transient 1309 / `97142d3e`
  - BODY 947 / `c1168570`
  - 80 logical +32 fade-tail reserve
- M4c String Attractor: `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
  - shared heap 415/512; pending 60/64
  - advances/mutations/binds 1470/1470/2884
  - random `b7a9a395`; late `e777a771`
- M4d Drift: `d7e39e2481c33628f402bed51d761b8be8304bb3`
  - host 488 / `0aec6bc2`
- M4e Clock: `db9d0b3ecbc7931be343cec7717103059d3c5160`
  - STONE 2081 / `edcc4a81`
  - GLASS 1880 / `0adf9829`
  - SNAP 738 / `04bc514a`
  - pool stress 23/32, 176 triggers, 0 drops
- M4f Spiral: `734229f4d23626a176a8366de82aab2c58c48cc9`
  - THREAD 1230 / `aef6c83e`
  - GLASS 1157 / `9efa9bdc`
  - REED 967 / `e67a3c15`
  - HALO 1256 / `e7ed6161`
  - stress 30/32 legs, 90 oscillators, 0 steals/drops, `02be4bc5`
- M4g Bloom: `713a1e9ac746a1a4d19ba825a0e6765b337ba410`
  - VEIL 175 / `6177470f`
  - CHOIR 143 / `4b2969f8`
  - ORGAN 122 / `d54a68d9`
  - GLASS 129 / `ca1c3127`
  - stress 186 triggers, 125/136 petals, 375 oscillators, 0 drops, peak 266, `5881bb36`

## Frozen M4g / merged Phase-5 dry target

M4g merged through PR #13. Phase 5 later verified the live structural/sounding contracts and squash-merged through PR #15 as `d677a887cf6dc8552634bde5a71fec0221c299b5` without changing production scheduler/DSP/runtime.

Real target baseline:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- RAM headroom **10,968 B**
- pool **30,720 / 344,064 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- product `FM-1_972`

Pinned default emulator:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

Bloom bypass exactly returns to frozen M4f:
- image/RAM/pool 95,804 / 58,056 / 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- same PCM/runtime telemetry as M4f

## Phase 5 — sounding-source structural integration

Final Phase-5 head `17e7fd1247b0f5675261a7395b0558b5bbd6780d` passed core #363 + staged #352 and was merged via PR #15 as `d677a887cf6dc8552634bde5a71fec0221c299b5`.

Verified normal + ASAN/UBSAN:
- P5a immediate structural late binding reaches sounding Spiral/Bloom DSP;
- P5b quantized Gravity asymmetry reaches sounding Spiral/Bloom DSP;
- P5c Mother-held Gravity reaches sounding sources only after `->1` commit;
- P5d Chain discrete boundary reaches sounding Spiral/Bloom in source order;
- P5e Chain continuous Drift morph begins on target and reaches sounding DSP;
- P5f step-one RESYNC OFF free-runs into sounding Spiral energy.

Phase 5 consumed **0 additional static RAM** and did not change target audio/runtime identity.

## Phase 6 — candidate comparison

Branch `p6-space-cathedral`, draft PR #16.

Deterministic host prototype `tools/p6_space_prototype.c` compares:
- Study-126 statistical reference proxy;
- compact 8-line modulated FDN;
- nested allpass/Schroeder hybrid.

Normal + ASAN/UBSAN candidate runs pass.

Selected candidate: **compact 8-line modulated FDN**.

Selected FDN:
- delays `1871,1999,2137,2281,2441,2593,2767,2953` samples;
- +16 samples reserve each;
- mod rates `.071,.083,.097,.109,.127,.139,.157,.181 Hz`;
- ~±9 sample modulation;
- normalized 8x8 Hadamard feedback;
- one-pole damping per line.

Evidence across 4 / 8.4 / 12 s:
- endpoint within 3 dB of deterministic Study-126 reference;
- `abs(L/R corr) < 0.10`;
- HF proxy within 0.06;
- FDN metallicity ~0.11 and <0.20;
- Schroeder metallicity roughly 0.69..0.73;
- FDN tail 38,340 B vs Schroeder 50,900 B.

Conservative selected memory model:
- existing Dinghy pool 30,720 B;
- stereo 140 ms input history 24,696 B;
- stereo 1.2 s echo 211,680 B;
- FDN tail 38,340 B;
- total 305,436 B;
- headroom 38,628 B.

## Phase 6 — silent real-target memory gate

### Failed intermediate layouts

Two intentionally strict intermediate gates exposed a startup issue:
1. keeping the Space arrays in ordinary `.pool` and calling a retention function before `runtime_ready` produced cursor 123,392;
2. removing that call and retaining the arrays only with linker `KEEP(*(.pool*))` restored image 100,944 B but cursor still stayed 123,392.

Root cause was verified in pinned Dinghy: `fm1_cstart()` zeroes `[_pool_start, _pool_end)` at boot. Clearing the added 274,716 B shifted startup/audio callback cadence by one 256-frame callback.

The cursor invariant was **not** relaxed.

### Accepted memory layout

Validated head: `11ce731a740d11262d988c352a79774bbea3103a`.

The CI-only copied linker script places Phase-6 storage in a separate NOLOAD `.apsis_space_pool` section immediately before ordinary `.pool`. `_pool_end` remains after both sections so real POOL-region accounting still includes all storage; `_pool_start` begins after Space so Dinghy clears only the original pool.

Core #375: **PASS**.

Staged #364: **PASS**, including real target, default emulator, Bloom bypass and Gate 5c silent Space storage.

Real target with exact selected Space storage retained:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- pool **305,436 / 344,064 B**
- pool headroom **38,628 B**
- package `5db84c9e09c76d1434abd3e01e55abd4e5e85afa4b39124869fe87d3bb02ffed`
- product `FM-1_972`

Room renderer is still disabled in this gate.

Emulator remains exact frozen dry behaviour:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648**
- late trace `811c9dc5`

CI conclusions:
- `PASS: P6 selected-FDN pool storage fits real target pool=305436/344064 ram=87336/98304`
- `PASS: silent P6 pool probe preserves frozen M4g/Phase-5 PCM/runtime`

## Next verification

Implement the selected FDN as a **pure host fixed-point engine first**, using externally supplied delay storage and validity/fill counters so NOLOAD memory is never read before being written. Add deterministic reference and sanitizer tests before any `app_audio()` integration. Only then integrate the five post-Mother source sends into one shared room and require a room-disabled exact-dry target/emulator bypass gate.
