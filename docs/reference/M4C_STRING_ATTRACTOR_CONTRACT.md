# M4C STRING ATTRACTOR CONTRACT

This document is the normative source/implementation contract for the String Attractor excitation slice after merged M4b.

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
- one shared-heap bind callback per audible side;
- late pitch/level re-read;
- target-delayed use of the M4b transient pool;
- right-side upper-octave Attractor accent.

Explicitly not part of M4c:

- dynamic String bus-filter cutoff modulation;
- String filter sequence audio;
- manual physical-control mapping;
- shared Space/Cathedral;
- sample-exact Phase-5 structural state application unrelated to this already-looked-ahead Attractor path;
- physical FM-1 validation / real ISR timing.

The S&H planner must still advance correctly when Attractor is in filter-only pulse mode even though filter audio is deferred, so later filter work does not start from the wrong cell/cursor state.

## Source state

Study 126 String defaults relevant to Attractor:

- `filterMode = off`;
- `filterAttractorEvery = 1`;
- legal rates: `0.5, 1, 2, 4, 8` Clock intervals;
- `filterAttractorLength = 8`, legal lengths 4/8/12/16;
- `filterAttractorLock = 0.86`;
- `filterAttractorStrum = 0.62`;
- `filterAttractorStrumLevel = 0.78`;
- `filterAttractorSeed = 317`;
- default gates: `[1,0,1,0,1,1,0,0,1,0,1,0,1,0,0,1]`;
- `attractorTarget = alternate`, legal `left/right/alternate/both`.

Native M2 state already contains every listed Attractor field except `attractorTarget`. M4c adds an explicit target enum/field with default `ALTERNATE`; it must not hide target policy inside DSP code.

## Seeded S&H reset

Study 126 `resetStringAttractor()` creates the current-length cell array from a local seeded LCG:

- `x = seed` (minimum 1; source default 317);
- each draw: `x = 1664525*x + 1013904223` modulo 2^32;
- cell value is the resulting uniform value in `[0,1)`;
- reset sets planner cursor to zero;
- changing/rerolling seed regenerates the cells;
- changing length regenerates the cells for the new length;
- Clock step-one resync resets the cursor to zero but does **not** regenerate cell values.

Native cells use Q15. Conversion from the deterministic 32-bit draw to Q15 must be monotonic and tested against a source-derived reference vector.

The local seeded reset generator does not consume the shared M3c random stream.

## Planning order and random mutation

Study 126 calls `clockStringFilter(step, when, interval)` at the beginning of each scheduled Clock iteration, before Spiral Attractor decisions and before the occasional Clock ghost random draw.

Therefore when String Attractor is active, native random draw order is:

1. String Attractor lock/mutation draws for the scheduled pulse (and half-pulse when rate=0.5);
2. Spiral Attractor draws;
3. Clock ghost draw when due;
4. later runtime random consumers in their existing order.

When String Attractor is inactive, M4c must consume **zero** additional shared random draws/events so all frozen M3c/M4b default traces remain bit-identical.

For each S&H advance:

1. `pos = cursor % length`;
2. read current lock;
3. draw one shared random value and compare to lock;
4. if draw > lock, draw a second shared random value and replace the current cell;
5. freeze the resulting cell value `v`;
6. advance planner cursor immediately to `(pos+1)%length`;
7. use the same frozen `v/pos` for optional filter CV and optional excitation.

Gates are not randomized by this per-step mutation path; the current gate mask is sampled for the frozen position.

## Rate and shared-timeline targets

Attractor is driven by the same Clock lookahead as Phase 3.

For rates 1/2/4/8:

- only Clock steps satisfying `step % rate == 0` advance the S&H;
- target sample is the Clock pulse target.

For rate 0.5:

- every Clock pulse advances once at the pulse target;
- it advances a second time at `pulse + interval/2`;
- both targets are inserted into the **shared M3c heap**; no module-local clock/timer exists.

Step-one resync, when enabled, resets the S&H cursor before the step-zero advance.

## Pulse diagnostic gating

Study 126 derives:

- `allowFilter = pulseDiag in {FULL,FILTER}`;
- `allowExcite = pulseDiag in {FULL,EXCITE}`.

