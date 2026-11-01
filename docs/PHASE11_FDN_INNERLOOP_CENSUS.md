# Phase 11M — FDN inner-loop branch census

Status: measurement only. No production DSP/state/target changes are authorized by this phase.

## Why this is next

P11H measured **131,072 FDN line reads**, **131,072 line writes** and **655,360 FDN Q15 multiplies** over the 16,384-room-frame census. P11J–P11L removed repeated block-rate feedback-coefficient work, so the next measurement returns to the per-line/per-frame path.

Inspection of `space_line_read()` shows several checks executed on every line read:
- defensive null/storage-bound guard;
- minimum-delay clamp;
- startup fill/readiness calculation;
- negative ring wrap;
- upper ring wrap;
- integer-vs-fractional interpolation branch;
- `i1` ring-end wrap.

P11M measures their actual frequency and separately documents topology invariants that may prove some branches impossible rather than merely unobserved.

## Measurement scenarios

1. **cold 16,384-frame census** — exact P11H room-frame count starting from reset;
2. **warm 65,536-frame census** — prefill all FDN lines first, reset counters only, then measure steady full-line behavior over a longer deterministic run.

Record:
- sine/LFO calls and phase-fraction distribution;
- line-read calls;
- modulation Q8 min/max and exact multiples of 256;
- minimum-delay-clamp opportunities;
- not-ready startup returns;
- full-line reads (`fill_count == capacity`);
- negative-wrap and upper-wrap conditions;
- integer reads (`frac == 0`) vs interpolated reads;
- `i1` ring-end wraps.

## Structural invariants to verify

With frozen topology:
- minimum base delay is 1,871 samples;
- modulation depth is bounded to approximately ±9 samples (`SPACE_MOD_DEPTH_Q8 = 2304`);
- each line capacity is `base_delay + 16` samples.

Therefore the minimum-delay clamp appears unreachable under the production caller, and a single negative-wrap correction appears sufficient: the raw read position cannot reach `>= span` after either the positive or negative path. P11M must keep measured evidence separate from this algebraic claim.

Once a line is fully filled, `needed <= base_delay + 9 < base_delay + 16 == capacity`, so the not-ready condition is impossible for that line. The current implementation still computes `needed` on every read; P11M measures how much of representative operation is already in this full-line state.

## Acceptance boundary

P11M may select a later proof candidate only if the census/invariants identify meaningful repeated work that can be removed without changing interpolation, modulation, delay lengths or startup semantics.

Any later candidate requires bit-identical equivalence against the current P11L production authority before production modification.

No physical FM-1 CPU/ISR claim is made by P11M.
