# P5b — quantized Gravity sounding contract

Status: normative Phase-5 slice contract.

Authority:
- Study 126 behavioural/timing/state semantics.
- frozen M3c Gravity/Spiral/Bloom late-binding rules.
- M4f/M4g finite dry engines.

## Purpose

P5a proves immediate structural edits reach actual sounding Spiral/Bloom voices. P5b proves the deliberately asymmetric **ordinary quantized Gravity** semantics reach those same finite DSP voices.

No new scheduler, queue, random source, voice pool or persistent DSP state is permitted for this slice.

## Shared pending state

For ordinary quantized Gravity:
- the candidate lives in `ApsisRuntime.gravity_pending` until its source-defined commit callback;
- committed `state.gravity` remains unchanged before that callback;
- native M3c commits ordinary Gravity at musical target−18 ms;
- the pending candidate itself does not consume random numbers or move musical targets.

Mother-held Gravity is a different Phase-5 case and is not covered by P5b.

## Spiral asymmetry

Spiral bind is target−80 ms and resolves against **currently committed state only**.

Therefore, when a quantized Gravity candidate targets the same Clock boundary as a Spiral note:
1. the pending candidate exists at Spiral bind;
2. `state.gravity` is still the old committed Gravity;
3. Spiral must resolve and sound the old Gravity pitch;
4. it must not project `gravity_pending`;
5. the note target remains fixed.

After the target−18 ms Gravity commit, the next Spiral bind must resolve/sound the new committed Gravity.

## Bloom asymmetry

Bloom is deliberately future-aware for ordinary pending Gravity.

At a petal bind, M3c evaluates target-aware Bloom harmony. If ordinary pending Gravity has `pending.target_sample <= petalTarget + 0.5 ms`, the pending degree/ring is projected even though shared `state.gravity` has not yet committed.

Therefore, when Bloom petal 0 targets the same Clock boundary as the pending Gravity candidate:
1. Bloom occurrence/petal-0 bind at target−45 ms still happens before the target−18 ms shared commit;
2. shared `state.gravity` is still old;
3. the Bloom petal binding must already resolve/sound the pending degree/ring;
4. occurrence-frozen petal geometry and target sample remain unchanged.

This asymmetry is source-defined and must not be simplified into one generic "latest state wins" rule.

## Deterministic P5b reference timeline

Use Clock:
- `n=3`
- 180 BPM
- 1/16 source division (`division_num=1`, `division_den=4`)
- no swing
- Gravity grid = 1 part / Clock-1 boundaries
- Spiral rate 1x with one active route
- Bloom cycles 1, rate 1, trigger step 0, delay 0

After the initial 1.20 s lookahead fill, request an ordinary quantized Gravity change from degree 0 to degree 4.

Expected aligned boundary:
- pending / Spiral / Bloom target: **55,125 samples**
- Spiral bind: **51,597** (`target−3,528`, 80 ms)
- Bloom occurrence / petal-0 bind: **53,140** (`target−1,985`, 45 ms causal clamp)
- Gravity commit: **54,331** (`target−794`, nearest 18 ms)
- next Spiral target: **58,800**; its bind is after the commit.

## Exit gate

Normal + ASAN/UBSAN scripted integration must prove with the actual finite DSP objects:
- before commit, pending-case Spiral phase increment equals the no-pending control;
- before commit, pending-case Bloom petal phase increment differs from the no-pending control and follows the projected pending candidate;
- shared state is still old at both pre-commit bindings;
- target/bind samples do not move;
- after commit, the next Spiral finite voice uses the new Gravity pitch;
- no `ApsisRuntime` growth or M3/M4 reference regression.
