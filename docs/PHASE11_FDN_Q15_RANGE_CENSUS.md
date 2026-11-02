# Phase 11S — FDN Q15 multiply operand-range census

Status: complete measurement phase. Production `apsis_space_dsp.c/.h` and Cathedral room source remain exact P11R.

## Question

P11H measured **40 `space_mul_q15()` evaluations per FDN room frame**. The frozen helper always forms a signed 64-bit product before applying the exact asymmetric rounding term and arithmetic right shift:

```c
int64_t p = (int64_t)a * b;
if (p >= 0) p += 16384;
else p -= 16384;
return (int32_t)(p >> 15);
```

P11S asks which dynamic call-site families can *structurally* guarantee that `a * b`, including the existing ±16,384 rounding term, fits signed 32-bit. It does not change arithmetic or assume that a narrower multiply is faster on physical FM-1 hardware.

## Method

The committed P11R production source is hashed before and after the census. `ci/p11s_make_instrumented_space.py` creates a test-only copy in `/tmp`; the observer records operands and the mathematical 64-bit product, while the original `space_mul_q15()` remains the authority that computes every result.

The 40 dynamic calls per room frame divide into six families:

| Family | Calls / frame | Role |
| --- | ---: | --- |
| alpha | 10 | eight line damping updates + two post smoothers |
| output | 2 | left/right FDN output scaling |
| cross | 4 | stereo cross/self mix |
| Hadamard normalization | 8 | feedback-vector normalization |
| excitation | 8 | alternating-sign room input injection |
| feedback | 8 | normalized feedback × cached line gain |

Coverage includes a 16,384-frame cold trajectory, continuing 65,536-frame warm trajectory, all 47 legal damping-table points combined with 4/8/12-s decay anchors, 4,096 deterministic randomized blocks with resets, optimized and ASAN/UBSAN runs, plus explicit structural bounds.

## Observed natural ranges

### Cold — 16,384 frames / 655,360 multiplies

| Family | Operand A range | Operand B range | Maximum absolute product |
| --- | ---: | ---: | ---: |
| alpha | −25,490 .. 25,372 | 11,167 | 284,646,830 |
| output | −57,469 .. 52,760 | 3,277 | 188,325,913 |
| cross | −4,510 .. 4,680 | 3,769 .. 28,998 | 135,710,640 |
| Hadamard normalization | −64,229 .. 62,245 | 11,585 | 744,092,965 |
| excitation | −32,754 .. 32,767 | −12,976 .. 12,976 | 425,184,592 |
| feedback | −22,709 .. 22,006 | 31,651 .. 32,056 | 726,846,963 |

### Warm — 65,536 frames / 2,621,440 multiplies

| Family | Operand A range | Operand B range | Maximum absolute product |
| --- | ---: | ---: | ---: |
| alpha | −29,482 .. 30,433 | 11,167 | 339,845,311 |
| output | −82,290 .. 91,610 | 3,277 | 300,205,970 |
| cross | −7,287 .. 7,957 | 3,769 .. 28,998 | 230,737,086 |
| Hadamard normalization | −93,955 .. 98,138 | 11,585 | 1,136,928,730 |
| excitation | −32,767 .. 32,766 | −12,976 .. 12,976 | 425,184,592 |
| feedback | −33,218 .. 34,696 | 31,651 .. 32,056 | 1,110,514,872 |

The measured trajectories remain inside signed 32-bit product range even for Hadamard normalization and feedback. That observation alone is **not** sufficient to authorize narrowing.

## Legal sweep and randomized trajectories

The legal sweep covers **13,536 frames / 541,440 multiplies**. Its maximum absolute products are:

- alpha: **409,063,676**
- output: **214,614,007**
- cross: **175,408,902**
- Hadamard normalization: **827,331,190**
- excitation: **425,171,616**
- feedback: **814,475,232**

The deterministic randomized/reset trajectory covers **4,096 blocks / 197,319 frames / 7,892,760 multiplies**. Maximum absolute products are:

- alpha: **377,925,534**
- output: **157,450,019**
- cross: **119,413,764**
- Hadamard normalization: **617,573,180**
- excitation: **425,184,592**
- feedback: **602,727,060**

These measurements are workload evidence, not the width proof.

## Structural signed-32-bit classification

The decisive test includes the existing rounding term. A family is classified signed-32-safe only if its absolute product cannot exceed `INT32_MAX - 16384` under frozen legal structure.

| Family | Conservative `abs(a)` | Conservative `abs(b)` | Product bound | Signed-32 safe? |
| --- | ---: | ---: | ---: | :---: |
| alpha | 65,534 | 15,311 | 1,003,391,074 | yes |
| output | 262,136 | 3,277 | 859,019,672 | yes |
| cross | 65,534 | 28,998 | 1,900,354,932 | yes |
| excitation | 32,767 | 12,976 | 425,184,592 | yes |
| Hadamard normalization | 262,136 | 11,585 | 3,036,845,560 | **no** |
| feedback | 92,678 | 32,291 | 2,992,665,298 | **no** |

Therefore exactly **24/40 dynamic multiplies per room frame** belong to structurally safe families:

- alpha: 10
- output: 2
- cross: 4
- excitation: 8

The remaining **16/40** must retain the wide authority in any next candidate:

- Hadamard normalization: 8
- feedback gain: 8

This is deliberately stricter than the observed high-water result. Both rejected families have legal structural bounds above signed 32-bit even though none of the measured trajectories happened to reach those bounds.

## Frozen real-target authority

The measurement branch regenerates exact P11R:

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- inlined `apsis_space_fdn_render` **1,098 B / 366** static instruction lines

Authoritative census run: `41383031271` on technical head `6ddfa625b371d9a93db3d0a5a635b48b7eb08046`.

Evidence artifact:

- ID `13114240999`
- ZIP SHA-256 `2365e3fbcc4578479d438f42085562e77d16417dd9a8443bc419ba194c96bddc`

Optimized and ASAN/UBSAN census gates pass. Committed production Space hashes remain exact P11R throughout.

## Decision

P11S authorizes **P11T proof only** for a selective exact 32-bit Q15 helper at the four structurally safe families: alpha, output, cross and excitation. The existing wide helper remains mandatory for Hadamard normalization and feedback.

P11T must:

- preserve the exact `product ± 16384`, then `>> 15`, semantics;
- prove the narrow helper against the wide authority at structural boundaries and deterministic randomized values for every authorized family;
- prove complete-FDN stereo output, state and delay storage bit-identical across cold, warm, legal sweep, randomized/reset and adversarial boundary trajectories;
- prove the narrow helper cannot be reached from Hadamard-normalization or feedback sites;
- run optimized + ASAN/UBSAN and full historical behavior;
- regenerate exact P11R target before applying an **ephemeral** candidate;
- measure target image/RAM/POOL/package and inlined render code-shape before any production authorization.

No production arithmetic change is authorized by P11S. No result here measures physical FM-1 cycles, ISR margin or audible quality.
