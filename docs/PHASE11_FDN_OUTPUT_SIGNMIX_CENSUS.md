# Phase 11X — FDN output sign-mix census

Status: measurement only. Production remains exact P11W.

## Question

The FDN output projection currently accumulates two fixed eight-term dot products using `SPACE_OUT_LEFT[]` and `SPACE_OUT_RIGHT[]`. Every coefficient is exactly `+1` or `-1`, but the current source still expresses the work as a multiply inside the eight-line loop.

P11X measures whether this is a meaningful remaining workload/code-shape candidate before any proof or production change.

## Required evidence

1. Verify the frozen sign vectors contain only ±1 and record their sign distribution.
2. Prove the structural absolute bound of an eight-term signed sum of Q15-range FDN line values is safe in signed 32-bit.
3. Quantify the exact source-level sign-product count over the established cold 16,384-frame and warm 65,536-frame census windows.
4. Regenerate the exact P11W JieLi target and capture the actual `apsis_space_fdn_render` opcode histogram/raw disassembly.
5. Keep production Space source/header byte-identical to P11W.

## Acceptance boundary

P11X may only authorize a later proof candidate if the target evidence supports testing fixed add/sub accumulation as a plausible non-regressive replacement. The candidate would still require full arithmetic/FDN equivalence and ephemeral real-target projection before any production validation.

No physical FM-1 cycle or ISR claim is made by this census.
