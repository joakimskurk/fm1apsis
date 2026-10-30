# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

Physical FM-1 is not currently available. Nothing below is level 6.

## Pinned baseline

- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`
- target image budget 581,564 B
- RAM budget 98,304 B
- POOL budget 344,064 B
- source/output rate 44.1 kHz, 256-frame callback

## Frozen Phase-3 runtime

- `sizeof(ApsisRuntime)=21,928 B`
- queue 352/512 canonical; 357/512 extreme
- Spiral slots 59/64
- Bloom slots 44/64
- random `78771ff5`
- late `0e6fc422`
- normal + ASAN/UBSAN agree

## Frozen Phase-5 / M4g target

- image 100,944 B
- RAM 87,336 B
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- dry PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648**
- late trace `811c9dc5`

M4f Bloom-bypass remains independently frozen:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- same dry PCM/runtime telemetry and cursor 123,648

## Phase 6 host/reference evidence

Selected compact 8-line modulated FDN:
- default peak 217 / hash `a190ca86` / late 2,646,000
- decay4 1,455,300
- decay12 3,507,773
- dark `7d4c13e3`
- bright `7e1a4d9e`

Full host room:
- default peak 47
- hash `1350a6da`
- late 1,278,900
- first non-zero sample 927
- no-echo `97669507`
- dark `5ff1f59a`

Normal and ASAN/UBSAN gates are clean. The host/reference room remains 44.1 kHz.

## Phase 6 target architecture

One shared post-Mother room with sends:
- String 0.72
- Drift 0.20
- Clock 0.06
- Spiral 0.62
- Bloom 0.58

Long storage is in dedicated pre-`.pool` NOLOAD memory and self-initialises through fill/valid counters.

The generated FM-1 target uses a quarter-rate room renderer at **11.025 kHz**. Source engines, scheduler and final output remain 44.1 kHz. Target-only retiming preserves room milliseconds, early taps, predelay, echo delay, FDN decay/damping and modulation rate.

## Final technical Phase-6 verification head

Head before final documentation freeze: `0ec9915f9d994454aa079a5be107765844534666`.

- core #416: **PASS**
- staged #405: **PASS all gates**
- P6 final #14: **PASS all gates**

The staged multi-variant reporter was corrected on this head to parse the final resource summary in the append-only target log. This restored truthful M4f Bloom-bypass reporting without changing firmware bytes.

### Real target, audible P6

- product `FM-1_972`
- image **107,352 / 581,564 B**
- RAM **87,780 / 98,304 B**
- RAM headroom 10,524 B
- POOL **309,532 / 344,064 B**
- POOL headroom 34,532 B

Pinned emulator:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`

### P6 dry-only regression

The same P6 target/source-split/storage path with final room return omitted produces:
- exact Phase-5 PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`

The shared 123,392 value for audible and dry-only P6 variants is treated as the explicit Phase-6 boot-to-stop scenario baseline. It is one 256-frame callback below the historical M4g scenario but does not coincide with late flags, queue overflow, trace divergence, or a scheduler change.

### Historical silent-storage gate

Before audible integration, the selected 44.1-kHz conservative storage probe used 305,436 / 344,064 B POOL and preserved the old 123,648 dry cursor. This proved the pre-`.pool` NOLOAD placement itself does not perturb boot timing.

## Current verification boundary

Phase 6 is verified through level 5: implementation, desktop/reference tests, sanitizers, real JieLi target build and pinned emulator.

Still unverified until physical hardware exists:
- real ISR-cycle margin
- actual DAC/audio listening quality
- full physical controls/LED/display behaviour
- worst-case combined UI/audio load
- power-loss persistence behaviour

## Next verification

Run the same core + staged + P6-final gates on the final documentation head, then guarded-merge PR #16.