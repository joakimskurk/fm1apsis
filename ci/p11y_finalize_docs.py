#!/usr/bin/env python3
from pathlib import Path

roadmap=Path('ROADMAP.md')
s=roadmap.read_text()
old_status='Status: **[~] active on `p11-optimization`. P11A–P11X are complete. P11W remains the accepted production baseline; P11X authorizes P11Y proof-only for the measured FDN output sign-mix candidate.**'
new_status='Status: **[~] active on `p11-optimization`. P11A–P11Y are complete. P11W remains the accepted production baseline; P11Y is a proved-but-rejected target regression. P11Z is the final pre-hardware Phase-11 closure/readiness matrix.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s=s.replace(old_status,new_status,1)
old='''### P11Y — fixed FDN output add/sub equivalence proof

Status: **[ ] next. Production remains P11W.**

- [ ] prove loop sign-multiply and fixed add/sub arithmetic identical over boundary/exhaustive/random mixed vectors
- [ ] prove complete FDN stereo output/state/delay-storage equivalence across cold/warm/legal/random/reset/adversarial trajectories
- [ ] optimized + ASAN/UBSAN PASS
- [ ] patch only an ephemeral generated target; committed Space remains P11W
- [ ] compare exact image/RAM/POOL/package and FDN render code shape to P11W
- [ ] reject if target shape/resources regress without physical timing evidence
- [ ] authorize production validation only if the real-target projection is non-regressive and justified

'''
new='''### P11Y — fixed FDN output add/sub equivalence proof

Status: **[x] complete, exact proof + real-target projection frozen; candidate rejected. Production remains P11W.**

- [x] **1,000,312** arithmetic cases bit-identical over extremes, boundaries and deterministic legal-Q15 random vectors
- [x] complete FDN **6,311 blocks / 386,191 frames** bit-identical across cold/warm/legal/random/reset/adversarial trajectories
- [x] stereo output, complete FDN state and full delay storage exact after every block
- [x] optimized + ASAN/UBSAN PASS
- [x] exact P11W baseline regenerated first: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines
- [x] ephemeral add/sub target builds successfully but regresses image to **140,520 B (+48 B)**
- [x] RAM **87,524 B** and POOL **319,176 B** unchanged
- [x] candidate FDN render **1,154 B / 385** lines = **+60 B / +21 lines**
- [x] candidate package SHA-256 `01d9f3fbc225973b6f2dcb861a86c2ee62af49594a288ba43c7cd2fac4ba61d0`
- [x] authoritative run `41397297203` on technical head `ca46ac5a5d3db9b9f0e43adb061ba780fab39c9a`
- [x] evidence artifact ID `13119718531`; ZIP SHA-256 `dde978fd3a521dac64c3c5cb1a66e417b39a3a70508be2569481e765c228f1ec`
- [x] committed production Space remains exact P11W
- [x] historical proof workflow manual-only and pinned to the proven technical head

Decision: **reject production validation.** The candidate removes 16 explicit sign multiplications/frame semantically, but the actual JieLi target is strictly worse on every changed static proxy: +48 B image, +60 B render text, +21 render lines, with no RAM/POOL saving. Without physical cycle/ISR evidence, that regression is not acceptable. Preserve P11Y as a negative result and keep P11W production.

### P11Z — Phase-11 closure/readiness matrix

Status: **[ ] next. Measurement/regression only; no new production optimization.**

- [ ] regenerate exact P11W production target/package and current ownership/resource inventory
- [ ] rerun permanent high-water/stress authorities for scheduler, Bloom, Spiral, String transient/Attractor and Motion
- [ ] run complete historical host + sanitizer matrix on the accepted P11W source
- [ ] run exact idle pinned-emulator PCM/runtime gate
- [ ] run exact active Motion lifecycle gate
- [ ] capture final pre-hardware image/RAM/POOL/package hashes and explicit unmeasured hardware boundary
- [ ] freeze one Phase-11 closure head and hand Phase 12 only the genuinely physical checks

P11Z is intended to close Phase 11, not discover another speculative micro-optimization. New optimization work after P11Z requires new evidence or physical timing data.

'''
if old not in s: raise SystemExit('ROADMAP P11Y anchor mismatch')
s=s.replace(old,new,1)
old_next='''## Exact next action

Promote frozen P11X measurement state to `p11-optimization`, then create one P11Y proof branch from that freeze. Keep production at P11W. Prove fixed add/sub output mixing against the frozen sign-multiply authority and project it ephemerally on the real JieLi target before any production-validation decision.'''
new_next='''## Exact next action

Promote frozen P11Y proof/rejection state to `p11-optimization`, then create one P11Z closure branch. Do not change production DSP. Run the final Phase-11 pre-hardware readiness matrix against accepted P11W and freeze a single closure head that cleanly separates verified software/target/emulator evidence from Phase-12 physical-hardware work.'''
if old_next not in s: raise SystemExit('ROADMAP next anchor mismatch')
roadmap.write_text(s.replace(old_next,new_next,1))

