# Phase 11P — FDN LFO/sine workload census

Status: measurement-only. Production `apsis_space_dsp.c/.h` and Cathedral room source remain exact P11O.

## Question

P11M/P11N/P11O reduced repeated work in the FDN line reader. The next remaining per-line/per-frame helper in the same hot loop is the modulation sine path:

```c
line->lfo_phase += line->lfo_phase_inc;
mod_q8 = ((int32_t)space_sine_q15(line->lfo_phase) * SPACE_MOD_DEPTH_Q8) >> 15;
```

The frozen `space_sine_q15()` extracts an 8-bit table index and 8-bit fraction, evaluates two quarter-wave endpoints through `space_sine_point()`, then linearly interpolates them. P11P asks which part of that work actually changes at audio rate.

## Method

`tests/test_p11p_fdn_lfo_sine_census.c` includes the unchanged production DSP translation unit so the static sine helpers themselves are the authority. It performs:

1. a natural 16,384-frame cold trajectory from the exact FDN reset phases/increments;
2. a continuing 65,536-frame warm trajectory with endpoint-cache bookkeeping preserved across the boundary;
3. an exhaustive 65,536-case sweep of all top-16 phase combinations (`index × frac`), because the lower 16 phase bits are ignored by the frozen helper;
4. optimized and ASAN/UBSAN runs.

The cache bookkeeping is measurement only. It records how often the 8-bit endpoint index changes; it does not alter the sine result or production state.

## Natural trajectory census

Cold 16,384 frames / 131,072 sine calls:

- point evaluations: **262,144**
- fractional byte zero: **535**
- fractional byte non-zero: **130,537**
- quadrant-boundary endpoint crossings: **0**
- endpoint-pair refreshes with a cold cache model: **96**
- endpoint-pair reuses: **130,976**
- point evaluations that a correctly keyed cache could avoid: **261,952**
- distinct top-byte indices observed: **96 / 256**
- distinct fractional values observed: **256 / 256**
- per-line refreshes: **7, 8, 10, 11, 13, 14, 15, 18**

Warm continuing 65,536 frames / 524,288 sine calls:

- point evaluations: **1,048,576**
- fractional byte zero: **2,050**
- fractional byte non-zero: **522,238**
- quadrant-boundary endpoint crossings: **6,944**
- endpoint-pair refreshes: **367**
- endpoint-pair reuses: **523,921**
- point evaluations that a correctly keyed cache could avoid: **1,047,842**
- distinct top-byte indices observed: **255 / 256**
- distinct fractional values observed: **256 / 256**
- per-line refreshes: **27, 32, 37, 41, 48, 53, 60, 69**

Combined natural window:

- sine calls: **655,360**
- current point evaluations: **1,310,720**
- cache-model endpoint refreshes: **463**
- endpoint-pair reuses: **654,897**
- avoidable point evaluations under that model: **1,309,794**
- endpoint pair is unchanged for **99.929%** of calls

The first eight cold refreshes are cache initialization, one per line. A proof candidate may move those endpoint calculations into reset and then refresh only when the phase top byte actually changes; P11P does not yet authorize that implementation.

## Exhaustive helper-domain census

Across all 65,536 top-16 phase combinations:

- point evaluations: **131,072**
- `frac == 0`: **256**
- `frac != 0`: **65,280**
- endpoint crossing (`index & 63 == 63`): **1,024**
- all 256 indices occur
- all 256 fractional values occur
- all four quadrants are equally represented for both endpoints
- equal adjacent endpoint pairs: **0**

Consequences:

- interpolation is real and cannot be removed;
- quadrant/wrap semantics are real and cannot be removed;
- there is no zero-slope adjacent endpoint special case to exploit;
- the measured redundancy is specifically repeated evaluation of the same two endpoints while the fractional byte continues to move.

## Candidate decision

P11P supports one narrow next proof candidate: **per-line endpoint caching keyed by the LFO phase top-byte transition while preserving the existing interpolation expression exactly**.

A preferred proof shape is to initialize the two exact endpoints from each line's reset phase, retain only those two signed 16-bit endpoints per line if possible, and refresh them when the 8-bit phase index changes. This potentially needs only 4 B additional state per line (**32 B total before ABI/layout confirmation**) and avoids adding a persistent index if the refresh test can safely use the old/new `lfo_phase` top byte.

This is not production authorization. A later proof phase must establish:

- bit-identical sine output for the complete relevant phase domain;
- bit-identical complete-FDN output/state/storage across startup, warm continuation, resets and randomized trajectories;
- exact target ABI/state-size cost rather than assuming 32 B;
- target image/RAM/POOL and inlined render code-shape delta;
- optimized + ASAN/UBSAN behavior;
- no regression to reset/startup semantics.

If those gates fail or the target tradeoff is poor, the candidate is rejected.

## Physical-hardware boundary

No P11P result measures FM-1 CPU cycles, ISR margin, runtime stack high-water or audible quality. The census only identifies repeated work and a falsifiable exact-equivalence candidate.
