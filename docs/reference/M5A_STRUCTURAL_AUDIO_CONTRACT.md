# M5a structural-audio handoff contract

Status: source-audit freeze before implementation.

Base: merged M4g `713a1e9ac746a1a4d19ba825a0e6765b337ba410`.

Behavioural/state/timing authority: locked Study 126. M3c remains the authoritative shared scheduler and state-observation timeline. M5a only makes already-defined structural timing audible in the five dry engines.

## 1. Scope

Phase 5 exists because the merged dry-engine target still renders each 256-frame callback from state visible at block start and advances structural runtime afterwards. That preserves “never early” behaviour but can make a state-dependent dry source up to one callback late.

M5a removes that block-quantisation where Study 126 provides an explicit audible target without changing musical scheduling.

M5a must not:
- create another musical clock or event queue;
- change M3c event ordering, random draws, lookahead or late-binding semantics;
- grow `ApsisRuntime`;
- copy the entire `ApsisState` into an audio queue;
- add Space/Cathedral;
- rebaseline the five dry source identities;
- spend the remaining 10,968 B static-RAM headroom casually.

## 2. Critical distinction: state-observation time vs audible-target time

Study 126 intentionally performs several structural callbacks before their musical target. Those early callbacks mutate shared state so later callbacks can observe the new structure, while WebAudio automation may still be scheduled for the future musical target.

Native M5a therefore needs two views:
1. **structural state** — existing `ApsisState`, mutated at the already-frozen M3c observation event;
2. **persistent audible target** — compact String/Drift DSP targets that become active at the source-defined audio target sample.

Finite Clock/String/Spiral/Bloom voices already carry exact target/sample-delay information and do not need a second musical scheduler.

## 3. Quantized Gravity

Study 126 quantized Gravity:
- chooses a future Gravity target;
- calls `commitPendingGravity()` approximately target−18 ms;
- clears pending and mutates `state.frame.degree` / voicing at that early callback;
- calls `applyGravityCandidate(candidate, at)`;
- `applyGravityCandidate` calls `audio.refreshDrone(at)`.

Therefore:
- shared structural state becomes committed at **target−18 ms**;
- Spiral/Bloom callbacks after that point must see committed Gravity according to the existing M3c rules;
- persistent String/Drift target calculation uses the newly committed state at the early callback;
- those persistent targets become audible at the original Gravity **target sample**, not at target−18 ms.

Immediate Gravity has no future target separation: structural and audible target are current/native input time.

M5a does not alter Bloom's existing ordinary-pending future projection or Spiral's deliberate lack of pending projection.

## 4. Chain discrete apply and morph

Study 126 `scheduleChainPulse()` invokes `advanceChainAtPulse(when, clockStep)` approximately **24 ms before** `when`.

When a cell applies:
- discrete lane state is mutated by `applySnapshot(..., audioTime=when)` during the early callback;
- `applySnapshot` calls `audio.refreshDrone(audioTime)`, scheduling persistent String/Drift changes for the true target `when`;
- if Chain morph is nonzero, continuous String tension and Drift amount are skipped during the discrete snapshot and `morphValue()` starts at `audioTime=when`;
- Chain meter `n` remains the frozen startChain snapshot already implemented by M3b/M3c.

Therefore:
- Chain discrete state remains visible to structural/late-binding logic from **target−24 ms**;
- persistent String/Drift discrete targets derived from that state switch their target at the true Chain target sample;
- continuous tension/Drift morph begins at the true target sample and follows the existing source smoothstep over the source-defined duration.

## 5. Mother `->1`

Study 126 `scheduleMotherPulse()` invokes `motherMaybeCommitPulse(when, ...)` approximately **26 ms before** `when`.

At commit:
- queued structural state is applied early;
- pending Gravity is applied through `applyGravityCandidate(gravity, atTime)` and therefore schedules persistent drone change for the target;
- Mother mix/mute controls call `updateMotherMix(..., atTime)` and `updateAllMotherMix(atTime)`, so their audible change is target-scheduled;
- some generic structural `applyParam()` paths request a RAF/current-time drone refresh instead of using `atTime`.

Native parity disposition for M5a:
- preserve the source's early **structural visibility** at target−26 ms;
- pending Gravity and Mother mix/mute are explicitly target-scheduled, matching the source;
- generic held structural parameters are not silently assigned one universal audible rule. They require field-specific tests/disposition before M5a is closed;
- where browser RAF timing is only incidental engine jitter and the musical intent is a `->1` boundary, native may use the target boundary only after that disposition is documented in this file/DECISIONS.

## 6. Spiral and Bloom

M5a does not redesign their late-binding engines.

Spiral:
- occurrence/energy/route index and acoustic snapshot remain planning-time data;
- existing M3c bind at target−80 ms remains authoritative for committed Gravity/register/tuning pitch;
- prepared finite DSP remains targeted to the original note-on sample.

