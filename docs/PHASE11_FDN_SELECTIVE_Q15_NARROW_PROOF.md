# Phase 11T — selective FDN Q15 narrow-helper proof

Status: complete proof and ephemeral target projection. Candidate is **rejected for production**. Production remains exact P11R.

## Candidate

P11S structurally proved that 24 of the 40 dynamic FDN `space_mul_q15()` calls per room frame can fit the existing `product ± 16384`, then arithmetic `>> 15`, operation inside signed 32-bit:

- alpha: 10/frame
- output scaling: 2/frame
- stereo cross/self mix: 4/frame
- excitation: 8/frame

The remaining 16 calls remain structurally wide and were never eligible for the candidate:

- Hadamard normalization: 8/frame
- feedback gain: 8/frame

P11T therefore tested one narrow candidate helper only at the 24 structurally safe sites while leaving the original signed-64-bit helper untouched at the two vetoed families.

## Exact arithmetic proof

Boundary coverage:

- alpha: **35** boundary cases
- output: **7** boundary cases
- cross: **14** boundary cases
- excitation: **14** boundary cases

Randomized helper comparison:

- **1,000,000** deterministic cases
- every candidate result bit-identical to frozen P11R `space_mul_q15()`

The proof also reasserts that the P11S Hadamard-normalization and feedback structural bounds exceed signed-32 product-plus-rounding range. They remain on the wide authority.

## Complete-FDN proof

Frozen P11R production render and the selective candidate were compared after every block, including complete stereo output, all FDN state, feedback-cache state and complete delay storage.

Coverage:

- cold startup: **256 blocks / 16,384 frames**
- fully warm continuation: **1,024 blocks / 65,536 frames**
- legal damping/decay sweep: **423 blocks / 13,536 frames**
- deterministic randomized/reset trajectory: **4,096 blocks / 212,458 frames**
- adversarial alternating full-scale input / parameter extremes: **512 blocks / 32,768 frames**
- total: **6,311 blocks / 340,682 frames**

Result: bit-identical stereo output, state and storage throughout.

Optimized and ASAN/UBSAN proof passes. The ephemeral candidate also passes the complete historical host + sanitizer suite.

## Exact P11R target baseline

Before applying the candidate, P11T regenerated the accepted P11R target exactly:

- image **140,472 B**
- ordinary RAM **87,524 B**
- POOL **319,176 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- `apsis_space_fdn_render` **1,098 B / 366** static instruction lines
- wide `space_mul_q15` helper **88 B / 27** static instruction lines

## Ephemeral JieLi target projection

The exact candidate was then applied only in the CI working tree and rebuilt using the same pinned JieLi target chain.

Candidate target:

- image **140,552 B** — **+80 B**
- ordinary RAM **87,524 B** — **0 B**
- POOL **319,176 B** — **0 B**
- package bytes **610,110 B**
- package SHA-256 `e466515629a7be86f49de2db5bc9eb81d887025c32fafc235b2834e4a57df15c`
- `apsis_space_fdn_render` **1,166 B / 410** static instruction lines

Code-shape delta versus P11R:

- render bytes: **+68 B**
- render static instruction lines: **+44**
- total image: **+80 B**
- RAM: **0 B**
- POOL: **0 B**

The narrow helper has no separate target symbol; the compiler fully inlines it. The original wide helper remains **88 B / 27** static instruction lines because the 16 structurally wide calls still require it.

## Decision — reject production validation

P11T proves that the selective narrow arithmetic is exact, but it does **not** justify a production change.

The target projection is strictly worse in every available static code-size/code-shape proxy that changes:

- +80 B image
- +68 B in the FDN render symbol
- +44 static instruction lines in the render hot path
- no RAM saving
- no POOL saving

Physical FM-1 cycle timing and ISR margin are unavailable. Therefore there is no evidence that the larger inlined candidate is faster, and static target evidence moves in the wrong direction. Under the Phase-11 evidence-first rule, speculative execution-speed benefit is insufficient to override the measured target regression.

**No P11U production validation is authorized for this candidate.** P11R remains the accepted production baseline.

## Authority

Authoritative P11T run:

- run `41383730919`
- technical head `a1907d5389de36ba094d86e2a74a6af729327651`
- evidence artifact ID `13114376798`
- evidence ZIP SHA-256 `cfbf81487f4d5c7ffd6c6093baed7ae8a0f04a779bb8a43d82e73ff786169ed4`

The final proof gate verifies that committed production `apsis_space_dsp.c/.h` and Cathedral room source remain exact P11R. The candidate exists only as an ephemeral proof/projection patch.

## Next direction

Return to measurement. Do not continue narrowing Q15 arithmetic. A later measurement phase should re-rank the residual FDN workload and target code shape after P11R, without preselecting another production rewrite.
