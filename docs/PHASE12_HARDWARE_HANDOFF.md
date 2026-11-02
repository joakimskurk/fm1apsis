# Phase 12 — physical FM-1 hardware validation handoff

Last updated: 2026-11-02

## Status

Phase 11 is complete and squash-merged to `main` via PR #21.

- Phase-11 merge commit: `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c`
- exact Phase-11 merge-candidate head: `0b08288b5bf900aa61c81075b4509c6553510db1`
- active PR regression matrix on that exact head: **18/18 PASS**
- final P11Z closure run: `41403698227` — **PASS all 8 gates**
- P11Z closure artifact: `13123041882`
- artifact SHA-256: `bfe62013f029c395d6d7fc3bba5fca32328cb316a6a6f6cd86859b8a1d862f75`

P11W remains the accepted production firmware baseline. Phase 12 is **not** another speculative software-optimization phase. It exists to replace emulator/static-target assumptions with measurements from a real M-Vave FM-1.

Physical hardware is not yet available, so every Phase-12 item below remains unverified.

## Frozen software authority entering Phase 12

Product identity:
- `FM-1_972`

Production target:
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**
- RAM headroom **10,780 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines

Frozen idle emulator authority:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor **123,392**
- queue high-water **28/512**
- queue overflow **0**
- late flags **0**
- late trace `811c9dc5`

Frozen active String Motion authority:
- active target image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- seen `ff`; final `14`
- points **149/149**
- record **220,672 -> 441,088**
- cycle **220,416 samples**
- manual override **450,560**
- resume **466,176**
- cursor **481,024**
- queue **30/512**
- overflow **0**
- late flags **0**
- WAV **521,984 frames**, peak **1,171**, RMS **173**

These values are the software comparison authority. Phase 12 may discover physical limitations, but it must not silently relax these software invariants to make hardware tests pass.

## Phase-12 validation order

The order matters. Do not begin acoustic tuning or performance optimization before install/recovery and electrical/runtime safety are established.

### P12A — safe install and rescue path

Goal: prove that APSIS can be installed on a real FM-1 and that the device can be recovered back to known-good official firmware.

Required evidence:
- identify exact physical unit/revision and USB behavior;
- preserve/download any recoverable stock image or document the official recovery source;
- perform APSIS install using the repository-supported path;
- cold boot and repeated warm reboot;
- prove a repeatable rescue/back-to-official path;
- reinstall APSIS after rescue;
- capture exact firmware file SHA-256 used on hardware;
- record every command/tool/version needed for reproduction.

Hard gate: **no destructive persistence/power-cut tests until rescue is proven.**

### P12B — basic panel/display/audio bring-up

Goal: prove the emulator-visible firmware actually maps correctly to the physical panel and audio hardware.

Check all ten surfaces:
- MOTHER
- GRAVITY
- STRING
- DRIFT/TUNING
- CLOCK
- SPIRAL
- BLOOM
- KEEP
- CHAIN
- SPACE/CATHEDRAL

Verify:
- all 14 buttons;
- all 7 encoders, direction and detent behavior;
- all 27 note keys;
- MASTER/platform volume behavior;
- navigation gestures including SEL+FX;
- LEDs if exposed by the platform path;
- 240x240 display orientation, strip updates, tearing, stale rows and takeover timing;
- no unexpected stock-UI remnants after native takeover;
- stereo DAC output, channel orientation and gross level sanity.

Evidence should include photos/video plus a written pass/fail table.

### P12C — real audio callback / ISR timing

Goal: replace Phase-11 static workload proxies with real timing evidence.

Measure on the physical target, preferably with a test-only instrumentation build that does not change production musical behavior:
- `app_audio()` callback period;
- callback execution high-water;
- average and worst observed execution time;
- deadline margin;
- deadline misses/underruns if the platform exposes them;
- separate idle and deliberately worst-case musical loads;
- warm Cathedral state, not only cold startup;
- active String Motion plus dense scheduler/runtime activity;
- simultaneous display activity where possible.

