#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11I are complete; any further optimization requires a new measured/equivalence-backed candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11J are complete; P11K is an equivalence-proof phase for the measured FDN feedback-gain cache candidate.**'
if old_status not in s:
    raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
marker = '### Later Phase-11 work\n'
p11j = '''### P11J — FDN coefficient recomputation census

Status: **[x] complete, frozen measurement phase. Production Space sources remain exact P11I.**

- [x] 256 FDN blocks × 8 feedback gains = **2,048** feedback-gain evaluations per scenario
- [x] current cost per scenario includes **2,048 32-bit divisions + 6,144 wide fixed-point multiplies** in coefficient setup
- [x] steady room: decay changes **0**; theoretical recompute-on-change calls **8**; **2,040/2,048 (99.6%) redundant**
- [x] one legal 4s→12s smoothed sweep: decay changes **192**; cached calls **1,544**; **504/2,048 (24.6%) redundant**
- [x] adversarial legal extreme churn: decay changes **255**; cached calls **2,048**; **0** redundant
- [x] optimized + ASAN/UBSAN census PASS
- [x] P11I production Space blobs unchanged
- [x] authoritative run `41350487323` on technical head `17030232e5f21f95a81b834e831d9021683d81bf`
- [x] historical census workflow manual-only and pinned to that technical head

Decision: authorize P11K **proof only** for a cache of the eight feedback gains keyed solely by the rounded `decay_ms`. Worst-case churn has zero computation savings and must remain exact.

### P11K — feedback-gain cache equivalence proof

Status: **[ ] next. No production patch until equivalence is proved.**

- [ ] compare always-recomputed and cached feedback-gain paths across reset/partial-fill/steady/sweeps/adversarial decay sequences
- [ ] preserve `space_feedback_gain()` arithmetic exactly; cache only its eight outputs + rounded decay key/validity
- [ ] normal + ASAN/UBSAN bit-identical output/state proof
- [ ] measure projected host/target state-size cost before production authorization
- [ ] only after proof, decide whether a separate production-validation stage is justified

'''
if marker not in s:
    raise SystemExit('ROADMAP later-work anchor missing')
s = s.replace(marker, p11j + marker, 1)
old_next = '''## Exact next action

Promote the frozen P11I state to `p11-optimization`, then return to measurement before selecting P11J. Do not claim physical CPU/ISR improvement from the P11I code-shape proxy; the next production optimization requires a new concrete hotspot and a falsifiable equivalence/regression gate.'''
new_next = '''## Exact next action

Promote frozen P11J measurement state to `p11-optimization`, then create one P11K proof branch. Prove an eight-value feedback-gain cache keyed by rounded `decay_ms` bit-identical to the current always-recomputed FDN before any production source is modified.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11I are complete. P11I is exact-head validated and frozen; further production optimization requires a new measured/equivalence-backed candidate.**'
new_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11J are complete. P11J measured FDN feedback-gain recomputation redundancy; P11K is proof-only until cached and always-recomputed FDN behavior are bit-identical.**'
if old_milestone not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_milestone, new_milestone, 1)
marker = '\n## Do not regress\n'
p11j_state = '''
## P11J — FDN coefficient recomputation census complete

Authority:
- technical head `17030232e5f21f95a81b834e831d9021683d81bf`
- run `41350487323`
- optimized and ASAN/UBSAN census PASS
- production `apsis_space_dsp.c` blob `72210e122b68bff77c56805618b2703d363a25ef` unchanged
- production `apsis_space_room.c` P11I blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346` unchanged

Current FDN behavior recomputes eight feedback gains on every FDN render. Across 256 FDN blocks this is always 2,048 `space_feedback_gain()` evaluations = 2,048 32-bit divisions + 6,144 wide fixed-point multiplies before per-frame work.

Measured rounded-decay churn:
- steady: 0 changes / 1 unique value → recompute-on-change model needs 8 calls; 2,040 redundant current calls (99.6%)
- one legal 4s→12s smoothed transition: 192 changes / 193 unique values → 1,544 calls; 504 redundant current calls (24.6%)
- adversarial extreme alternation: 255 changes → 2,048 calls; zero savings

P11J therefore selects one narrow P11K candidate: cache exactly the eight feedback gains keyed by rounded `decay_ms`, invalidated on reset and recomputed only when that key changes. No other coefficient or DSP arithmetic is authorized to change.

## Exact next phase — P11K feedback-gain cache proof

Before production modification:
1. implement baseline and candidate paths in a proof harness or ephemeral source transformation;
2. cover reset, partial fill, steady state, legal smoothed transitions and adversarial decay churn;
3. require bit-identical stereo output and all pre-existing FDN musical/runtime state;
4. run optimized + ASAN/UBSAN;
5. measure exact projected state-size cost of the cache;
6. only if all proof gates pass, authorize a separate production-validation patch.

P11J/P11K remain workload/equivalence work. Physical FM-1 CPU cycles and ISR margin are still unverified.
'''
if marker not in s:
    raise SystemExit('PROJECT_STATE do-not-regress anchor missing')
s = s.replace(marker, p11j_state + marker, 1)
state.write_text(s)
print('PASS: P11J roadmap/project state updated')
