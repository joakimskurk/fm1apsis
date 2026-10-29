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
- app image: 581,564 B
- RAM `.data + .bss`: 98,304 B
- pool: 344,064 B
- sample rate: 44.1 kHz
- callback baseline: 256 frames

Physical FM-1 is not currently available. Nothing below is promoted to physical-hardware verification.

## Phase 1

Status: **verified through emulator**.

Verified pinned source/upstream identity, unchanged Dinghy host/target, APSIS overlay/package identity and emulator boot/input smoke.

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

Merged to `main` as `fe2911c43fd69e63839220482e50bb10c7a2e4c2` via PR #4.

Status: **verified through emulator and merged**.

Final evidence:
- core #60: PASS
- staged #50: PASS
- image 56,680 B
- RAM 14,104 B
- pool 30,720 B
- package SHA-256 `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`
- emulator WAV peak 0

## Phase 3 / M3b

Merged to `main` as `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5.

Status: **verified through emulator and merged**.

Final evidence:
- core #81 implementation head: PASS incl. ASAN/UBSAN
- staged #71 implementation head: PASS
- final documentation-head core #84 / staged #74: PASS
- image 66,428 B
- RAM 22,552 B
- pool 30,720 B
- host runtime sizeof 8,456 B
- host queue 14/256
- emulator live queue 7/256, overflow 0, cursor 123,648
- package SHA-256 `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`
- emulator WAV peak 0

## Phase 3 / M3c

Merged to `main` as `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6.
Final pre-merge head: `4fa7d25920523b9e5d8591ae6185e1b5f910c083`.

Status: **verified through emulator and merged**.

Normative references:
- `docs/reference/LATE_BINDING_CONTRACT.md`
- `docs/reference/M3C_REFERENCE_TRACE.md`

Verified source behavior includes:
- Spiral occurrence identity frozen but final sequence/pitch/register/tuning re-read at native bind lead 80 ms;
- Bloom occurrence at target−45 ms with frozen plan/register/random/unfurl geometry and per-petal target-aware bind at `max(occurrence, target−50 ms)`;
- narrow-browser 110/70 ms leads retained as source facts;
- normal Gravity target−18 ms, Mother target−26 ms, Chain discrete target−24 ms, Chain morph at target;
- Stop/restart cancellation semantics;
- Bloom unfurl represented as 0..3600 ms, default 420 ms;
- 32-bit `spiral_step` for long-run parity.

Capacity/determinism evidence:
- original 256-entry M3 queue failed legal M3c worst-case stress and was raised to 512;
- canonical 30 s: runtime 21,928 B, queue 352/512, Spiral slots 59/64, Bloom slots 44/64, random `78771ff5`, late `0e6fc422`;
- 60 s max-swing extreme: queue 357/512, Spiral 59/64, Bloom 44/64;
- overflow/starvation/late-slot overflow all 0;
- normal and ASAN/UBSAN agree.

Final exact-head gates:
- APSIS core #123: PASS incl. ASAN/UBSAN
- staged validation #113: PASS through real JieLi target and emulator

Final target:
- image 71,608 B / 581,564 B
- RAM 35,272 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA-256 `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Final emulator:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`
- WAV peak 0, mandatory because M3c is intentionally silent

## Phase 4 / M4a — persistent dry String core

Branch: `m4-string-dsp`.
PR: #7.
Base: M3c merge `ec0dc0b350793572f19040efc938f00121128dc6`.
First live-audio implementation/evidence head: `839bc7b65695b7fa81a6336995537d374b8a3e1b`.

Status: **implementation complete and verified through real target + emulator; final documentation-head gate required before merge**.

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

### M4a acoustic contract implemented

Persistent Foundation, both modes:
- Study-98 low Gravity-root pair;
- `scaleAt(frameDegree())`, foundation octave -1 default;
- root ±0.55× current audible Drift delta;
- sine pair;
- source-derived gain and dark low-pass behavior.

Classic:
- Study-98 sine/Drift sustain pair around current Study-126 String pitch;
- pitch ±0.5× Drift delta;
- source-derived `.020` per-voice gain before source level;
- smooth pitch/gain with no phase reset.

