#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11U are complete; P11R remains the accepted production baseline. P11U identifies cached sine delta as the next proof-only candidate; P11V is authorized for equivalence + ephemeral target projection only.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11W are complete. P11W is the accepted production baseline; any further production optimization requires a new measured, equivalence-backed candidate.**'
if old_status not in s:
    raise SystemExit('ROADMAP Phase-11 status anchor mismatch')
s = s.replace(old_status, new_status, 1)

old_p11w = '''### P11W — production FDN sine A+delta cache validation

Status: **[ ] next.**

- [ ] replace only cached `B` with exact signed `delta=B-A`; preserve two-int16 state width
- [ ] reset/refresh store exact A and delta; interpolation remains `A + ((delta*frac)>>8)`
- [ ] actual-production equivalence against frozen P11R A+B authority, optimized + ASAN/UBSAN
- [ ] complete historical host + sanitizer behavior
- [ ] exact real JieLi target must match P11V projection: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final package hashes before promotion
'''
new_p11w = '''### P11W — production FDN sine A+delta cache validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production replaces only cached `B` with exact signed `delta=B-A`; two-int16 state width remains unchanged
- [x] reset/refresh store exact A and delta; interpolation remains the proved `A + ((delta*frac)>>8)` form
- [x] committed production is byte-identical to the P11V-proved candidate regenerated from frozen P11R authority
- [x] complete historical host + sanitizer behavior PASS
- [x] state remains `ApsisSpaceFdnLine` **28 B**, `ApsisSpaceFdn` **264 B**
- [x] real JieLi target exactly matches P11V projection: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**
- [x] inlined FDN render **1,094 B / 364** static instruction lines
- [x] production package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,736 B**, RAM **87,572 B**, POOL **319,176 B**
- [x] active package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- [x] authoritative validation run `41393330606` on production head `ac39372d8181c7fb27c9c25b55e825a5db53e10c`
- [x] evidence artifact ID `13118836564`; ZIP SHA-256 `c866cf8b844b80e31d070c934f9c26dc7378f21fe7acbae36f9c1ac550bb1c05`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11W production target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- FDN render **1,094 B / 364** static instruction lines

P11W replaces P11R as the accepted production baseline. It keeps target image/RAM/POOL size unchanged relative to P11R while removing two additional static render instruction lines through the zero-growth A+delta representation. Physical CPU/ISR improvement remains unverified.
'''
if old_p11w not in s:
    raise SystemExit('ROADMAP P11W anchor mismatch')
s = s.replace(old_p11w, new_p11w, 1)

old_next = '''## Exact next action

Promote frozen P11V proof/projection state to `p11-optimization`, then create one P11W production-validation branch. Apply only the proved zero-growth A+B → A+delta cache representation and require complete actual-production equivalence, historical, real-target, idle and active-Motion gates before promotion.'''
new_next = '''## Exact next action

Promote frozen P11W production state to `p11-optimization`, then return to measurement before selecting P11X. Do not infer physical CPU/ISR improvement from static code-shape proxies. Any further production optimization requires a new measured hotspot, explicit structural bounds where relevant, an equivalence proof and non-regressive real-target evidence.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11V are complete. P11R remains the accepted production baseline. P11V proved and real-target-projected the zero-growth A+delta sine cache and authorizes P11W production validation.**'
new_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11W are complete. P11W is exact-head validated and is the accepted production baseline; further production optimization returns to measurement-first selection.**'
if old_milestone not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_milestone, new_milestone, 1)

old_baseline = '''## Current frozen production baseline — P11R

P11R is the current accepted production target after the P11P/P11Q measured-and-proved FDN sine-endpoint cache layered over the P11O line-read specialization and P11L feedback-gain cache:

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**
- RAM headroom **10,780 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- inlined FDN render **1,098 B / 366** static instruction lines
'''
new_baseline = '''## Current frozen production baseline — P11W

P11W is the current accepted production target after the zero-growth P11V-proved A+B → A+delta sine-cache representation layered over the P11R endpoint cache, P11O line-read specialization and P11L feedback-gain cache:

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**
- RAM headroom **10,780 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines
'''
if old_baseline not in s:
    raise SystemExit('PROJECT_STATE baseline anchor mismatch')
s = s.replace(old_baseline, new_baseline, 1)

marker = '## Exact next phase — P11W production sine A+delta validation\n'
end = '\n## Do not regress\n'
if marker not in s or end not in s:
    raise SystemExit('PROJECT_STATE P11W section anchors missing')
before, rest = s.split(marker, 1)
_, after = rest.split(end, 1)
p11w = '''## P11W — production FDN sine A+delta validation complete

Production authority:
- exact technical production head `ac39372d8181c7fb27c9c25b55e825a5db53e10c`
- authoritative run `41393330606` — PASS
- evidence artifact ID `13118836564`
- evidence ZIP SHA-256 `c866cf8b844b80e31d070c934f9c26dc7378f21fe7acbae36f9c1ac550bb1c05`
- validation workflow frozen manual-only and pinned to the proven production head

Production source authority:
- `apsis_space_dsp.c` blob `3bcd294dd9fb610df888ddd0517da7de3bf4c086`
- `apsis_space_dsp.h` blob `ec221e22aeb6670ec628cbb0cc7c7eea91038554`
- Cathedral room source remains `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`
- committed source regenerated byte-for-byte from frozen P11R authority plus the P11V candidate transform

State/algorithm:
- `ApsisSpaceFdnLine` remains **28 B**
- `ApsisSpaceFdn` remains **264 B**
- cached sine representation changes only from exact A+B to exact A+delta (`delta=B-A`)
- reset/refresh and interpolation retain the P11V-proved semantics
- no FDN topology, phase/increment, room, feedback or scheduler change

Frozen production target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- FDN render **1,094 B / 364** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- peak 856; RMS 144/144; stereo delta 126

Active Motion authority remains exact:
- active image **137,736 B**
- RAM **87,572 B**; headroom **10,732 B**
- POOL **319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

P11W therefore replaces P11R as the accepted production baseline. Relative to P11R it has zero image/RAM/POOL growth and reduces the inlined FDN render from 1,098 B / 366 lines to 1,094 B / 364 lines. This is exact static/proxy evidence, not physical FM-1 cycle timing.

## Exact next phase

Return to measurement before P11X. Select no new production optimization unless a concrete remaining workload/resource hotspot is measured, bounded where necessary, proved equivalent and shown non-regressive on the real JieLi target. Physical ISR timing, stack high-water, LCD/audio contention, NOR timing and DAC/listening quality remain Phase-12 hardware work.
'''
state.write_text(before + p11w + end + after)
print('PASS: P11W roadmap/project state updated')
