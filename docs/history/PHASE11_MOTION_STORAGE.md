# Phase 11B — Motion storage proof

P11A established that the final Phase-10 target uses 328,740 / 344,064 B of POOL (95.546%) and that the entire 19,208-B increase from Phase 9 is exactly `ApsisStringMotionStore`.

P11B is a proof step only. It does not change firmware storage or musical behavior.

## Hypothesis

The current Motion point stores:
- phase (`uint16_t`)
- split (`uint16_t`)
- frozen absolute pitch in millisemintones (`int32_t`)

During recording, however, every point in a targeted lane uses the same frozen `ApsisStringMotionPitchContext`. The point pitch is produced only by:

`apsis_string_motion_pitch_for_split(frozen_lane_context, side, point.split_q15)`

Therefore a lane can potentially retain one frozen pitch context and store only phase+split per point. Free-Dual overdub requires independent contexts per lane: overdubbing the right lane must preserve the left lane's earlier context and shape.

## What P11B must prove before implementation

1. Every pitch stored by an actual capture is exactly recomputable from the lane's frozen context plus its stored split.
2. A later free-Dual overdub can use a materially different frozen context for the other lane while the untouched lane remains bit-for-bit unchanged.
3. Reducing `pitch_milli` to a signed 16-bit scalar is **not** lossless across the existing source-bounded String range; the optimisation must remove redundancy, not precision.
4. The candidate keeps the source-locked 1200-point-per-lane ceiling and 32-Hz capture bound unchanged.

## Candidate layout model

Without changing the 1200-point limit:
- current point: 8 B
- candidate point: 4 B (`phase_q16`, `split_q15`)
- frozen pitch context: 20 B once per lane
- current store: 19,208 B
- candidate store model: 9,644 B
- projected saving: 9,564 B

If target section accounting follows the C layout exactly, POOL would move from 328,740 B to about 319,176 B, increasing headroom from 15,324 B to about 24,888 B. Those are projections only until a real JieLi target build measures the implementation.

## Implementation gate

Do not implement the compact lane until the P11B host proof and ASAN/UBSAN workflow are green. After that, the implementation must preserve:
- exact capture point count/phase/split behavior;
- exact millisemintone pitch values at playback;
- free-Dual overdub ownership;
- quantized hold vs continuous interpolation behavior;
- frozen Gravity/Tuning context behavior;
- all Phase-10 Motion active lifecycle evidence;
- exact inactive Phase-10 PCM/runtime baseline.
