# P5a — sounding late-binding integration contract

Status: normative Phase-5 slice contract; **P5a completed and Phase 5 subsequently closed through P5f verification.**

Authority:
- Study 126 remains behavioural/state/timing/safety authority.
- M3c remains the only musical scheduler/runtime timeline.
- M4f/M4g dry-engine acoustic contracts remain frozen.

## Purpose

Phase 4 proved the five dry source engines in isolation and proved the symbolic/shared-runtime late-binding handoffs. P5a proves the stronger end-to-end property: source-defined structural state changes must reach the **actual sounding Spiral/Bloom DSP voices** at the same observation points, without moving musical targets or thawing occurrence-frozen acoustic data.

P5a is primarily a structural verification slice. If the current M3c + M4f/M4g architecture already satisfies the contract, production code should not be changed merely to manufacture a Phase-5 diff.

## Spiral contract

Study 126 / frozen M3c semantics:
1. Spiral occurrence identity, energy and duration are planned on the shared lookahead timeline.
2. M4f occurrence-time acoustic state is frozen: energy-derived amplitude, note duration/noteLength, Spiral source level, Drift delta/width, topology voice, attack/tail/Q and main/shadow geometry.
3. The musical target sample never moves.
4. At native target−80 ms, M3c resolves the frozen route ordinal against the **currently committed** Spiral/Gravity/register/tuning state.
5. Spiral deliberately does **not** project ordinary pending Gravity before its actual commit.
6. M4f combines that M3c pitch result with the frozen acoustic snapshot and the finite renderer instantiates the oscillator bank for the original target sample.
7. A state change after the target−80 ms bind does not retroactively retune that already-bound voice.

P5a acceptance:
- changing Gravity immediately after occurrence capture but before Spiral bind changes the final `ApsisSpiralLateBinding` and the actual finite-DSP oscillator phase increment;
- the occurrence token, target sample and frozen acoustic plan remain unchanged;
- the finite voice remains delayed to the original target sample.

## Bloom contract

Study 126 / frozen M3c semantics:
1. Phase-3 plans the Bloom target.
2. Existing occurrence at nominal target−45 ms freezes voicing plan plus M4g acoustic geometry: petal count/order, duration, unfurl/target offsets, width/pan and source-level/amplitude.
3. Each existing M3c petal bind occurs at `max(occurrence, petalTarget−50 ms)`.
4. Each bind resolves target-aware Gravity/tuning and late-reads current Bloom voice/attack/brightness/source ON/OFF.
5. Ordinary pending Gravity may be projected by Bloom when its pending target is visible to the petal target under the frozen +0.5 ms rule; Mother-held future Gravity is not projected.
6. A later DEPTH edit cannot delete petals from the occurrence-frozen plan.
7. M4g passes the M3c pitch result into the finite Bloom renderer without recomputing harmony or target time.

P5a acceptance:
- after a Bloom occurrence has frozen its acoustic plan, an immediate Gravity change before a later petal bind changes that petal's final M3c pitch and the actual finite-DSP oscillator phase increment;
- the occurrence token, petal target sample and occurrence-frozen petal acoustic values remain unchanged;
- the finite petal remains delayed to the original target sample.

## Original P5a non-goals

P5a itself did not close all of Phase 5. At contract freeze, these were deliberately deferred:
- quantized Gravity sounding-source cases, including the deliberate Spiral-vs-Bloom pending asymmetry;
- Mother `->1` held structural edits with live mixer/master;
- Chain discrete/morph application into already-sounding engines;
- pending-Gravity UI/visual distinction;
- shared Space/Cathedral (Phase 6 only).

The first three items were subsequently closed by P5b–P5e. Step-one sounding free-run/resync was additionally closed by P5f. Pending-Gravity UI/visual work remains a later UI concern; shared Space/Cathedral remains Phase 6.

## Resource and architecture constraints

P5a must add:
- no scheduler;
- no event queue;
- no random draw;
- no new persistent voice pool;
- no `ApsisRuntime` growth;
- no large unconditional BSS allocation.

The post-M4g real target leaves only 10,968 B static RAM headroom, so a verification-only P5a is intentionally preferred when production semantics are already correct.

## Exit gate

A dedicated normal + ASAN/UBSAN scripted test must prove both Spiral and Bloom end-to-end chains from structural state mutation through the existing M3c bind to actual finite-DSP oscillator frequency, while all frozen M3/M4 regressions remain green.

## Completion evidence

Verification head before the final documentation freeze:
- `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`
- APSIS core #357 PASS
- APSIS staged validation #346 PASS

Completed sounding gates:
- P5a structural late binding reaches sounding Spiral/Bloom DSP
- P5b quantized Gravity asymmetry reaches sounding Spiral/Bloom DSP
- P5c Mother-held Gravity reaches sounding sources only after `->1` commit
- P5d Chain discrete boundary reaches sounding Spiral/Bloom in source order
- P5e Chain continuous Drift morph begins on target and reaches sounding DSP
- P5f step-one RESYNC OFF free-runs into sounding Spiral energy

All P5 gates pass normal + ASAN/UBSAN. No production scheduler/DSP/runtime change was required.

Staged #346 therefore remains exact M4g:
- image 100,944 B
- RAM 87,336 B
- pool 30,720 B
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

This is the desired Phase-5 outcome: sounding structural parity was proven without consuming additional target RAM or creating a second timing/audio architecture.
