# P5f — Clock step-1 / Spiral free-run sounding contract

Status: normative Phase-5 slice contract.

Authority:
- Study 126 Clock/Spiral interaction.
- frozen M3c planning-time step-1 resync semantics.
- M4f Spiral acoustic snapshot + finite renderer.

## Purpose

Phase 5 explicitly requires `STEP 1 RESYNC` default OFF. P5f proves that this is not only a planner counter property: the free-running attractor phase changes the actual sounding Spiral acoustic energy at a bar boundary, while target/pitch identity remains unchanged.

## Source rule

- default `clock.step_one_resync = false`;
- when OFF, Spiral attractor position free-runs across Clock step 1;
- when ON, planning a Clock step 1 resets the Spiral attractor position to 0 before that pulse's attractor tick;
- route-step/pitch sequencing is not reset by this switch.

## Deterministic case

Use:
- Clock n=3, 180 BPM, 1/16 source division, no swing;
- Spiral rate 1x, attractor length 8;
- every attractor gate ON;
- one Spiral route point, so pitch is identical in both cases;
- distinct energy at attractor positions 0 and 3.

At startup, the first cycle consumes positions 0,1,2. At the next Clock-1 target **11,025**:
- RESYNC OFF must consume position 3;
- RESYNC ON must reset and consume position 0.

The existing M4f acoustic snapshot converts that frozen energy into amplitude; therefore the finite DSP leg `peak_gain_q15` must differ, while:
- target sample remains 11,025;
- bind sample remains 7,497 (target−80 ms);
- resolved pitch/oscillator phase increment remains equal because route identity is unchanged.

## Exit gate

Normal + ASAN/UBSAN integration must prove:
1. default state has resync OFF;
2. OFF boundary snapshot has trigger position 3;
3. ON boundary snapshot has trigger position 0;
4. target/bind samples and actual oscillator phase increment are equal;
5. frozen acoustic energy and finite DSP peak gain differ;
6. no production scheduler/runtime/DSP state is added.
