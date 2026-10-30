# P5e — Chain continuous morph sounding contract

Status: normative Phase-5 slice contract.

Authority:
- Study 126 Chain apply/morph timing.
- frozen M3c distinction between target−24 ms discrete cell application and target-time continuous morph start.
- M4d persistent Drift dry engine.

## Purpose

P5d gates discrete structural Chain application into sounding Spiral/Bloom. P5e gates the separate continuous lane rule: selecting/applying a new cell early must not make its morph audible early.

## Deterministic case

Use Clock n=3, 180 BPM, 1/16 source division, no swing.

Chain:
- scope DRIFT enabled;
- cell 0 = KEEP slot 0 with Drift amount 0.20;
- cell 0 length = 1 fine pulse;
- cell 1 = KEEP slot 1 with Drift amount 0.80;
- cell 1 morph = 1/4 cycle.

After startup applies cell 0, the next Chain pulse has musical target **3,675 samples**:
- discrete Chain callback = **2,617** (`target−1,058`, nearest 24 ms);
- cell 1 becomes the active Chain cell at 2,617;
- continuous Drift state remains exactly cell-0 amount through target 3,675;
- M3c `chain_morph_start_sample` is exactly 3,675;
- a quarter-cycle morph at n=3 spans about 2,756 samples and begins only at the target.

## Sounding acceptance

Normal + ASAN/UBSAN integration must prove:
1. at target−24 ms, Chain position has advanced but `state.drift.amount_q15` is still the old value;
2. `ApsisDriftDsp` target phase increments are unchanged at that pre-target callback;
3. at the musical target, the morph still has t=0 and Drift DSP target is unchanged;
4. midway through the morph, state Drift amount is strictly between old/new and the actual Drift DSP target phase increments have moved;
5. after rendering, the oscillator phase increment moves toward the new target rather than jumping before the target;
6. at morph end, state reaches the KEEP slot target and the morph deactivates;
7. no scheduler, queue, random source, runtime layout or DSP pool is added.
