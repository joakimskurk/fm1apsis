#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11M are complete; P11N is proof-only for the measured FDN line-read invariant simplification candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11N are complete; P11O is authorized as the separate production-validation phase for the proved FDN line-read simplification.**'
if old_status not in s:
    raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)

old = '''### P11N — FDN line-read invariant simplification proof

Status: **[ ] next. No production patch until equivalence is proved.**

- [ ] prove current P11L line-read authority vs candidate across all eight line topologies, boundary modulation values, readiness thresholds, ring boundaries and deterministic randomized valid states
- [ ] preserve exact partial-fill/startup zero-return behavior
- [ ] preserve negative wrap, fractional interpolation and `i1` wrap exactly
- [ ] full-FDN render equivalence across cold, warm, randomized phase/state and reset trajectories
- [ ] optimized + ASAN/UBSAN proof
- [ ] only after proof, measure an ephemeral real-target code-shape/resource projection before production authorization
'''
new = '''### P11N — FDN line-read invariant simplification proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production Space source remains P11L.**

- [x] current P11L helper vs candidate bit-identical across all eight line topologies and every integer modulation value **−2,304 .. +2,303 Q8**
- [x] readiness/ring boundary matrix + **500,000** deterministic randomized valid states
- [x] total helper comparisons **905,504**
- [x] complete-FDN authority/candidate comparison **3,328 blocks / 210,677 frames**
- [x] cold startup, warm continuation, randomized frame/decay/damping, resets and randomized LFO phases covered
- [x] exact partial-fill zero-return, negative wrap, fractional interpolation and `i1` wrap preserved
- [x] optimized + ASAN/UBSAN PASS
- [x] authoritative proof run `41355949491` on combined head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- [x] ephemeral target projection run `41355949486` on the same head
- [x] P11L baseline regenerated exactly before candidate application
- [x] target image **140,440 B**, RAM **87,492 B**, POOL **319,176 B** — all unchanged
- [x] candidate package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- [x] inlined FDN-render code shape **1,118 → 1,114 B**, **379 → 374** static instruction lines
- [x] evidence artifact ID `13103071866`; ZIP SHA-256 `6d39137a817dd8a2d842790c40b4f51f6d0c0ef12ce7b33c56733f6aaa84711b`
- [x] repo production Space blobs remained exact P11L throughout P11N
- [x] proof/projection workflows manual-only and pinned to the combined proven head

Decision: authorize P11O production validation. The candidate has zero target-resource cost, a small strict code-shape reduction, and removes repeated full-line readiness work identified by P11M. Physical CPU/ISR improvement remains unverified.

### P11O — production FDN line-read validation

Status: **[ ] next.**

- [ ] apply only the proved `space_line_read()` simplification to production DSP
- [ ] retain defensive helper guard, exact partial-line readiness, negative wrap, interpolation and `i1` wrap
- [ ] actual-production helper/full-FDN equivalence against frozen P11L authority
- [ ] complete historical host + ASAN/UBSAN behavior
- [ ] exact real JieLi target match to P11N projection
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final target/package evidence before promotion
'''
if old not in s:
    raise SystemExit('ROADMAP P11N anchor mismatch')
s = s.replace(old, new, 1)

old_next = '''## Exact next action

Promote frozen P11M measurement state to `p11-optimization`, then create one P11N proof branch. Prove the production-caller-specialized FDN line-read candidate bit-identical to current P11L behavior before any production source is modified.'''
new_next = '''## Exact next action

Promote frozen P11N proof/projection state to `p11-optimization`, then create one P11O production-validation branch. Apply only the proved FDN line-read simplification and require actual-production equivalence, full historical host/sanitizer, real-target, exact idle and exact active-Motion gates before promotion.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11M are complete. P11L remains the production baseline; P11M measured FDN line-read branch behavior and P11N is proof-only until the invariant-simplified reader is bit-identical.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11N are complete. P11L remains the production baseline; P11N proved and target-projected the invariant-simplified FDN line reader, and P11O is authorized as the separate production-validation phase.**'
if old_m not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)

old_next = '''## Exact next phase — P11N FDN line-read invariant proof

Before production modification, prove current and candidate helper outputs bit-identical across all eight line topologies, boundary modulation/readiness/wrap classes and deterministic randomized valid states, then confirm complete FDN render equivalence under cold/warm/randomized trajectories. Run optimized + ASAN/UBSAN. Only then may an ephemeral real-target projection be used to decide whether a production phase is justified.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
new_next = '''## P11N — FDN line-read invariant proof complete

Proof/projection authority:
- combined head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- authoritative proof re-run `41355949491` — PASS
- target-projection run `41355949486` — PASS
- optimized + ASAN/UBSAN proof PASS
- production P11L Space blobs remained unchanged

Helper proof:
- all eight FDN line topologies
- every integer production modulation value −2,304..+2,303 Q8
- readiness and ring boundary classes
- 500,000 deterministic randomized valid states
- **905,504** total helper comparisons, all bit-identical

Complete-FDN proof:
- cold startup 16,384 frames
- warm continuation 65,536 frames
- 2,048 deterministic randomized parameter/frame blocks with resets and randomized LFO phases
- **3,328 blocks / 210,677 frames** total
- bit-identical stereo output, FDN state, feedback-cache state and full delay storage after every block

Ephemeral real-target projection:
- exact P11L baseline regenerated first
- candidate patched only into generated target worktree
- image **140,440 B** unchanged
- RAM **87,492 B** unchanged; headroom **10,812 B**
- POOL **319,176 B** unchanged
- package bytes **610,110 B** unchanged
- candidate projection package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined `apsis_space_fdn_render` **1,118 → 1,114 B**, **379 → 374** static instruction lines
- artifact ID `13103071866`
- artifact ZIP SHA-256 `6d39137a817dd8a2d842790c40b4f51f6d0c0ef12ce7b33c56733f6aaa84711b`

P11M provides the repeated-work rationale: 100% of warm measured line reads and 85.37% of the cold census are full-line, so the candidate skips readiness setup on the dominant path while preserving exact startup semantics. Minimum-delay and upper-wrap branches are structurally unreachable under the frozen caller.

Decision: P11O may apply only the proved candidate to production `space_line_read()`. No other DSP/state/header change is authorized.

## Exact next phase — P11O production FDN line-read validation

Apply the proved candidate to production DSP, then require actual-production equivalence against frozen P11L authority, complete historical host/sanitizer behavior, exact real-target match to the P11N projection, exact idle PCM/runtime and exact active Motion lifecycle before promotion.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
if old_next not in s:
    raise SystemExit('PROJECT_STATE P11N anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11N roadmap/project state updated')
