# P5d — Chain sounding boundary contract

Status: normative Phase-5 slice contract.

Authority:
- Study 126 Chain boundary/apply semantics.
- frozen M3c Chain apply observation at target−24 ms.
- M4f/M4g sounding Spiral/Bloom engines.

## Purpose

P5d proves that Chain structural cell changes reach sounding engines on the same shared event/boundary model and do not leak backward into voices whose source-defined bind points precede the Chain apply callback.

## Deterministic boundary case

Use:
- Clock `n=3`, 180 BPM, 1/16 source division, no swing;
- Spiral rate 1x with one active route;
- Bloom cycles 1, rate 1, trigger step 0, delay 0;
- Chain scope CHORD enabled;
- cell 0: KEEP slot 0, degree 0, length 3 fine pulses;
- cell 1: KEEP slot 1, degree 4, length >= 3 fine pulses;
- Chain starts on Clock-1.

After the startup Clock-1 applies cell 0, the third following Chain pulse targets sample **11,025** and advances to cell 1 at the frozen M3c callback:
- Spiral bind: **7,497** (`target−3,528`, 80 ms)
- Bloom occurrence/petal-0 bind: **9,040** (`target−1,985`, 45 ms)
- Chain discrete apply: **9,967** (`target−1,058`, nearest 24 ms)

Therefore the voices targeting 11,025 must still sound the cell-0 Gravity/chord. The next Spiral target 14,700 and the next Bloom Clock-1 target 22,050 must sound cell 1.

## Sounding acceptance

Normal + ASAN/UBSAN integration must prove with finite DSP voices:
1. same-boundary Spiral pitch/phase equals a control whose second KEEP slot is still degree 0;
2. same-boundary Bloom petal pitch/phase equals that control;
3. shared Gravity is still degree 0 at both same-boundary bind points;
4. Chain has switched to cell 1 and shared Gravity is degree 4 before the next Spiral bind;
5. next Spiral finite-DSP phase differs from control;
6. next Bloom Clock-1 petal finite-DSP phase differs from control;
7. target and bind samples remain unchanged;
8. no new scheduler, queue, random source, runtime layout or voice pool is added.

## Continuous morph

This slice gates discrete structural Chain application. Existing M3c tests already freeze the separate source rule that continuous Chain morph begins on the musical target rather than the target−24 ms discrete callback. Phase 5 will add one sounding continuous-lane test separately so this distinction is not collapsed.
