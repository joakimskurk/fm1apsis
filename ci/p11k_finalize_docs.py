#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11J are complete; P11K is an equivalence-proof phase for the measured FDN feedback-gain cache candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11K are complete; P11L is authorized as the separate production-validation phase for the proved FDN feedback-gain cache.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
old = '''### P11K — feedback-gain cache equivalence proof

Status: **[ ] next. No production patch until equivalence is proved.**

- [ ] compare always-recomputed and cached feedback-gain paths across reset/partial-fill/steady/sweeps/adversarial decay sequences
- [ ] preserve `space_feedback_gain()` arithmetic exactly; cache only its eight outputs + rounded decay key/validity
- [ ] normal + ASAN/UBSAN bit-identical output/state proof
- [ ] measure projected host/target state-size cost before production authorization
- [ ] only after proof, decide whether a separate production-validation stage is justified
'''
new = '''### P11K — feedback-gain cache equivalence proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production FDN source/layout remain P11I.**

- [x] cached vs always-recomputed complete FDN output/state/storage bit-identical
- [x] reset/partial fill, steady, smoothed sweep, adversarial churn, all **8,001** legal decay keys and 4,096 random/reset blocks covered
- [x] optimized + ASAN/UBSAN PASS; host external cache representation **20 B**
- [x] steady: **8** gain evaluations vs 2,048 baseline; sweep: **1,544**; adversarial: **2,048**
- [x] proof run `41350818792` on head `ffb436dd87247af23c37e7e533cfd87fc2d161d0`
- [x] ephemeral target projection run `41351123630` on head `72b48c499ce7619fd3a077edac664ea2c65c4ca5`
- [x] projected target delta: image **+48 B**, RAM **+16 B**, POOL **0 B**
- [x] projected target RAM **87,492 B**; headroom **10,812 B**
- [x] FDN-render code shape **1,082 → 1,118 B**, **353 → 379** static instruction lines
- [x] evidence artifact ID `13101765292`; ZIP SHA-256 `a5cf89df0e49c8d79c5118db8f444f74a65808e6ca0f820e43bfda4d40ff0ef3`
- [x] repo production FDN source/header blobs remained unchanged throughout P11K
- [x] proof/projection workflows manual-only and pinned to their proven heads

Decision: authorize P11L production validation. The cache intentionally spends 16 B target RAM / 48 B image to remove 2,040 divisions + 6,120 wide multiplies in steady census and 504 divisions + 1,512 wide multiplies in the measured one-way sweep. Worst-case adversarial churn has zero coefficient-computation savings.

### P11L — production feedback-gain cache validation

Status: **[ ] next.**

- [ ] apply only the proved eight-gain cache + rounded decay key/validity to production FDN state/render/reset
- [ ] keep `space_feedback_gain()` arithmetic exact and all other FDN math/topology unchanged
- [ ] run P11K equivalence proof against actual production implementation
- [ ] full historical host + ASAN/UBSAN behavior green
- [ ] exact real JieLi target resource measurement
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final image/RAM/POOL/package hashes before promotion
'''
if old not in s: raise SystemExit('ROADMAP P11K anchor mismatch')
s = s.replace(old, new, 1)
old_next = '''## Exact next action

Promote frozen P11J measurement state to `p11-optimization`, then create one P11K proof branch. Prove an eight-value feedback-gain cache keyed by rounded `decay_ms` bit-identical to the current always-recomputed FDN before any production source is modified.'''
new_next = '''## Exact next action

Promote frozen P11K proof/projection state to `p11-optimization`, then create one P11L production-validation branch. Apply only the proved FDN feedback-gain cache and require complete host/sanitizer, real-target, exact idle and exact active-Motion gates before promotion.'''
if old_next not in s: raise SystemExit('ROADMAP next-action anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11J are complete. P11J measured FDN feedback-gain recomputation redundancy; P11K is proof-only until cached and always-recomputed FDN behavior are bit-identical.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11K are complete. P11K proved and target-projected the FDN feedback-gain cache without changing production source; P11L is authorized as the separate production-validation phase.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
old_next = '''## Exact next phase — P11K feedback-gain cache proof

Before production modification:
1. implement baseline and candidate paths in a proof harness or ephemeral source transformation;
2. cover reset, partial fill, steady state, legal smoothed transitions and adversarial decay churn;
3. require bit-identical stereo output and all pre-existing FDN musical/runtime state;
4. run optimized + ASAN/UBSAN;
5. measure exact projected state-size cost of the cache;
6. only if all proof gates pass, authorize a separate production-validation patch.

P11J/P11K remain workload/equivalence work. Physical FM-1 CPU cycles and ISR margin are still unverified.
'''
new_next = '''## P11K — FDN feedback-gain cache proof complete

Equivalence authority:
- proof head `ffb436dd87247af23c37e7e533cfd87fc2d161d0`
- run `41350818792`
- optimized + ASAN/UBSAN PASS
- complete baseline/candidate stereo output, existing FDN state and full delay storage compared after every block
- reset/partial-fill, steady, exact room-style sweep, adversarial churn, all 8,001 legal decay keys and 4,096 deterministic random/reset blocks covered
- host external cache representation **20 B**

Measured proof counts:
- steady: 256 blocks / 16,384 frames → 1 recompute block / 8 gain evaluations
- 4s→12s smoothed sweep: 192 decay changes → 193 recompute blocks / 1,544 gain evaluations
- adversarial: 256 recompute blocks / 2,048 gain evaluations
- all legal keys: 16,002 blocks / 8,001 recomputes / 64,008 gain evaluations
- random/reset: 4,096 blocks / 92,927 frames / 3,840 recomputes / 30,720 gain evaluations

Ephemeral real-target projection authority:
- head `72b48c499ce7619fd3a077edac664ea2c65c4ca5`
- run `41351123630`
- artifact ID `13101765292`
- artifact ZIP SHA-256 `a5cf89df0e49c8d79c5118db8f444f74a65808e6ca0f820e43bfda4d40ff0ef3`
- exact P11I baseline regenerated before candidate patch
- candidate patched only in generated target worktree; repo production blobs remained exact

Projected target tradeoff:
- image **140,392 → 140,440 B** (**+48 B**)
- ordinary RAM **87,476 → 87,492 B** (**+16 B**); projected headroom **10,812 B**
- POOL **319,176 B** unchanged
- package bytes **610,110 B** unchanged
- projection package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- FDN-render code shape **1,082 → 1,118 B**, **353 → 379** static instruction lines

Workload payoff from P11J/P11K model:
- steady census: 2,040 fewer 32-bit divisions + 6,120 fewer wide multiplies
- measured one-way sweep: 504 fewer divisions + 1,512 fewer wide multiplies
- adversarial every-block decay churn: zero coefficient-computation savings, with only the cache-key decision added

P11K therefore authorizes P11L production validation. It does not claim physical FM-1 cycle or ISR improvement.

## Exact next phase — P11L production feedback-gain cache validation

Apply exactly the proved cache to production FDN state/reset/render. Preserve `space_feedback_gain()` arithmetic and all other FDN math. Require actual-production equivalence proof, full historical host/sanitizer behavior, exact real-target resources, exact idle PCM/runtime and exact active Motion lifecycle before promotion.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE P11K anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11K roadmap/project state updated')