Required scenarios should include at minimum:
1. frozen idle baseline;
2. dense Clock/Spiral/Bloom activity;
3. warm Cathedral with demanding sends/decay;
4. active String Motion lifecycle;
5. combined worst-case audio + controls + display redraw.

Do not infer cycle margin from ELF size or emulator wall-clock time.

### P12D — runtime stack high-water

Goal: measure real user/system stack use rather than treating linker reservations as usage.

Measure:
- user stack high-water;
- system/interrupt stack high-water if separately observable;
- remaining guard margin under the same worst-case scenarios as P12C;
- cold boot, long-run and repeated interaction cases.

No stack reservation may be reduced merely because the configured region appears large.

### P12E — LCD/audio/control contention

Goal: prove the native 30-strip redraw path does not destabilize audio or interaction on the real bus/DMA implementation.

Stress:
- continuous surface navigation;
- parameter edits that trigger redraws;
- simultaneous note/performance input;
- active Motion playback/override;
- warm Cathedral;
- dense scheduler events.

Observe:
- audio glitches/underruns;
- missed or delayed control events;
- display tearing/stall;
- LCD DMA completion behavior;
- callback timing delta with display idle vs busy.

### P12F — persistence and real NOR behavior

Goal: validate the Phase-7 journal and deferred-write assumptions against actual NOR timing/failure modes.

First measure normal behavior:
- boot read timing;
- save/erase/program duration;
- repeated save cycles;
- newest-generation selection;
- reboot persistence.

Only after P12A rescue is proven, perform controlled interruption testing:
- interrupt during payload write;
- interrupt before commit header;
- corrupt/torn newest record fallback;
- generation wrap behavior where practical;
- verify older valid generation survives.

Audio must remain quiesced/stopped for erase/program exactly as the frozen contract requires.

### P12G — listening and DAC validation

Goal: validate what software hashes cannot establish.

Check through both measurement and listening where practical:
- stereo orientation;
- unexpected DC/clicks/zipper noise;
- output level/headroom;
- noise floor and obvious clipping;
- String/Bloom transient behavior;
- Drift/Clock/Spiral tonal balance;
- Cathedral decay/damping/modulation across its useful range;
- mono compatibility if relevant to intended use;
- subjective Cathedral tuning only after the technical timing path is stable.

Any perceptual tuning change must be treated as a new musical change with explicit before/after evidence; it is not automatically authorized by Phase 12.

### P12H — release candidate freeze

Only after P12A-G pass:
- build the exact release candidate from a clean checkout;
- rerun the software regression authority;
- rerun the physical smoke/stress subset on the release binary;
- freeze release firmware SHA-256;
- freeze install/rescue instructions;
- record hardware revision(s) validated;
- document known limitations;
- tag/release only from the exact validated commit.

## Hardware evidence format

For every physical test, record:
- date;
- physical FM-1 hardware/revision identifier;
- firmware commit and firmware SHA-256;
- instrumentation commit if different;
- tool/installer version;
- exact scenario and duration;
- measured values;
- pass/fail result;
- photos/video/logs where useful;
- whether the test changes any production source.

Prefer additive test-only instrumentation and evidence artifacts. Do not mix measurement instrumentation into production code unless it is independently justified.

## Stop conditions

Stop and diagnose before proceeding if any of these occur:
- install/recovery path is not repeatable;
- boot instability or unexplained resets;
- audio underrun/glitch under normal interaction;
- callback deadline margin is inadequate or unknown under worst-case load;
- stack guard is touched or margin is unclear;
- display activity materially destabilizes audio;
- persistence can corrupt both journal generations;
- controls do not match the frozen panel vocabulary;
- production audio differs unexpectedly from the validated software behavior.

Do not compensate by weakening regression gates or reducing feature parity without a separately documented decision.

## Exact next action

**Wait for a physical M-Vave FM-1.** When hardware is available, begin with **P12A safe install + rescue/back-to-official proof** using the Phase-11 merged baseline `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c` and production package authority `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`.

Until then, do not reopen speculative pre-hardware optimization. Phase 11 is closed.