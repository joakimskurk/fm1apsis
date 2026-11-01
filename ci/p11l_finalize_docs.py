#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11K are complete; P11L is authorized as the separate production-validation phase for the proved FDN feedback-gain cache.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11L are complete; further production optimisation requires a new measured, equivalence-backed candidate.**'
if old_status not in s:
    raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)

old_p11l = '''### P11L — production feedback-gain cache validation

Status: **[ ] next.**

- [ ] apply only the proved eight-gain cache + rounded decay key/validity to production FDN state/render/reset
- [ ] keep `space_feedback_gain()` arithmetic exact and all other FDN math/topology unchanged
- [ ] run P11K equivalence proof against actual production implementation
- [ ] full historical host + ASAN/UBSAN behavior green
- [ ] exact real JieLi target resource measurement
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final image/RAM/POOL/package hashes before promotion
'''
new_p11l = '''### P11L — production feedback-gain cache validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production FDN state stores only the eight exact gains + rounded decay key/validity
- [x] reset invalidates cache; render recomputes only when rounded decay changes
- [x] `space_feedback_gain()` arithmetic and all other FDN math/topology remain exact
- [x] actual production render is bit-identical to frozen always-recomputed authority across reset/partial fill, steady, smoothed sweep, adversarial churn, all **8,001** legal decay keys and 4,096 random/reset blocks
- [x] optimized + ASAN/UBSAN production equivalence PASS
- [x] complete historical host + sanitizer behavior PASS
- [x] actual real-target build exactly matches P11K projection
- [x] production image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- [x] production package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,704 B**, RAM **87,540 B**, POOL **319,176 B**
- [x] active package SHA-256 `54616e07ad5b579d33ec07732b5ec3daa824d71950ec8844805631251245374a`
- [x] authoritative run `41351791632` on production head `4a08705815fffcbf9407193a21334764d8710d51`
- [x] evidence artifact ID `13101743282`; ZIP SHA-256 `819f932f701d2e46cf13b26a641d68130785f84cc026eec39347563903f24d72`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11L production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`

Accepted dynamic workload tradeoff from P11J–P11L: steady census eliminates 2,040 coefficient divisions + 6,120 wide multiplies; the measured 4s→12s sweep eliminates 504 divisions + 1,512 wide multiplies; adversarial every-block decay churn has zero coefficient-computation savings. Physical CPU/ISR improvement remains unverified.
'''
if old_p11l not in s:
    raise SystemExit('ROADMAP P11L anchor mismatch')
s = s.replace(old_p11l, new_p11l, 1)

old_next = '''## Exact next action

Promote frozen P11K proof/projection state to `p11-optimization`, then create one P11L production-validation branch. Apply only the proved FDN feedback-gain cache and require complete host/sanitizer, real-target, exact idle and exact active-Motion gates before promotion.'''
new_next = '''## Exact next action

Promote the frozen P11L production state to `p11-optimization`. Then return to measurement before selecting any P11M production candidate; no further DSP change is authorized until a concrete hotspot and falsifiable equivalence/regression gate are defined.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11K are complete. P11K proved and target-projected the FDN feedback-gain cache without changing production source; P11L is authorized as the separate production-validation phase.**'
new_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11L are complete. P11L is the current exact-head validated production baseline; further production optimisation requires a new measured/equivalence-backed candidate.**'
if old_milestone not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_milestone, new_milestone, 1)

old_baseline = '''## Current frozen production baseline — P11I

P11G and P11H were measurement-only. P11I is the current accepted production target after the equivalence-backed stereo-pair delay-read optimization:

- product `FM-1_972`
- image **140,392 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`
'''
new_baseline = '''## Current frozen production baseline — P11L

P11L is the current accepted production target after the P11J/P11K measured-and-proved FDN feedback-gain cache:

- product `FM-1_972`
- image **140,440 / 581,564 B**
- ordinary RAM **87,492 / 98,304 B**
- RAM headroom **10,812 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
'''
if old_baseline not in s:
    raise SystemExit('PROJECT_STATE baseline anchor mismatch')
s = s.replace(old_baseline, new_baseline, 1)

old_next = '''## Exact next phase — P11L production feedback-gain cache validation

Apply exactly the proved cache to production FDN state/reset/render. Preserve `space_feedback_gain()` arithmetic and all other FDN math. Require actual-production equivalence proof, full historical host/sanitizer behavior, exact real-target resources, exact idle PCM/runtime and exact active Motion lifecycle before promotion.
'''
new_next = '''## P11L — production FDN feedback-gain cache complete

Production authority:
- exact production head `4a08705815fffcbf9407193a21334764d8710d51`
- authoritative run `41351791632`, all six gates PASS
- artifact ID `13101743282`
- artifact ZIP SHA-256 `819f932f701d2e46cf13b26a641d68130785f84cc026eec39347563903f24d72`

Production change is narrow:
- eight exact cached feedback gains + rounded decay key/validity in `ApsisSpaceFdn`
- cache invalidated on reset
- exact gains recomputed only when rounded `decay_ms` changes
- `space_feedback_gain()` arithmetic, FDN topology and Cathedral room source unchanged

Actual production equivalence:
- optimized + ASAN/UBSAN PASS
- reset/partial fill, 256-block steady, exact smoothed sweep, adversarial churn, all 8,001 legal decay keys and 4,096 deterministic random/reset blocks
- bit-identical stereo output, all pre-existing FDN state and full FDN delay storage after every block

Frozen production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- FDN render **1,118 B / 379** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Active Motion authority remains exact:
- active image **137,704 B**
- RAM **87,540 B**; headroom **10,764 B**
- POOL **319,176 B**
- package SHA-256 `54616e07ad5b579d33ec07732b5ec3daa824d71950ec8844805631251245374a`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

Accepted workload tradeoff from P11J/P11K:
- steady census: 2,040 fewer coefficient divisions + 6,120 fewer wide multiplies
- measured one-way sweep: 504 fewer divisions + 1,512 fewer wide multiplies
- adversarial every-block decay churn: zero coefficient-computation savings

P11L proves exact behavior and real-target resource cost. It still does not prove physical FM-1 CPU cycles or ISR margin.

## Exact next phase

Promote P11L to `p11-optimization`, then return to measurement. No P11M production change is authorized until a new concrete hotspot and falsifiable equivalence/regression gate are established. Physical ISR timing, stack high-water, LCD/audio contention and listening quality remain deferred to hardware validation.
'''
if old_next not in s:
    raise SystemExit('PROJECT_STATE P11L next-phase anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11L roadmap/project state updated')
