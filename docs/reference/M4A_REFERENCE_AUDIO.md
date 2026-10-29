# M4A REFERENCE AUDIO

This file freezes the first verified native APSIS audio reference on FM-1 target code.

## Scope

Milestone: Phase 4 / M4a — persistent dry String core.

Reference implementation head:
- `839bc7b65695b7fa81a6336995537d374b8a3e1b`
- branch `m4-string-dsp`
- PR #7

This is the first APSIS milestone where `app_audio()` intentionally emits non-zero audio. It contains only the persistent dry String layer:
- Study-98 Foundation pair;
- Study-98 Classic sine/Drift pair when Classic is selected;
- Study-126 BODY/WIRE pair in default Dual mode;
- no String transient/rhythm/Attractor engine yet;
- no Clock/Spiral/Bloom audio yet;
- no shared Cathedral/Space.

Behavioral/timing authority remains Study 126. Study 98 is only the acoustic authority described in `STRING_DSP_CONTRACT.md`.

## Host deterministic references

Normal and ASAN/UBSAN runs agree.

Classic persistent render:
- peak: **1477 Q15**
- deterministic hash: **`b20610c1`**

Default Dual persistent render:
- left peak: **823 Q15**
- right peak: **787 Q15**
- deterministic hash: **`10d23cf4`**

The complete existing M3c suites remain unchanged on the same head:
- runtime queue 352/512, Spiral slots 59/64, Bloom slots 44/64
- random trace `78771ff5`
- late-binding trace `0e6fc422`
- extreme queue 357/512

## Real JieLi target reference

Staged validation #126 compiled the live M4a renderer into the real FM-1 target single translation unit and passed packaging.

Target:
- app image: **75,700 B / 581,564 B**
- image headroom: **505,864 B**
- RAM `.data + .bss`: **35,464 B / 98,304 B**
- RAM headroom: **62,840 B**
- pool: **30,720 B / 344,064 B**
- pool headroom: **313,344 B**
- package bytes: **610,110 B**
- product: **`FM-1_972`**
- package SHA-256: **`eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`**

Delta from final M3c target:
- image: **+4,092 B**
- RAM: **+192 B**
- pool: **0 B**

The RAM delta is consistent with the small persistent four-voice DSP state. The larger image delta carries the oscillator/frequency tables and renderer code.

## FM-1 emulator audio reference

Staged validation #126 PASS on the same implementation head.

WAV:
- channels: **2**
- frames: **124,928**
- peak: **317 Q15**
- RMS left: **101 Q15**
- RMS right: **100 Q15**
- maximum stereo delta: **63 Q15**
- mean left: **-2**
- mean right: **-2**
- raw PCM SHA-256: **`10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`**

The low absolute level is intentional at this stage: M4a preserves the source-derived per-voice gains and has not yet added later Mother/master/source-bus or room gain architecture.

Runtime telemetry remains identical to M3c smoke expectations:
- live queue high-water: **28 / 512**
- queue overflow: **0**
- late-slot/starvation flags: **0**
- runtime cursor low32: **123,648**
- late trace: **`811c9dc5`**

UI smoke:
- 500 ms: `4c6096fddb2d15cbf3e7a6aa19410e9dc9b7dfc2d41fbe37ccd3d0d6a8c6623b`
- 1100 ms: `6aa885849725b63695932727e46793f0bd49a36bde23af47975c343462e774b1`
- 2100 ms: `4c6096fddb2d15cbf3e7a6aa19410e9dc9b7dfc2d41fbe37ccd3d0d6a8c6623b`

## Callback integration boundary

M4a renders String from the state visible at the beginning of each callback, then advances the M3c structural runtime for that callback.

At the 44.1 kHz / 256-frame baseline this means a structural state change inside a callback may become audible at most one callback late (about 5.8 ms), but never early from future state.

This is an intentional Phase-4 boundary. Sample-exact structural-to-sounding-source application belongs to Phase 5 and must not be claimed by M4a.

## Performance interpretation

The emulator smoke step took approximately the same wall-clock duration as the earlier M3c emulator smoke, so there is no emulator-based evidence of a major performance regression from the first String engine.

This is **not** a substitute for real ISR timing on physical FM-1 hardware. Physical hardware is unavailable, so actual callback-cycle headroom remains unverified.

Static arithmetic bounds show that BODY harmonic accumulation, one-pole filtering, gain multiplication and stereo panning can later be reduced from 64-bit to 32-bit intermediates without overflow under the current Q15 contracts. Do not make that optimization unless the host and emulator audio hashes remain bit-identical afterward.

## Verification level

- implemented: YES
- desktop regression: PASS
- ASAN/UBSAN: PASS
- real JieLi target build: PASS
- FM-1 emulator with non-zero audio: PASS
- physical FM-1: NO

The next String slice should add source-derived transient behavior separately; do not fold transient work into this frozen persistent reference.