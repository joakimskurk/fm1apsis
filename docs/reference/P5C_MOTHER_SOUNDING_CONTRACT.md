# P5c — Mother `->1` sounding structural contract

Status: normative Phase-5 slice contract.

Authority:
- Study 126 Mother `->1` behavioural semantics.
- frozen M3c Mother pre-target callback and Spiral/Bloom late-binding rules.
- M4f/M4g finite dry engines.

## Purpose

P5c proves that Mother-held structural edits stay outside both committed state and ordinary pending-Gravity projection until Mother actually commits them, while Mother/output domains remain live.

## Hold semantics

With Mother sync enabled:
- structural edits are held;
- Gravity is stored in `ApsisMotherPending.gravity_pending`, not `ApsisRuntime.gravity_pending`;
- Mother/output domains are not held by `apsis_mother_should_hold()`;
- the held structural edit must not alter already-planned target times or consume random numbers.

Mother-held Gravity is deliberately different from ordinary quantized Gravity:
- Spiral cannot see it before commit;
- Bloom cannot future-project it before commit because it is not represented as ordinary `framePending` / `ApsisRuntime.gravity_pending`.

## Commit timing

M3c models Study 126 Mother application at Clock target−26 ms.

For the deterministic aligned P5c timeline:
- Clock: n=3, 180 BPM, 1/16 source division, no swing;
- Mother sync ON, horizon 1;
- Gravity degree 4 queued immediately after the initial lookahead fill;
- `pending_start_abs = 14`;
- next eligible Clock-1 target = **55,125 samples**.

Expected observations:
- Spiral bind = **51,597** (target−80 ms): old degree 0;
- Bloom occurrence/petal-0 bind = **53,140** (target−45 ms): old degree 0;
- Mother commit = **53,978** (target−1,147; nearest 26 ms): degree 4 becomes committed;
- next Spiral target = **58,800**, bind **55,272**: new degree 4;
- next Bloom Clock-1 target = **66,150**: new degree 4.

## Sounding acceptance

Normal + ASAN/UBSAN scripted integration must prove with actual finite DSP voices:
1. the same-boundary Spiral finite voice is identical to a no-Mother control;
2. the same-boundary Bloom petal finite voice is identical to a no-Mother control;
3. shared `state.gravity` remains degree 0 and ordinary `runtime.gravity_pending` remains inactive at both pre-commit bindings;
4. Mother pending Gravity remains armed at those bindings;
5. after the target−26 ms Mother commit, the next Spiral finite voice and next Bloom Clock-1 petal use degree 4 and differ from the control;
6. Mother pending clears and Mother sync returns OFF after successful commit;
7. `APSIS_MOTHER_DOMAIN_MOTHER` and `APSIS_MOTHER_DOMAIN_OUTPUT` remain live while structural hold is armed.

## Non-goals

P5c does not yet prove generic pending-param dispatch, Chain application/morph, or UI rendering of pending state. Those remain separate Phase-5 work.

No production scheduler, queue, random source, `ApsisRuntime` layout or voice-pool change is expected for P5c if the existing architecture is correct.
