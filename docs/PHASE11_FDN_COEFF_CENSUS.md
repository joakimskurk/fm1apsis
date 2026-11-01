# Phase 11J — FDN coefficient recomputation census

Status: measurement only. No production behavior, state, capacity or target generator changes are authorized by this document.

## Why this is next

P11H measured 256 FDN render calls over the 65,536-output-frame census and 655,360 FDN Q15 multiplies. Inspection after P11I shows another narrower block-rate cost: `apsis_space_fdn_render()` recomputes all eight `space_feedback_gain()` values on every render call, even when the rounded `decay_ms` argument is unchanged.

Each feedback-gain evaluation contains one 32-bit division and three 64-bit fixed-point multiplies before the normal per-frame FDN work begins.

## P11J question

Measure how often the FDN render parameters actually change across representative legal room trajectories, and therefore how many coefficient evaluations are provably redundant at block rate.

Required scenarios:
- steady default/constant room state;
- one legal minimum→maximum decay transition under the existing room smoothing law;
- repeated legal extreme changes as an adversarial churn case.

Record:
- FDN render calls;
- `space_feedback_gain()` calls;
- distinct/change events for rounded `decay_ms` and `damping_hz` at FDN entry;
- exact derived division/64-bit-multiply counts;
- theoretical coefficient calls if values were recomputed only when rounded decay changes.

## Acceptance boundary

P11J may select a later cache candidate only if the census demonstrates meaningful redundant recomputation. It must not modify `ApsisSpaceFdn`, source behavior, target code or resource budgets.

Any later cache implementation requires a separate equivalence proof and complete real-target/idle/active regression gate. No physical FM-1 CPU or ISR claim may be made without hardware measurement.
