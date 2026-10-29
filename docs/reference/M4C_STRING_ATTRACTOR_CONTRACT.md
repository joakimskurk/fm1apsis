# M4C STRING ATTRACTOR CONTRACT

This document is the normative source/implementation contract for String Attractor excitation after merged M4b.

M4b is merged to `main` as `c96c00debf9a67948f1e874e36b090999dacadb9`. Its persistent + finite String references, shared Rhythm integration, pool behavior and M3c regression anchors are frozen.

## Authority hierarchy

1. **Study 126 remains behavioural/timing/safety authority.** It owns Attractor S&H progression, planning order, rate, lock mutation, gates, target selection, late re-read behavior, BODY/WIRE topology, pulse isolation and later de-click fixes.
2. **M4b remains acoustic/finite-voice implementation authority.** M4c reuses the proven 80+32 finite pool and BODY/WIRE primitives; it does not create a second transient engine.
3. **Study 98 remains narrow acoustic authority** only where M4b already uses its Classic/pizz partial primitive.
4. Native FM-1 may replace browser timers/WebAudio scheduling with the shared sample heap and fixed-point state, but must not create a String-private clock or change which values are frozen vs late-read.

## Scope

M4c implements **String Attractor excitation** only:

- deterministic native S&H cell state and cursor;
- source rate/lock/gate/target semantics;
- source planning-time decision on the existing shared Clock lookahead;
- shared-heap bind callbacks;
- late pitch/level re-read;
- target-delayed use of the M4b transient pool;
- right-side upper-octave Attractor accent.

Deferred beyond M4c:

- audible dynamic String bus-filter cutoff modulation;
- String filter sequence audio;
- UI/display cursor rendering and manual physical-control mapping;
- shared Space/Cathedral;
- unrelated Phase-5 sample-exact structural state application;
- physical FM-1 validation / real ISR timing.

The S&H planner still advances correctly in FILTER-only mode even though filter audio is deferred, so later filter work cannot start from the wrong cell/cursor state.

## Source state

Study 126 defaults relevant to Attractor:

- `filterMode = off`;
- `filterAttractorEvery = 1`; legal rates `0.5, 1, 2, 4, 8` Clock intervals;
- `filterAttractorLength = 8`; legal lengths 4/8/12/16;
- `filterAttractorLock = 0.86`;
- `filterAttractorStrum = 0.62`;
- `filterAttractorStrumLevel = 0.78`;
- `filterAttractorSeed = 317`;
- default gates `[1,0,1,0,1,1,0,0,1,0,1,0,1,0,0,1]`;
- `attractorTarget = alternate`; legal `left/right/alternate/both`.

M4c adds an explicit native `attractorTarget` enum/field with default `ALTERNATE`; target policy is not hidden in DSP code.

## Seeded S&H reset

Study 126 `resetStringAttractor()`:

- starts `x = seed` with source default 317;
- advances `x = 1664525*x + 1013904223` modulo 2^32;
- converts each draw to one cell value in `[0,1)`;
- resets planner cursor to zero;
- regenerates cells on seed/length reset;
- step-one resync resets cursor only, not cell values.

`audio.start()` calls `resetStringAttractor()` before `startScheduler()`, so each new transport run regenerates cells deterministically. A mid-run seed/length reset regenerates cells/cursor but does not cancel already-created Attractor voice timers. `audio.stop()` cancels pending Attractor timers.

Native cells use Q15. The local seeded reset generator consumes no shared M3c RNG.

## Planning order and mutation

Study 126 calls `clockStringFilter(step, when, interval)` before Spiral Attractor decisions and before the occasional Clock ghost random draw.

Active native draw order is therefore:

1. String Attractor lock/mutation draw(s), including the half-pulse when rate=0.5;
2. Spiral Attractor draw(s);
3. Clock ghost draw when due;
4. later runtime consumers in their existing order.

Inactive/default Attractor consumes zero additional shared random draws/events.

For each S&H advance:

1. `pos = cursor % length`;
2. draw once and compare against current lock;
3. if draw > lock, draw a second time and replace the current cell;
4. freeze resulting value `v` and current gate;
5. advance cursor immediately;
6. use the same frozen `v/pos` for optional filter CV and optional excitation.

Gates are not randomized by the per-step mutation path.

## Rate and shared timeline

For rates 1/2/4/8, only Clock steps satisfying `step % rate == 0` advance the S&H and target the Clock pulse sample.

For rate 0.5, every Clock pulse advances at:

- the pulse target;
- `pulse + interval/2`.

Both use the **shared M3c heap**. No String-private clock or queue exists.

