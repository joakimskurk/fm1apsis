# Phase 11M — FDN inner-loop branch census

Status: **complete measurement phase. Production Space sources remain exact P11L authority.**

## Why this phase

P11H measured **131,072 FDN line reads**, **131,072 line writes** and **655,360 FDN Q15 multiplies** over the 16,384-room-frame census. P11J–P11L removed repeated block-rate feedback-coefficient work, so P11M returned to the per-line/per-frame path.

Inspection of `space_line_read()` showed several checks executed on every line read:
- defensive null/storage-bound guard;
- minimum-delay clamp;
- startup fill/readiness calculation;
- negative ring wrap;
- upper ring wrap;
- integer-vs-fractional interpolation branch;
- `i1` ring-end wrap.

P11M measures their frequency and keeps measured evidence separate from topology invariants that can prove branches impossible rather than merely unobserved.

## Authority

- branch `p11m-fdn-innerloop-census-staging`
- technical head `04b807617ffec084e3721fa2a166ac6ad9e76d59`
- authoritative run `41355338593`
- optimized census PASS
- ASAN/UBSAN census PASS
- production `apsis_space_dsp.c` remains P11L blob `0c472f3844210f0dfb3d31bf6dacf007272ac078`
- production `apsis_space_dsp.h` remains P11L blob `71afae3c4c52bbd735121597dfa7ad6cea4f32f5`
- production `apsis_space_room.c` remains P11I/P11L blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`

Instrumentation is generated into `/tmp`; no P11M probe enters production firmware.

## Cold census — reset to 16,384 FDN frames

This matches the P11H FDN-room-frame count.

- sine/LFO calls: **131,072**
- line-read calls: **131,072**
- sine phase low-byte zero: **535**
- modulation range: **−2,304 .. +2,303 Q8**
- modulation exact multiple of 256: **1,670** calls
- derived delay range before clamp: **478,976 .. 755,150 Q8**
- minimum-delay-clamp opportunities: **0**
- startup not-ready returns: **19,047 / 131,072 = 14.53%**
- calls already at full line capacity: **111,902 / 131,072 = 85.37%**
- ready reads: **112,025**
- negative-wrap condition among ready reads: **111,261 / 112,025 = 99.32%**
- upper-wrap condition: **0**
- raw read-position range: **−755,065 .. +6,140 Q8**
- exact integer reads: **348 / 112,025 = 0.31%**
- interpolated reads: **111,677 / 112,025 = 99.69%**
- `i1` ring-end wraps: **44 / 112,025 = 0.039%**

The cold result shows that startup readiness semantics are real and cannot simply be removed: 19,047 reads correctly return zero before enough history exists. However, most of the same cold census is already in the permanently-full state.

## Warm census — 65,536 measured frames after full prefill

All eight FDN lines are filled to capacity before counters are reset.

- sine/LFO calls: **524,288**
- line-read calls: **524,288**
- sine phase low-byte zero: **2,058**
- modulation range: **−2,304 .. +2,303 Q8**
- modulation exact multiple of 256: **4,438** calls
- derived delay range before clamp: **479,071 .. 757,919 Q8**
- minimum-delay-clamp opportunities: **0**
- startup not-ready returns: **0**
- calls at full line capacity: **524,288 / 524,288 = 100%**
- ready reads: **524,288**
- negative-wrap condition: **521,026 / 524,288 = 99.38%**
- upper-wrap condition: **0**
- raw read-position range: **−757,875 .. +6,144 Q8**
- exact integer reads: **4,438 / 524,288 = 0.85%**
- interpolated reads: **519,850 / 524,288 = 99.15%**
- `i1` ring-end wraps: **224 / 524,288 = 0.043%**

This rejects interpolation removal as a useful candidate: almost every steady-state read is genuinely fractional. The `i1` wrap is rare but real, so it must also remain unless replaced by an exactly equivalent formulation.

## Structural invariants

Frozen topology gives:
- minimum base delay **1,871 samples**;
- production modulation bounded to **−2,304 .. +2,303 Q8**, i.e. approximately ±9 samples;
- line capacity exactly `base_delay + 16` samples;
- `write_pos` always in `0 .. capacity-1`.

### Minimum-delay clamp is unreachable from the production caller

The smallest possible delay is:

`1871*256 - 2304 = 476,672 Q8`, far above the clamp threshold `256 Q8`.

The measured minima are even higher because the shortest-delay line did not coincide with the exact most-negative modulation at the sampled instants. Therefore the existing `if (delay_q8 < 256)` branch is structurally unreachable under the frozen production caller.

### Upper ring-wrap branch is structurally unreachable

For any line, the largest raw positive read position occurs at maximum `write_pos` and minimum delay:

`(base+15)*256 - (base*256 - 2304) = 6,144 Q8`.

The smallest ring span is `(1871+16)*256 = 483,072 Q8`, so a non-negative raw read is always far below `span_q8`.

For a negative raw read, the most-negative value is greater than `-span_q8` because maximum delay adds at most 2,303 Q8 while the 16-sample reserve contributes 4,096 Q8. One `+ span_q8` correction therefore always lands in `[0, span_q8)`. The second `if (read_q8 >= span_q8)` correction is structurally unreachable.

### Full-line readiness can use a fast path without changing startup semantics

The maximum required history is:

`ceil(base_delay + 2303/256) = base_delay + 9`,

while capacity is `base_delay + 16`. Therefore once `fill_count == capacity`, readiness failure is impossible and remains impossible because fill count saturates at capacity.

Current code nevertheless computes `needed = (delay_q8 + 255) >> 8` and compares `fill_count < needed` on every read. A candidate may preserve the exact startup calculation only while a line is not full, while bypassing it for full lines.

## Decision

P11M **authorizes a P11N proof candidate**, not a production patch.

The candidate should be limited to `space_line_read()` production-caller invariants:
1. retain startup readiness exactly for partial lines, but fast-path full lines without calculating `needed`;
2. remove the structurally unreachable minimum-delay clamp;
3. remove the structurally unreachable upper ring-wrap correction;
4. optionally remove the defensive helper guard only if the proof separately establishes that this static helper has no call path outside the already-validated `apsis_space_fdn_render()` loop.

Do **not** change:
- LFO phase or sine interpolation;
- modulation depth;
- delay lengths or capacities;
- negative-wrap behavior;
- fractional interpolation;
- `i1` ring-end behavior;
- startup output semantics.

P11N must prove candidate and current P11L line-read authority bit-identical across boundary classes and deterministic randomized valid states before any production source is modified. Full FDN render equivalence should then confirm the helper-level proof at system level.

No physical FM-1 CPU/ISR improvement is claimed by P11M.