Attractor returns without advancing only when neither path is allowed.

M4c behavior:

- FULL: advance S&H and schedule excitation; filter audio remains deferred;
- EXCITE: advance S&H and schedule excitation;
- FILTER: advance/mutate S&H exactly as source but schedule no voice; future filter work may consume the frozen S&H behavior without rewinding state;
- SUSTAIN: no Attractor S&H advance and no additional random draw/event.

## Planning-time excitation freeze

Study 126 `attractorStrum(value, target, pos, gate)` executes at Clock planning time.

If String `sound` is off, the gate is false, or strum amount is effectively zero, no audio bind timer is created.

When an excitation is created, the following values are frozen at planning time:

- target sample;
- S&H position `pos`;
- normalized S&H value `norm`;
- gate result;
- base strum level:
  - `(.050 + .095*norm) * strumAmount * strumLevel`;
- Classic vs Dual surface **at planning time**;
- target-side selection **at planning time**:
  - Classic -> one `CLASSIC` route;
  - Dual LEFT -> left;
  - Dual RIGHT -> right;
  - Dual BOTH -> left + right;
  - Dual ALTERNATE -> left for even `pos`, right for odd `pos`;
- whether the route is one-sided or BOTH; BOTH applies the source ×0.78 level scale to each side.

This is intentionally different from M4b Rhythm, where surface is re-read at bind. Do not reuse Rhythm's observation policy blindly.

## Bind lead and late-read contract

Browser source facts:

- narrow UI (`innerWidth <= 700`): bind timer fires 65 ms before target;
- normal/wide UI: bind timer fires 45 ms before target;
- at fire, WebAudio start is guarded to `max(target, currentTime+8 ms)`.

FM-1 has no browser viewport. Native M4c uses a fixed **45 ms bind lead**, matching the normal/wide source path and the existing native policy of selecting one deterministic non-UI lead.

At bind, re-read current:

- String `sound` flag; if off, skip the voice;
- String source level;
- selected side frequency from current String/Gravity/tuning state;
- selected side level (Classic uses 1.0);
- String brightness;
- Foundation enabled state/root contribution used by the side transient;
- other acoustic state already read by the proven M4b BODY/WIRE trigger.

Do **not** re-evaluate:

- Classic vs Dual routing;
- LEFT/RIGHT/BOTH/ALTERNATE selection;
- S&H value/position/gate;
- base strum amount/strum-level product.

Those are planning-time facts.

## Shared-heap native architecture

M4c must not enlarge or rewrite the frozen `ApsisRuntime` merely to store String-module state.

Add a separate fixed `ApsisStringAttractorRuntime` owned by the String layer. It contains:

- at most 16 Q15 S&H cells;
- planner cursor;
- reset identity (seed/length);
- fixed pending occurrence/bind slots and telemetry.

The existing `apsis_runtime_process()` remains the frozen M3c path. M4c may add an explicit extended process entry point that receives the external String Attractor runtime and internally preserves the same scheduler, heap and event ordering. With a null/inactive Attractor context, behavior and hashes must remain identical to merged M4b.

Append any M4c-only shared event type after existing Phase-3/M4b event values so prior same-sample ordering remains stable.

## Bind slot and target-delayed voice policy

A planned audible Attractor side allocates a fixed pending slot and a shared-heap `STRING_ATTRACTOR_BIND` event at:

`max(0, targetSample - 45 ms)`.

The slot freezes planning-time values and target sample. The bind event carries only slot/token identity; the slot is invalidated after bind/skip.

At bind, M4c resolves current pitch/level and allocates M4b finite voice(s) with an additional start delay equal to the remaining samples until target.

Because the bind lead is ~1985 samples at 44.1 kHz and the audio callback is 256 frames, the bind callback can be observed up to one block late and still normally has more than enough lead to schedule the transient at its exact target sample inside a later render block.

Startup/near-immediate targets that do not have 45 ms of lead remain a documented boundary case and must be deterministic/never early.

## Attractor acoustic topology

### Classic

Classic uses the M4b composite at the current Classic/right frequency:

- left BODY Attractor leg at target, source composite level ×0.66;
- right WIRE/pizz Attractor leg at target +4 ms, source composite level ×0.78;
- Attractor-specific side durations/attack/root contributions below still apply.