Step-one resync, when enabled, resets the S&H cursor before the step-zero advance.

## Pulse diagnostic gating

Study 126 derives:

- `allowFilter = pulseDiag in {FULL,FILTER}`;
- `allowExcite = pulseDiag in {FULL,EXCITE}`.

M4c behavior:

- FULL: advance/mutate and schedule excitation; filter audio deferred;
- EXCITE: advance/mutate and schedule excitation;
- FILTER: advance/mutate, no voice;
- SUSTAIN: no Attractor advance, random draw or event.

## Planning-time excitation freeze

`attractorStrum(value,target,pos,gate)` runs at Clock planning time. If String sound is off then, gate is false, or strum amount is effectively zero, no audio bind is created.

The following values freeze at planning time:

- target sample;
- S&H position/value/gate;
- base strum level `(.050 + .095*norm) * strumAmount * strumLevel`;
- Classic vs Dual surface;
- target route:
  - Classic -> CLASSIC;
  - Dual LEFT -> left;
  - Dual RIGHT -> right;
  - Dual BOTH -> left + right;
  - Dual ALTERNATE -> even pos left, odd pos right;
- whether BOTH scaling applies; each BOTH side gets source ×0.78.

This intentionally differs from M4b Rhythm, whose surface is late-read.

## Bind lead and late-read contract

Browser source facts:

- narrow UI: 65 ms pre-target timer;
- normal/wide UI: 45 ms pre-target timer;
- WebAudio start guard `max(target,currentTime+8 ms)`.

FM-1 has no viewport, so native M4c uses fixed **45 ms** bind lead.

At bind, re-read current:

- String `sound`; off skips voice;
- String source level;
- selected side frequency from current String/Gravity/tuning state;
- selected side level; Classic uses 1.0;
- current String brightness as actually consumed by the selected acoustic path;
- Foundation state/root contribution used by WIRE.

Do not re-evaluate planning-time surface/route/value/position/gate/base level.

## Shared-heap architecture

M4c does not enlarge the frozen `ApsisRuntime` for String-module state.

`ApsisStringAttractorRuntime` is external and contains:

- max 16 Q15 cells;
- planner cursor and reset identity;
- 64 fixed pending side slots;
- six per-process completed-binding outputs;
- Attractor-specific telemetry.

`APSIS_EVENT_STRING_ATTRACTOR_BIND` is appended after existing Phase-3/M4b event values. It is not added to the frozen `ApsisRuntime.event_count[]`; `sizeof(ApsisRuntime)` remains **21,928 B**.

The original `apsis_runtime_process()` remains the frozen M3c path. Explicit M4c entry points extend the same scheduler/heap when an Attractor runtime is active.

## Bind slot and target-delayed voice policy

Each planned audible side gets a fixed pending slot and shared-heap bind event at:

`max(0, targetSample - 45 ms)`.

At bind, M4c resolves current state and allocates existing M4b finite voice(s) with a pool start delay equal to remaining samples until the frozen target.

With normal 45 ms lead and 256-frame callbacks, bind can be observed up to one callback late while still leaving enough lead to hit the musical target sample exactly. Startup/near-immediate clamped binds are deterministic and never early.

Target smoke preserves the frozen M4b startup path while Attractor has never been active: external Attractor BSS remains zero and the original M4b observer path runs. First real Attractor planning lazily generates cells from seed. Once initialized, the M4c consumer stays available even if filter mode later turns OFF so already-planned bind events can still complete.

## Attractor acoustic topology

### Classic

Classic uses current Classic/right frequency:

- BODY at target, composite level ×0.66;
- WIRE/pizz at target +4 ms, composite level ×0.78;
- right accent, when applicable, starts another +6 ms after WIRE = target +10 ms total.

### Dual left / BODY

Audible source behavior:

- duration 0.72 s;
- pan -0.30;
- bowed brightness input = current String brightness −0.10 +0.06 = current −0.04, bounded by the bowed path;
- attack 30 ms;
- frozen M4b BODY waveform identity;
- **no root shadow**: Study 126 passes `rootGain=.08` in options, but bowed `stringSolo()` never reads it.

### Dual right / WIRE

Audible source behavior:

- duration 0.42 s;
- pan +0.30;
- level into the M4b pizz primitive ×0.92;
- root contribution 0.05 when Foundation is enabled;
- WIRE uses the frozen Study-98-derived pizz primitive;
- **audible brightness is the current base `state.string.brightness`.** `stringSideTransient()` computes and passes a +0.24 brightness option, but `classicStringNote()` never reads `opts.brightness`; native follows the sounding source rather than the unused caller argument.

