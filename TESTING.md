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

## M4g real target baseline

Final M4g documentation head `f3633cec98a71108bc64d57049ed24064f9632a3` passed core #299 + staged #288 before merge via PR #13.

Real target:
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

Active PR #15, branch `p5-sounding-integration`, base merged M4g `713a1e9a…`.

Technically final verification head before docs freeze:
- `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`
- APSIS core #357 PASS
- APSIS staged validation #346 PASS

### P5a — structural late binding

Normal + ASAN/UBSAN:
- PASS: `APSIS P5a structural late binding reaches sounding Spiral/Bloom DSP`
- verifies structural state mutation through existing M3c bind to actual finite Spiral/Bloom oscillator phase increment
- occurrence token, target sample and occurrence-frozen acoustic values remain unchanged

### P5b — quantized Gravity

Normal + ASAN/UBSAN:
- PASS: `APSIS P5b quantized Gravity asymmetry reaches sounding Spiral/Bloom DSP`
- Spiral uses committed state only at its target−80 ms bind
- Bloom may project ordinary pending Gravity target-aware under the frozen +0.5 ms rule

### P5c — Mother `->1`

Normal + ASAN/UBSAN:
- PASS: `APSIS P5c Mother held Gravity reaches sounding sources only after ->1 commit`
- Mother-held future state is not projected by Spiral or Bloom before the structural commit

### P5d — Chain discrete

Normal + ASAN/UBSAN:
- PASS: `APSIS P5d Chain discrete boundary reaches sounding Spiral/Bloom in source order`
- verifies existing early Chain observation and subsequent M3c late-bound sounding consumers without another scheduler

### P5e — Chain continuous morph

Normal + ASAN/UBSAN:
- PASS: `APSIS P5e Chain continuous Drift morph begins on target and reaches sounding DSP`
- verifies source-defined continuous morph timing reaches the actual persistent Drift renderer

### P5f — step-one free-run/resync

Normal + ASAN/UBSAN:
- PASS: `APSIS P5f step-one RESYNC OFF free-runs into sounding Spiral energy`
- source trigger position is taken from the existing frozen M3c Spiral slot/token; M4f acoustic snapshots are not expanded to duplicate scheduler identity

### Phase-5 production-impact proof

Phase 5 required no production scheduler/DSP/runtime change.

Staged #346 rebuilt the exact M4g target:
- image **100,944 B**
- RAM **87,336 B**
- pool **30,720 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- default PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

The same staged run passed the exact frozen M4f Bloom-bypass gate. Therefore Phase 5 consumed **0 additional static RAM** and did not change target audio/runtime identity.

## Next verification

Run core + full staged validation on the exact Phase-5 final documentation head. If both pass, merge PR #15 with a guarded squash merge. Only then begin Phase 6 Space/Cathedral prototyping from the actual merge commit.
