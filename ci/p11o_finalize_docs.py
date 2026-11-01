#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11N are complete; P11O is authorized as the separate production-validation phase for the proved FDN line-read simplification.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11O are complete; P11P is the next measurement-only phase for the remaining high-frequency FDN LFO/sine path.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
old = '''### P11O — production FDN line-read validation

Status: **[ ] next.**

- [ ] apply only the proved `space_line_read()` simplification to production DSP
- [ ] retain defensive helper guard, exact partial-line readiness, negative wrap, interpolation and `i1` wrap
- [ ] actual-production helper/full-FDN equivalence against frozen P11L authority
- [ ] complete historical host + ASAN/UBSAN behavior
- [ ] exact real JieLi target match to P11N projection
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final target/package evidence before promotion

### Later Phase-11 work
'''
new = '''### P11O — production FDN line-read validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production `space_line_read()` uses only the P11N-proved specialization
- [x] defensive helper guard, exact partial-line readiness, negative wrap, interpolation and `i1` wrap retained
- [x] actual production helper/full-FDN bit-identical to frozen P11L authority across **905,504** helper cases and **3,328 blocks / 210,677 frames**
- [x] optimized + ASAN/UBSAN production equivalence PASS
- [x] complete historical host + sanitizer behavior PASS
- [x] actual real-target build exactly matches P11N projection
- [x] production image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- [x] production package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- [x] inlined FDN-render code shape **1,114 B / 374** static instruction lines
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,704 B**, RAM **87,540 B**, POOL **319,176 B**
- [x] active package SHA-256 `adb581ff61ab1dc48c7bd08f7ca83bfa33f786fb635c31f36813f40d964b54ed`
- [x] authoritative validation run `41356749785` on production head `6560118c1cf7dfa6e40d1e33e86167d4f0b6ea5b`
- [x] evidence artifact ID `13104130300`; ZIP SHA-256 `0d747a515218da33e6bd9d61ae027aea64ce0a18033f6c4af64d32b27ccd0d9d`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11O production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`

P11O proves exact behavior and a strict inlined code-shape reduction with no image/RAM/POOL cost. Physical CPU/ISR improvement remains unverified.

### P11P — FDN LFO/sine workload census

Status: **[ ] next. Measurement only; no production patch authorized.**

- [ ] quantify `space_sine_q15()` / quarter-wave lookup work on cold and fully warm FDN paths
- [ ] classify interpolation and quadrant/lookup behavior before selecting any candidate
- [ ] capture real-target static code shape for the sine/LFO path and surrounding inlined render
- [ ] separate measured repeated work from structural invariants
- [ ] authorize proof work only if a narrow exact candidate is justified

### Later Phase-11 work
'''
if old not in s: raise SystemExit('ROADMAP P11O anchor mismatch')
s = s.replace(old, new, 1)
old_next = '''## Exact next action

Promote frozen P11N proof/projection state to `p11-optimization`, then create one P11O production-validation branch. Apply only the proved FDN line-read simplification and require actual-production equivalence, full historical host/sanitizer, real-target, exact idle and exact active-Motion gates before promotion.'''
new_next = '''## Exact next action

Promote frozen P11O production state to `p11-optimization`, then create one P11P measurement branch. Measure the remaining high-frequency FDN LFO/sine path without changing production DSP; only a narrow evidence-backed candidate may advance to a later proof phase.'''
if old_next not in s: raise SystemExit('ROADMAP next-action anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11N are complete. P11L remains the production baseline; P11N proved and target-projected the invariant-simplified FDN line reader, and P11O is authorized as the separate production-validation phase.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11O are complete. P11O is the current accepted production baseline after the invariant-simplified FDN line reader; P11P is the next measurement-only phase.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
old_base = '''## Current frozen production baseline — P11L

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
new_base = '''## Current frozen production baseline — P11O

P11O is the current accepted production target after the P11M/P11N measured-and-proved FDN line-read simplification layered over the P11L feedback-gain cache:

- product `FM-1_972`
- image **140,440 / 581,564 B**
- ordinary RAM **87,492 / 98,304 B**
- RAM headroom **10,812 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined FDN render **1,114 B / 374** static instruction lines
'''
if old_base not in s: raise SystemExit('PROJECT_STATE baseline anchor mismatch')
s = s.replace(old_base, new_base, 1)
old_next = '''## Exact next phase — P11O production FDN line-read validation

Apply the proved candidate to production DSP, then require actual-production equivalence against frozen P11L authority, complete historical host/sanitizer behavior, exact real-target match to the P11N projection, exact idle PCM/runtime and exact active Motion lifecycle before promotion.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
new_next = '''## P11O — production FDN line-read validation complete

Production authority:
- validated production head `6560118c1cf7dfa6e40d1e33e86167d4f0b6ea5b`
- validation run `41356749785` — PASS
- workflow frozen manual-only at that production head
- evidence artifact ID `13104130300`
- evidence ZIP SHA-256 `0d747a515218da33e6bd9d61ae027aea64ce0a18033f6c4af64d32b27ccd0d9d`

Actual production equivalence:
- **905,504** helper comparisons against frozen P11L reader
- **3,328 blocks / 210,677 frames** complete-FDN comparison
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS
- exact partial-fill zero-return, negative wrap, fractional interpolation and `i1` wrap retained

Frozen production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined FDN render **1,114 B / 374** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Active Motion authority remains exact:
- active image **137,704 B**
- RAM **87,540 B**; headroom **10,764 B**
- POOL **319,176 B**
- package SHA-256 `adb581ff61ab1dc48c7bd08f7ca83bfa33f786fb635c31f36813f40d964b54ed`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

P11O therefore replaces P11L as the accepted production baseline. It proves exact behavior and target code shape, not physical FM-1 cycle or ISR improvement.

## Exact next phase — P11P FDN LFO/sine workload census

Measure the remaining high-frequency FDN LFO/sine path without changing production DSP. Quantify sine lookup/interpolation behavior and target code shape, then authorize a later proof phase only if the evidence identifies a narrow exact candidate.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE P11O anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11O roadmap/project state updated')
