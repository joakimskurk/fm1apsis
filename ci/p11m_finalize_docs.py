#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11L are complete; further production optimisation requires a new measured, equivalence-backed candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11M are complete; P11N is proof-only for the measured FDN line-read invariant simplification candidate.**'
if old_status not in s:
    raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)

marker = '### Later Phase-11 work\n'
p11m = '''### P11M — FDN inner-loop branch census

Status: **[x] complete, frozen measurement phase. Production Space sources remain exact P11L.**

- [x] cold census: **16,384 frames / 131,072 line reads**
- [x] warm census: **65,536 frames / 524,288 line reads** after all eight FDN lines reach full capacity
- [x] modulation range measured **−2,304 .. +2,303 Q8**
- [x] minimum-delay clamp opportunities: **0** cold and warm; topology proves the branch unreachable under the production caller
- [x] upper ring-wrap condition: **0** cold and warm; topology proves one negative-wrap correction is sufficient
- [x] cold startup not-ready returns: **19,047 / 131,072 (14.53%)** — startup semantics must remain
- [x] cold reads already at full capacity: **111,902 / 131,072 (85.37%)**
- [x] warm full-capacity reads: **524,288 / 524,288 (100%)**; readiness failure impossible once full
- [x] warm negative-wrap condition: **521,026 / 524,288 (99.38%)** — negative wrap must remain
- [x] warm integer reads: **4,438 / 524,288 (0.85%)**; interpolated reads **519,850 (99.15%)** — interpolation must remain
- [x] warm `i1` ring-end wraps: **224 (0.043%)** — rare but real
- [x] optimized + ASAN/UBSAN census PASS
- [x] production P11L Space blobs unchanged
- [x] authoritative run `41355338593` on technical head `04b807617ffec084e3721fa2a166ac6ad9e76d59`
- [x] census workflow manual-only and pinned to the proven technical head

Decision: authorize P11N **proof only** for a production-caller-specialized `space_line_read()` candidate: retain exact partial-line startup readiness, fast-path full lines without `needed`, remove only the structurally unreachable minimum-delay clamp and upper-wrap correction, and leave modulation/interpolation/negative-wrap/`i1` semantics untouched. Defensive helper-guard removal requires a separate call-path proof.

### P11N — FDN line-read invariant simplification proof

Status: **[ ] next. No production patch until equivalence is proved.**

- [ ] prove current P11L line-read authority vs candidate across all eight line topologies, boundary modulation values, readiness thresholds, ring boundaries and deterministic randomized valid states
- [ ] preserve exact partial-fill/startup zero-return behavior
- [ ] preserve negative wrap, fractional interpolation and `i1` wrap exactly
- [ ] full-FDN render equivalence across cold, warm, randomized phase/state and reset trajectories
- [ ] optimized + ASAN/UBSAN proof
- [ ] only after proof, measure an ephemeral real-target code-shape/resource projection before production authorization

'''
if marker not in s:
    raise SystemExit('ROADMAP later-work anchor missing')
s = s.replace(marker, p11m + marker, 1)

old_next = '''## Exact next action

Promote the frozen P11L production state to `p11-optimization`. Then return to measurement before selecting any P11M production candidate; no further DSP change is authorized until a concrete hotspot and falsifiable equivalence/regression gate are defined.'''
new_next = '''## Exact next action

Promote frozen P11M measurement state to `p11-optimization`, then create one P11N proof branch. Prove the production-caller-specialized FDN line-read candidate bit-identical to current P11L behavior before any production source is modified.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11L are complete. P11L is the current exact-head validated production baseline; further production optimisation requires a new measured/equivalence-backed candidate.**'
new_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11M are complete. P11L remains the production baseline; P11M measured FDN line-read branch behavior and P11N is proof-only until the invariant-simplified reader is bit-identical.**'
if old_milestone not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_milestone, new_milestone, 1)

old_next = '''## Exact next phase

Promote P11L to `p11-optimization`, then return to measurement. No P11M production change is authorized until a new concrete hotspot and falsifiable equivalence/regression gate are established. Physical ISR timing, stack high-water, LCD/audio contention and listening quality remain deferred to hardware validation.
'''
new_next = '''## P11M — FDN inner-loop branch census complete

Authority:
- technical head `04b807617ffec084e3721fa2a166ac6ad9e76d59`
- run `41355338593`
- optimized + ASAN/UBSAN PASS
- production DSP source/header and Cathedral room blobs remain exact P11L authority
- instrumentation exists only in generated `/tmp` source

Cold 16,384-frame census:
- 131,072 sine/LFO calls and line reads
- modulation **−2,304 .. +2,303 Q8**
- minimum-delay-clamp opportunities **0**
- not-ready startup returns **19,047 (14.53%)**
- full-capacity calls **111,902 (85.37%)**
- ready reads **112,025**
- negative-wrap **111,261 (99.32% of ready)**
- upper-wrap **0**
- integer reads **348 (0.31% of ready)**
- interpolated reads **111,677 (99.69%)**
- `i1` wraps **44 (0.039%)**

Warm 65,536-frame census after full prefill:
- 524,288 sine/LFO calls and line reads
- all **524,288** reads at full line capacity
- not-ready **0**
- minimum-delay-clamp opportunities **0**
- negative-wrap **521,026 (99.38%)**
- upper-wrap **0**
- integer reads **4,438 (0.85%)**
- interpolated reads **519,850 (99.15%)**
- `i1` wraps **224 (0.043%)**

Structural P11M bounds:
- min base delay 1,871 samples and modulation bound −2,304..+2,303 Q8 make the minimum-delay clamp unreachable;
- capacity `base_delay+16` and ±9-sample modulation guarantee raw non-negative reads stay far below span and negative reads need at most one `+span`, so the upper-wrap correction is unreachable;
- once `fill_count==capacity`, `needed<=base_delay+9<capacity`, so readiness failure is permanently impossible while startup semantics remain required before full capacity.

Decision: P11N may prove only a narrow production-caller-specialized reader: partial lines retain the exact readiness calculation, full lines bypass it; unreachable minimum-delay and upper-wrap branches may be removed; negative-wrap, interpolation and `i1` wrap remain exact. Defensive helper-guard removal is not authorized without separate call-path proof.

## Exact next phase — P11N FDN line-read invariant proof

Before production modification, prove current and candidate helper outputs bit-identical across all eight line topologies, boundary modulation/readiness/wrap classes and deterministic randomized valid states, then confirm complete FDN render equivalence under cold/warm/randomized trajectories. Run optimized + ASAN/UBSAN. Only then may an ephemeral real-target projection be used to decide whether a production phase is justified.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
if old_next not in s:
    raise SystemExit('PROJECT_STATE exact-next anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11M roadmap/project state updated')
