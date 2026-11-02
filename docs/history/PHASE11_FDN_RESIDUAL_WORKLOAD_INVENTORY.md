# Phase 11U — post-P11R residual FDN workload/code-shape inventory

Status: complete measurement phase. Production remains exact P11R.

## Purpose

P11T closed the selective narrow-Q15 path after proving exactness but measuring a worse JieLi target shape. P11U therefore returns to measurement without preselecting another arithmetic rewrite and re-ranks the residual work in the accepted P11R FDN.

No production source is modified by P11U.

## Fixed residual work per room frame

The unchanged P11R render structurally performs:

- **8** FDN line reads
- **8** FDN line writes
- **8** sine interpolations
- **8** `sine_b - sine_a` subtractions
- **16** fixed-sign output projections (`SPACE_OUT_LEFT/RIGHT[k] * mixed[k]`)
- **24** Hadamard add/sub operations
- **8** clamp calls
- **40** Q15 multiplies total
  - alpha: 10
  - output scale: 2
  - cross/self mix: 4
  - Hadamard normalization: 8
  - excitation: 8
  - feedback gain: 8

P11S/P11T already classified the 40 Q15 calls: 24 have signed-32 structural bounds but the selective narrow implementation produced a worse target shape, while the remaining 16 are structurally wide. P11U does not reopen that candidate.

## Residual sine-endpoint delta work

P11R stores two signed-16-bit sine endpoints per FDN line:

```c
lfo_sine_a_q15
lfo_sine_b_q15
```

The endpoint pair changes only when the LFO phase top byte changes, but every sine interpolation still evaluates:

```c
sine_b - sine_a
```

### Natural trajectory

Cold 16,384 frames:

- sine calls: **131,072**
- endpoint transitions: **88**
- endpoint reuses: **130,984**
- current delta subtractions: **131,072**

Continuing warm 65,536 frames:

- sine calls: **524,288**
- endpoint transitions: **367**
- endpoint reuses: **523,921**
- current delta subtractions: **524,288**

Combined:

- sine calls/current delta subtractions: **655,360**
- dynamic endpoint transitions: **455**
- delta evaluations if the delta is cached, including the eight reset initializations: **463**
- render-time delta subtractions removable under that model: **654,905**

This is the same slow endpoint-change property that justified P11R, but it identifies a residual operation left inside the interpolation expression after endpoint caching.

## Exhaustive delta domain

All 256 adjacent top-byte endpoint pairs were evaluated using the frozen production `space_sine_point()` authority:

- minimum delta: **−804**
- maximum delta: **+804**
- maximum absolute delta: **804**
- positive deltas: **128**
- negative deltas: **128**
- zero deltas: **0**
- complete delta domain fits signed 16-bit: **yes**

Therefore an `A + delta` representation can use the same two signed-16-bit fields per FDN line as P11R's `A + B` representation. P11U does **not** claim equivalence yet; it only proves representability and repeated-work rationale.

## Exact P11R JieLi target shape

P11U regenerates P11R exactly:

- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- `apsis_space_fdn_render` **1,098 B / 366** static instruction lines

Relevant target helper shape:

- `space_mul_q15`: **88 B / 27** static instruction lines
- `space_sine_point`: **48 B / 14** static instruction lines
- `space_line_read`: inlined / no separate symbol
- `space_hadamard8`: inlined / no separate symbol
- `space_clamp_q15`: inlined / no separate symbol
- `space_damping_alpha`: inlined / no separate symbol
- `space_feedback_gain`: inlined / no separate symbol

Inside the compiled FDN render, the disassembly contains **14 call instructions**:

- **12** static references to `space_mul_q15`
- **2** static references to `space_sine_point`

The sine delta subtraction itself is part of the inlined render arithmetic; an A+delta proof therefore tests whether removing that repeated subtraction improves or at least does not regress the actual render code shape without adding state.

Static instruction/code-size data are compiler proxies only, not physical CPU-cycle or ISR-margin measurements.

## Authority

Authoritative run:

- run `41384451346`
- technical head `28386fe6e26d6aadb2102469bceac662f310e599`
- artifact ID `13115075736`
- evidence ZIP SHA-256 `02b8f2d74fd932721e89524f1b2bad174a452ae7ccc189dd3193a853f69a88c3`

Optimized and ASAN/UBSAN measurement gates pass. The committed production Space hashes remain exact P11R throughout.

## Decision

P11U authorizes **P11V proof only** for one narrow representation change:

- replace each line's cached `A + B` sine endpoints with cached `A + delta`, where `delta = B - A`;
- retain exactly two signed-16-bit fields per line, so no state-size increase is expected;
- compute delta at reset and only when the phase top byte changes;
- preserve the exact interpolation expression result for all 65,536 top-16 phase combinations and every 8-bit index transition;
- preserve complete FDN output/state/storage across startup, warm continuation, legal parameter sweeps, randomized/reset and adversarial trajectories;
- run optimized + ASAN/UBSAN and the complete historical suite;
- regenerate exact P11R before applying an ephemeral candidate and measure real target image/RAM/POOL/package plus render code shape.

No P11V production change is authorized. If target shape is neutral/worse or equivalence fails, reject the candidate as with P11T.