### Right upper-octave accent

When frozen `norm > 0.52`, each right/WIRE route adds:

- sine at `freq * 2`;
- level `rightTransientInputLevel * 0.15` before WIRE's internal ×0.92;
- +6 ms from right-route start;
- 176.4 ms duration;
- pan +0.216;
- generic String 16 ms onset;
- sine/no-low-pass behavior.

The accent shares the same global 80 logical +32 fade-tail finite pool and steal policy.

## Frozen M4c1 capacity evidence

Worst legal 1.20 s planner-horizon stress:

- 180 BPM, 1/4 division, max swing;
- rate 0.5, BOTH, all gates, full excitation;
- maximal Attractor mutation draw load;
- concurrent String Rhythm, Spiral rate 4, Bloom and Clock events;
- 512-frame process blocks for extra burst margin;
- deterministic 60-second run.

Measured:

- shared heap **415 / 512**;
- planned-but-not-bound sides **60 / 64**;
- per-process binding-output bound **6**; normal 256-frame startup requires at most 4;
- S&H advances 1470;
- mutations 1470;
- bind callbacks 2884;
- random trace `b7a9a395`;
- late trace `e777a771`;
- zero pending/binding/heap/Spiral/Bloom overflow or starvation;
- normal + ASAN/UBSAN agree.

512 shared events remain sufficient with 97 measured worst-case headroom.

## Frozen M4c2 live/audio evidence

Host tests prove:

- planning-time route/value/base facts remain frozen;
- bind-time sound/pitch/source-level/side-level/brightness/Foundation are late-read;
- BODY/WIRE/Classic/accent topology matches the actual sounding Study-126 code;
- full shared timeline -> target−45 ms bind -> finite-pool delay is never early and is exact at target under normal lead.

Full-chain reference:

- target sample 36,750;
- bind-processing block ends at 34,816;
- remaining pool delay 1,934 samples;
- voice age remains 0 through the full delay and advances first on the target sample.

All host suites pass normal + ASAN/UBSAN.

## Target / emulator evidence

Validated implementation head: `e2171caac10eb402e97a6d566eff71a27544e3fc`.

- APSIS core #198: PASS normal + ASAN/UBSAN;
- staged validation #187: PASS all five gates, real JieLi target + FM-1 emulator;
- image **85,564 / 581,564 B**;
- RAM **44,680 / 98,304 B**;
- pool **30,720 / 344,064 B**;
- package SHA-256 `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`;
- emulator queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`;
- peak 317; RMS L/R 101/100; stereo delta 63; mean L/R −2/−2;
- default Attractor OFF is bit-identical to frozen M4a/M4b PCM:
  `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`.

An earlier target-integrated head changed only the smoke PCM startup phase while Attractor was OFF. That result was rejected. The final target preserves the exact frozen M4b observer/startup path until first actual Attractor activation, restoring the immutable PCM hash above.

## Acceptance gates

### M4c1 — state + silent planner

1. [x] explicit Attractor target state/default;
2. [x] deterministic source-seeded LCG vectors;
3. [x] cursor-only step-one resync;
4. [x] 0.5 pulse + half-pulse and source 1/2/4/8 selection;
5. [x] FILTER-only advance and SUSTAIN no-op;
6. [x] Attractor random draws precede Spiral/ghost;
7. [x] inactive/default preserves frozen M3c/M4b behavior;
8. [x] measured fixed slot/heap capacity with zero overflow;
9. [x] normal + ASAN/UBSAN.

### M4c2 — late bind + finite excitation

1. [x] planning freezes route/value/position/base level;
2. [x] target−45 ms bind late-reads sound/pitch/source/side/brightness/Foundation;
3. [x] bind-time mute skips; planning-time skip cannot be resurrected;
4. [x] pitch/side-level changes before bind affect sounding voice;
5. [x] later surface/target changes do not reroute planned occurrence;
6. [x] BODY/WIRE/Classic durations, pan, effective brightness/root behavior and scales match source;
7. [x] right `norm > 0.52` octave accent matches source;
8. [x] pool delay is deterministic, never early and sample-exact at normal target;
9. [x] M4a/M4b references remain unchanged when Attractor is inactive;
10. [x] shared heap/pending/binding/transient pools show zero legal-stress overflow/drop;
11. [x] real JieLi target and emulator regression pass.

## Deferred beyond M4c

- audible dynamic String filter S&H modulation and filter sequence;
- UI/display cursor and manual physical Attractor controls;
- shared Space/Cathedral;
- physical FM-1 listening and ISR-cycle evidence.
