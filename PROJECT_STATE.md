# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4b — finite dry String transients.**

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7

Active work:
- branch `m4b-string-transients`
- base `2ea217f09c143d30385f136e84e2b29d582987a0`
- current first gate: pure transient engine before transport/runtime audio integration

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic authority:
- Study 98 · Performance Architecture where the acoustic feature existed there
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- later features absent from Study 98 remain Study-126 behaviour/acoustics unless explicitly source-resolved otherwise

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

Physical FM-1: **not tested**.

## M3c frozen runtime baseline

M3c final pre-merge head `4fa7d25920523b9e5d8591ae6185e1b5f910c083` passed core #123 and staged #113.

Target:
- image 71,608 B
- RAM 35,272 B
- pool 30,720 B
- package SHA `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Emulator:
- queue 28/512, overflow 0, late flags 0
- cursor 123,648
- late trace `811c9dc5`
- WAV peak 0

## M4a frozen persistent String baseline

M4a final pre-merge head `afe81ae3984797bcb3b99153c96b493dfb8575ec` passed:
- APSIS core #141 including ASAN/UBSAN
- staged validation #130 through real JieLi target + FM-1 emulator

Merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

Persistent architecture:
- 2 Study-98 Foundation voices;
- plus 2 Study-98 Classic sine voices or Study-126 BODY/WIRE;
- fixed 44.1 kHz stereo Q15;
- transport-gated sustain;
- no shared Space.

Frozen host references when no transient exists:
- Classic peak 1477, hash `b20610c1`
- Dual peak L/R 823/787, hash `10d23cf4`

Final target:
- image 75,700 B / 581,564 B
- RAM 35,464 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA `eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`

Final emulator:
- peak 317 Q15
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM SHA `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- M3c queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

The M4a references are regression anchors. M4b must not alter them when its transient pool is idle.

## M4b authority

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

### Study-98 low-level transient DNA

`classicStringNote()`:
- normal harmonics 1,2,3,4;
- pizz harmonics 1,2,3;
- normal roll exponent `1.28 + (1-brightness)*0.58`;
- pizz exponent 1.52;
- amplitude `level * scale * resonance / h^roll`;
- 3 ms stagger per harmonic index;
- normal duration multiplier `1 + hi*0.16`;
- pizz duration multiplier `1 - hi*0.09`;
- first two harmonics triangle, higher harmonics sine;
- fundamental uses requested pan; upper harmonics use `pan * -0.45`.

Generic Study-98 `tone()` mechanics:
- triangle low-pass `clamp(freq*2.35,1600,16000)`, Q .32;
- sine bypasses this filter;
- exponential-style attack/decay envelope;
- oscillator lives ~40 ms beyond envelope end.

Safety override:
- current Study 126 String onset is 16 ms, replacing Study-98's older 10 ms String onset.

Root shadow:
- current committed Gravity root;
- fold down octaves until `root <= playedFrequency*1.2`;
- fire the same partial stack at `rootGain` scale;
- Study-98 manual reference 0.28 when Foundation is enabled;
- Study-98 pizz Rhythm reference 0.18.

### Study-126 topology and safety remain authoritative

Current performance topology:
- Classic direct pluck uses composite transient;
- composite left at t ×0.66, right at t+4 ms ×0.78;
- Dual left/BODY is longer bowed role, right/WIRE shorter pizz role;
- Rhythm side alternates by Clock step parity in Dual;
- Attractor re-reads pitch/side level near fire time.

Current safety:
- broadband random-noise String exciter stays removed;
- no hard scene-gain step on Clock pulse;
- logical transient ceiling in browser is 80;
- stolen audible voice fades toward zero with 14 ms time constant before stop 105 ms later;
- native fixed pool must explicitly account for those fade tails rather than immediately reusing an audible slot.

## Shared-timeline integration seam

Phase-3 transport already emits `APSIS_EVENT_STRING_RHYTHM`.

Source-derived native event facts:
- transport schedules it at Clock pulse +26 ms;
- event carries Clock step / absolute pulse / shared flags;
- trigger selection (EVERY vs SEQUENCE) already happens in transport;
- M4b must consume this event, not add a String-private scheduler.

Current M4a callback order remains:
1. update/render String from state visible at callback start;
2. advance M3c runtime through the block.

Do not claim sample-exact structural-to-audio handoff in M4b; Phase 5 owns that larger callback architecture change.

## M4b staging

### M4b1 — pure transient engine

Implement/test before any transport event becomes audible:
- finite allocation-free transient pool;
- source-derived normal + pizz partial primitives;
- fixed-point brightness-dependent attenuation;
- sample-referenced 3 ms staggering;
- 16 ms de-click onset;
- triangle filtering / sine bypass;
- Gravity-root shadow;
- stereo pan topology;
- bounded fade-tail-safe voice stealing;
- pool high-water/steal/drop telemetry;
- idle M4a hashes unchanged;
- normal + ASAN/UBSAN.

### M4b2 — String Rhythm integration

Only after M4b1 is green:
- consume existing `APSIS_EVENT_STRING_RHYTHM`;
- preserve pulse+26 ms timing and Study-126 Classic/Dual topology;
- preserve M3c runtime/capacity hashes;
- target + emulator reference.

### Later gate — Attractor

Attractor excitation is separate because its source contract re-reads String pitch/side state near the target. Specify native lead/bind semantics before making it audible.

## Exact next action

1. Extend `ApsisStringDsp` with a separate finite transient pool while leaving the four M4a sustain voices untouched.
2. Add source-derived target/envelope test vectors before live trigger integration.
3. Choose a physical pool + fade-tail reserve only after calculating/stressing legal overlap; do not equate browser logical ceiling 80 with physical native slots.
4. Implement normal/pizz/root-shadow rendering without `libm` or dynamic allocation.
5. Prove M4a idle hashes remain bit-identical and run ASAN/UBSAN.
6. Only then connect the existing String Rhythm transport event.

## Do not regress

- Study 126 remains behavioral/state/timing/safety authority;
- Study 98 is narrow acoustic authority, not a behavior rollback;
- M4a reference hashes are immutable while transient pool is idle;
- one shared Phase-3 scheduler; no module clocks;
- KEEP / CHAIN / Mother semantics unchanged;
- Spiral/Bloom M3c late binding unchanged;
- `stepOneResync` OFF by default;
- no broadband String noise;
- no hard-stop of audible transients;
- no shared Space/Cathedral during initial dry source work;
- do not claim physical hardware or real ISR timing without device evidence.