Bloom:
- existing M3c occurrence target−45 ms freezes plan/acoustic geometry;
- each existing causal petal bind uses `max(occurrence, petalTarget−50 ms)`;
- `ApsisBloomLateBinding` remains the authoritative target-aware Gravity/tuning result;
- finite DSP remains targeted to the original petal sample.

Phase-5 tests must prove that structural edits near these bind points change the **sounding** finite voice exactly when the M3c contract says they should, without introducing another pitch resolver.

## 7. Persistent-audio automation representation

M5a will use compact precomputed persistent DSP targets rather than whole-state snapshots.

A pending automation entry needs only:
- target sample;
- serial/order;
- `ApsisStringDspTargets` plus the small String smoothing/timbre metadata required to apply them exactly;
- `ApsisDriftDspTargets`;
- validity/source flags as required by tests.

The queue is fixed-capacity and external to `ApsisRuntime`.

Rationale:
- the early callback already has the correct post-commit shared state;
- `apsis_string_dsp_targets()` and `apsis_drift_dsp_targets()` turn that state into the exact persistent acoustic targets;
- storing targets avoids duplicating the large shared state graph;
- entries live only from an early observation callback until the nearby true target, normally 18–26 ms;
- same-target entries retain event order; latest applied target naturally wins after sequential application.

Capacity must be measured under legal worst-case structural scheduling before it is frozen. No guessed large pool.

## 8. DSP API rule

String and Drift must expose target-application helpers so the exact same target calculations are usable both by ordinary live update and by future-target automation.

Refactoring rule:
- existing M4 host hashes must remain bit-identical when no structural automation is pending;
- `apsis_string_dsp_update()` remains behaviourally equivalent, implemented through the same target-apply primitive;
- `apsis_drift_dsp_update()` remains behaviourally equivalent, implemented through the same target-apply primitive;
- no new oscillator/filter algorithm is introduced.

## 9. Render segmentation rule

When a persistent automation target falls inside an audio callback:
1. render all active dry engines from callback start up to that exact target sample using the old persistent targets;
2. apply the queued String/Drift targets at that sample;
3. continue rendering the remaining frames with the existing String/Drift smoothing laws starting from that boundary;
4. finite Clock/String/Spiral/Bloom voices remain on their existing sample-delay/target model.

A target exactly equal to callback end belongs to the next callback, matching the runtime convention that queue events are consumed when `event.sample < end`.

## 10. Planned M5a slices

### M5a1 — pure persistent target queue
- compact String/Drift snapshot type;
- fixed-capacity ordered queue;
- same-target ordering/replacement behaviour;
- segmented host render helper;
- no runtime integration yet;
- measure queue high-water before capacity freeze.

### M5a2 — runtime structural observer
- layer above M4g without changing M3c/M4g semantics;
- observe post-handler `GRAVITY_COMMIT`, `CHAIN_APPLY_OBSERVE`, and selected `MOTHER_COMMIT_OBSERVE` effects;
- use `event.interval_q32` as the original target sample for runtime-only observation events;
- capture persistent DSP targets after the structural state mutation;
- no extra shared event or random draw.

### M5a3 — target integration
- apply queued persistent targets at exact sample boundaries inside `app_audio()`;
- preserve all M4 bypass/default references when no automation is pending;
- real JieLi target and emulator gate;
- measure added static RAM and callback budget.

### M5a4 — structural sounding-source parity
- quantized Gravity: early state visibility + target-time persistent audio;
- immediate Gravity;
- Chain discrete target-time audio;
- Chain continuous morph start/time law;
- Mother pending Gravity + mixer/mute target timing;
- field-specific Mother structural timing matrix;
- scripted Spiral/Bloom edits around bind boundaries proving the already-frozen M3c result is the sounding result.

## 11. Exit gate

M5a/Phase 5 does not pass until tests prove all of the following:
- no new scheduler/random/late trace changes except explicitly source-required structural state effects;
- `ApsisRuntime` remains 21,928 B;
- quantized Gravity is structurally visible at target−18 ms but persistent String/Drift do not adopt the new target before the true Gravity target;
- Chain discrete state is visible at target−24 ms while persistent target application/morph begins at the true target;
- Mother `->1` pending Gravity and mix/mute obey target-time audible scheduling and generic held parameters have explicit field disposition;
- Spiral/Bloom audible voices use the existing M3c late-bound results under last-moment structural edits;
- old dry-engine golden hashes remain unchanged on paths without structural automation;
- real target + emulator are green;
- remaining static RAM is measured and still leaves credible Phase-6 headroom.

Physical FM-1 remains unavailable; no real ISR timing or listening claim may be made.