### Dual left / BODY

Study 126 Attractor left side:

- duration 0.72 s;
- pan -0.30;
- brightness = current String brightness -0.10 +0.06, clamped to source range;
- attack 30 ms;
- root contribution 0.08 when Foundation is enabled;
- BODY waveform identity is the already-frozen M4b native BODY.

### Dual right / WIRE

Study 126 Attractor right side:

- duration 0.42 s;
- pan +0.30;
- brightness = current String brightness +0.18 +0.06, clamped to source range;
- level into Classic/pizz primitive ×0.92;
- root contribution 0.05 when Foundation is enabled;
- WIRE uses the already-frozen Study-98-derived pizz primitive.

### Right upper-octave accent

When normalized S&H value `norm > 0.52`, every Attractor right/WIRE route adds:

- sine at `freq * 2`;
- level `rightTransientInputLevel * 0.15`;
- start target +6 ms;
- duration `min(0.20 s, 0.42*0.42 s)` = 176.4 ms for the source Attractor right duration;
- pan `+0.30 * 0.72 = +0.216`.

Classic composite also contains a right Attractor leg, so the accent applies there when `norm > 0.52`.

The accent must use the same global 80+32 finite pool/steal ceiling; it does not get a separate voice budget.

## Capacity requirements

Do not guess the pending-slot capacity from the 80-voice audio ceiling.

M4c must stress the worst legal 1.20 s planner horizon at:

- 180 BPM;
- 1/4 Clock division;
- maximum swing;
- Attractor rate 0.5;
- BOTH target;
- all gates on;
- full excitation;
- concurrent existing Spiral/Bloom/Clock events.

Choose a fixed pending-slot count only after measuring the maximum simultaneous planned-but-not-bound Attractor sides plus a deterministic safety margin.

The existing 512-event shared heap must also be re-stressed. Raise capacity only if legal source behavior proves the current bound insufficient; never add an unbounded/module-private queue.

## Acceptance gates

### M4c1 — state + silent planner

1. Add explicit `attractorTarget` state enum/field, default ALTERNATE.
2. Source-seeded LCG produces deterministic 4/8/12/16-cell reference vectors.
3. Step-one resync resets cursor but not cells.
4. Rate 0.5 produces pulse + half-pulse targets on the shared timeline; rates 1/2/4/8 match source step selection.
5. FILTER-only advances/mutates cells with no voice bind; SUSTAIN consumes no Attractor random/events.
6. Attractor random draws precede Spiral/ghost draws when active.
7. Inactive/default Attractor preserves all M3c/M4b random/late/queue/audio hashes exactly.
8. Worst legal horizon establishes fixed pending-slot and heap capacity with zero overflow/starvation.
9. Normal + ASAN/UBSAN pass before audio bind integration.

### M4c2 — late bind + finite excitation

1. Planning freezes surface/target-side/value/position/base-level as source specifies.
2. Bind at native target−45 ms re-reads current String sound/pitch/source-level/side-level/brightness/Foundation state.
3. Turning String off after planning but before bind skips the voice; turning it on after a plan-time skip does not resurrect a missing bind.
4. Pitch/side-level changes after planning but before bind affect the sounding voice.
5. Classic/Dual/LEFT/RIGHT/BOTH/ALTERNATE routing changes after planning do not reroute an already-planned occurrence.
6. BODY/WIRE durations, pan, brightness offsets, root contributions and Classic composite scales match Study 126.
7. Right `norm > 0.52` accent matches frequency/level/+6 ms/pan/duration rules.
8. Target-delay rendering is deterministic, never early, and sample-exact at target when normal 45 ms lead is available.
9. Existing M4b Rhythm and M4a sustain references remain unchanged when Attractor is inactive.
10. Shared heap/pending slots/transient pool show zero overflow/drop under legal stress.
11. Real JieLi target and emulator regression pass.

## Deferred beyond M4c

- audible dynamic String filter S&H modulation and filter sequence;
- UI/display cursor rendering;
- manual physical controls for Attractor target/rate/reroll/gates;
- shared Space/Cathedral;
- physical FM-1 listening and ISR-cycle evidence.