Dual:
- Study 98 predates Dual, so Study 126 remains the only acoustic authority for this later feature;
- independent left BODY and right WIRE pitches from the current Study-126 String model;
- BODY PeriodicWave spectrum `[1,.31,.15,.085,.052,.030,.018]`, normalized;
- WIRE triangle;
- BODY/WIRE source gains, low-pass roles and ±0.23 stereo positions preserved;
- mode change fades to zero before waveform/filter/pan swap, then fades in.

Runtime/output:
- fixed four persistent oscillator slots: two Foundation plus two Classic or BODY/WIRE;
- allocation-free fixed-point/Q15 renderer;
- no `libm` dependency;
- target-safe frequency/phase math;
- output is stereo interleaved Q15 matching Dinghy ABI;
- persistent sustain is transport-gated;
- M3c runtime remains the shared scheduler; no String-private clock.

### M4a host + sanitizer evidence

`tests/test_string_dsp.c`: PASS normal + ASAN/UBSAN.

Classic reference:
- peak 1477 Q15
- deterministic hash `b20610c1`

Dual reference:
- left peak 823 Q15
- right peak 787 Q15
- deterministic hash `10d23cf4`

Coverage includes:
- exact default Foundation/Classic/Dual target frequencies/gains;
- independent Dual left/right pitches;
- waveform/filter/pan role selection;
- Foundation enable/disable;
- sound/transport gating;
- pitch glide without phase reset;
- smooth stop/restart;
- Dual→Classic fade/swap transition;
- bounded long render and DC stability;
- deterministic reset/replay.

All M3c regression/capacity hashes remain unchanged on the same head.

### M4a real target evidence

Staged validation #126: PASS.

Target:
- image **75,700 B / 581,564 B**
- RAM **35,464 B / 98,304 B**
- pool **30,720 B / 344,064 B**
- package bytes 610,110 B
- package SHA-256 **`eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`**

Delta vs final M3c:
- image +4,092 B
- RAM +192 B
- pool 0 B

### M4a emulator evidence — first intentional APSIS audio

Staged validation #126: PASS.

WAV:
- channels 2
- frames 124,928
- peak **317 Q15**
- RMS L/R **101 / 100 Q15**
- maximum stereo delta **63 Q15**
- mean L/R **-2 / -2**
- raw PCM SHA-256 **`10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`**

Runtime telemetry remains healthy and unchanged from M3c smoke:
- queue **28 / 512**
- overflow **0**
- late flags **0**
- cursor **123,648**
- late trace **`811c9dc5`**

UI screen SHA-256:
- 500 ms `4c6096fddb2d15cbf3e7a6aa19410e9dc9b7dfc2d41fbe37ccd3d0d6a8c6623b`
- 1100 ms `6aa885849725b63695932727e46793f0bd49a36bde23af47975c343462e774b1`
- 2100 ms `4c6096fddb2d15cbf3e7a6aa19410e9dc9b7dfc2d41fbe37ccd3d0d6a8c6623b`

### M4a timing boundary

M4a renders from state visible at callback start, then advances M3c runtime for that callback. A structural change inside a 256-frame block may therefore become audible at most one block late (~5.8 ms), never early.

Sample-exact structural-to-sounding-source application is **not** claimed here; it belongs to Phase 5.

### M4a performance interpretation

The emulator smoke duration is approximately the same as the earlier M3c smoke, so there is no emulator evidence of a major performance regression. This does not replace real physical ISR timing.

Static bounds show several per-sample 64-bit intermediates can later be reduced to 32-bit while preserving exact Q15 arithmetic. Treat this as target optimization work and require bit-identical host/emulator references after any change.

### M4a verification levels now

- implemented: YES
- committed: YES
- desktop regression: PASS
- sanitizer: PASS
- real target build with live renderer: PASS
- emulator with non-zero stereo String audio: PASS
- M3c scheduler/runtime regressions: PASS
- physical FM-1: NO
- merged: NO

Before merge, run both workflows once more on the exact final documentation head.

## Release-level validation not yet performed

- physical FM-1 install
- OCT- rescue
- return to official firmware
- USB/MIDI interaction on device
- all keys/buttons/encoders/LEDs/MASTER
- real ISR worst-case timing
- five-source + 4x Spiral + Bloom + String Motion + Cathedral stress with real DSP
- persistent KEEP/CHAIN reboot/recovery
- power interruption during flash write

Do not call any of these verified until actual hardware evidence exists.
