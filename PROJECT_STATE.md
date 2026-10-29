# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4a — persistent dry String DSP core, final documentation gate before merge.**

Phase 3 is complete and merged:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6

Active work:
- branch `m4-string-dsp`
- PR #7
- base `ec0dc0b350793572f19040efc938f00121128dc6`
- first live-audio implementation/evidence head `839bc7b65695b7fa81a6336995537d374b8a3e1b`

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic authority:
- Study 98 · Performance Architecture where the acoustic feature existed there
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- later features absent from Study 98, notably Dual BODY/WIRE, remain Study-126 acoustic behavior

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

Physical FM-1: **not tested**.

## Phase 3 final baseline

M3c exact final pre-merge head `4fa7d25920523b9e5d8591ae6185e1b5f910c083` passed core #123 and staged #113.

M3c target:
- image 71,608 B
- RAM 35,272 B
- pool 30,720 B
- package SHA `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

M3c emulator:
- queue 28/512, overflow 0, late flags 0
- cursor 123,648
- late trace `811c9dc5`
- WAV peak 0

## Phase 4 String authority

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

### Study-98 acoustic recovery

Foundation:
- low current Gravity root;
- root ±0.55× Drift;
- sine pair;
- `.016 * foundationLevel` source gain;
- dark ~1050 Hz low-pass character.

Classic:
- current Study-126 played String pitch ±0.5× Drift;
- sine pair;
- `.020` each before source level;
- ~1800 Hz low-pass character.

### Study-126 Dual preservation

Study 98 has no Dual/surfaceMode/WIRE. Default Dual therefore keeps Study-126 acoustic identity:
- BODY = independent left String pitch, normalized `[1,.31,.15,.085,.052,.030,.018]` harmonic wave, `.020*leftLevel`, LP ~1550 Hz, pan -0.23;
- WIRE = independent right String pitch, triangle, `.0155*rightLevel`, LP ~3600 Hz, pan +0.23;
- mode changes fade to zero before timbre/filter/pan swap, then fade in.

Persistent voice ceiling stays four:
- 2 Foundation;
- plus 2 Classic sine voices or BODY+WIRE.

## M4a implementation complete

Implemented:
- `apsis_string_dsp.[ch]` fixed-state 44.1 kHz renderer;
- deterministic lookup-table sine, triangle and BODY harmonic oscillator;
- fixed-point frequency/tuning conversion;
- target-safe phase conversion without target-unavailable division helper;
- per-voice one-pole low-pass, pitch/gain smoothing and stereo mix;
- transport-gated sustain;
- smooth mode transition without phase reset;
- live `app_audio()` integration;
- generated single-TU target integration;
- non-zero emulator audio gate while preserving M3c telemetry.

M4a intentionally does **not** yet include:
- Study-98 pluck/root-shadow transient;
- String Rhythm transient modes;
- Attractor excitation;
- dynamic String bus filter modulation;
- Cathedral/Space;
- sample-exact structural-to-audio handoff.

## M4a verified evidence

Host + ASAN/UBSAN:
- Classic peak 1477, deterministic hash `b20610c1`
- Dual peak L/R 823/787, deterministic hash `10d23cf4`
- all M3c runtime/capacity hashes unchanged

Real JieLi target, staged #126:
- image **75,700 B / 581,564 B**
- RAM **35,464 B / 98,304 B**
- pool **30,720 B / 344,064 B**
- package SHA **`eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`**

Delta from M3c:
- image +4,092 B
- RAM +192 B
- pool 0 B

FM-1 emulator, staged #126:
- first intentional APSIS audio: PASS
- peak 317 Q15
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM SHA `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512, overflow 0, late flags 0
- cursor 123,648
- late trace `811c9dc5`

The low dry level is source-derived. Later Mother/master/source-bus/room architecture has not been added yet.

## M4a callback boundary

Current `app_audio()` order:
1. resolve/update String targets from state visible at callback start;
2. render dry String for the callback;
3. advance M3c runtime for that callback.

At 256 frames / 44.1 kHz, a structural event inside a block may become audible at most one callback late (~5.8 ms), never early from future state.

Sample-exact structural-to-sounding-source timing is reserved for Phase 5.

## Performance status

The live-audio emulator smoke took approximately the same wall-clock duration as M3c, so there is no emulator evidence of a major performance regression.

Real physical ISR cycle headroom remains unverified because no FM-1 hardware is available.

Several current 64-bit per-sample intermediates have statically proven 32-bit-safe bounds. Optimization is deferred rather than changing the first frozen audible reference without hardware need; any later optimization must preserve host and emulator audio hashes bit-for-bit.

## Exact next action

1. Finalize `TESTING.md`, `ROADMAP.md`, PR #7 body and this handoff.
2. Run APSIS core + ASAN/UBSAN and full staged real-target/emulator validation on the **exact final documentation head**.
3. If both are green, mark PR #7 ready and squash-merge M4a.
4. Start the next String slice from merged `main`, keeping `M4A_REFERENCE_AUDIO.md` immutable as the persistent-layer baseline.
5. Next String slice: implement source-derived Study-98 `classicStringNote()` transient/root-shadow behavior and String Rhythm excitation separately from the persistent engine.

## Do not regress

- Study 126 remains behavioral/state/timing authority;
- Study 98 is narrow acoustic authority, not a behavior rollback;
- one shared Phase-3 scheduler; no module clocks;
- KEEP remains partial musical memory;
- CHAIN and Mother semantics remain unchanged;
- Spiral/Bloom M3c late binding remains unchanged;
- `stepOneResync` OFF by default;
- no shared Space/Cathedral during initial dry source work;
- do not claim physical hardware or real ISR timing without device evidence.