state=Path('PROJECT_STATE.md')
s=state.read_text()
old_m='**Phase 11 — FM-1 optimisation is active. P11A–P11X are complete. P11W remains the accepted production baseline. P11X measured explicit target sign multiplies in the FDN output mix and authorizes P11Y proof-only.**'
new_m='**Phase 11 — FM-1 optimisation is active. P11A–P11Y are complete. P11W remains the accepted production baseline. P11Y proved fixed add/sub output mixing exact but rejected it on real-target regression; P11Z is the final pre-hardware closure/readiness matrix.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s=s.replace(old_m,new_m,1)
old_next='''## Exact next phase — P11Y fixed output add/sub proof

Production remains P11W. Prove arithmetic and complete FDN behavior bit-identical, normal + ASAN/UBSAN, then patch only an ephemeral generated target and compare exact resource/package/render shape. Reject any regressive target candidate absent physical timing evidence. Physical ISR timing, stack high-water, LCD/audio contention, NOR timing and DAC/listening quality remain Phase-12 hardware work.
'''
new_next='''## P11Y — fixed FDN output add/sub proof complete; candidate rejected

Authority:
- technical head `ca46ac5a5d3db9b9f0e43adb061ba780fab39c9a`
- run `41397297203` — all gates PASS
- evidence artifact ID `13119718531`
- evidence ZIP SHA-256 `dde978fd3a521dac64c3c5cb1a66e417b39a3a70508be2569481e765c228f1ec`
- committed production Space remains exact P11W

Equivalence proof:
- **1,000,312** fixed-sign arithmetic cases exact
- complete FDN **6,311 blocks / 386,191 frames** exact
- cold/warm steady, legal sweep, adversarial extremes, random frames/parameters and resets covered
- stereo output, complete FDN state and full delay storage bit-identical after every block
- optimized + ASAN/UBSAN PASS

Exact real-target projection:
- P11W baseline: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines, package `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- fixed add/sub candidate: image **140,520 B (+48 B)**, RAM **87,524 B**, POOL **319,176 B**
- candidate render **1,154 B / 385** = **+60 B / +21 lines**
- candidate package SHA-256 `01d9f3fbc225973b6f2dcb861a86c2ee62af49594a288ba43c7cd2fac4ba61d0`

Decision: reject production validation. The candidate is mathematically/behaviorally exact, but every changed static target proxy regresses and there is no RAM/POOL saving. Physical timing is unavailable, so the 16 removed semantic sign multiplies/frame do not justify the larger target. P11W remains production authority.

## Exact next phase — P11Z Phase-11 closure/readiness matrix

No production optimization is authorized. Regenerate and stress the accepted P11W stack as one final software/real-target/pinned-emulator checkpoint: complete host/sanitizer behavior, bounded-capacity/high-water authorities, exact production target/package, exact idle PCM/runtime and exact active Motion lifecycle. Freeze one closure head and move all remaining physical ISR/stack/LCD/NOR/DAC/listening checks to Phase 12.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE P11Y anchor mismatch')
state.write_text(s.replace(old_next,new_next,1))
print('PASS: P11Y rejection and P11Z closure state updated')
