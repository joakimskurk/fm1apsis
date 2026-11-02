#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11W are complete. P11W is the accepted production baseline; any further production optimization requires a new measured, equivalence-backed candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11X are complete. P11W remains the accepted production baseline; P11X authorizes P11Y proof-only for the measured FDN output sign-mix candidate.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
marker = '### Later Phase-11 work\n'
insert = '''### P11X — FDN output sign-mix census

Status: **[x] complete, frozen measurement phase. Production remains exact P11W.**

- [x] both frozen output sign vectors contain only ±1: left **5+/3−**, right **5+/3−**
- [x] structural absolute eight-term sum bound **262,136**, signed-32 safe with **2,147,221,511** headroom
- [x] current source workload **16 sign products/frame**
- [x] cold 16,384-frame census: **262,144** sign products
- [x] warm 65,536-frame census: **1,048,576** sign products
- [x] optimized + ASAN/UBSAN PASS
- [x] exact P11W target regenerated: **140,472 B image / 87,524 B RAM / 319,176 B POOL**, package `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- [x] raw JieLi render disassembly captured for all **364** static instruction lines
- [x] target loop loads signed `SPACE_OUT_RIGHT[k]`/`SPACE_OUT_LEFT[k]` and executes explicit `r1 *= r0` + `r0 *= r2` — **two general sign multiplies per FDN line**
- [x] authoritative run `41396085585` on technical head `a28fb0047550b729c15cc7e5bc3413a71a13819b`
- [x] evidence artifact ID `13119167571`; ZIP SHA-256 `5ab3d57b84c7fb941cabc46ae478144763d8cc6e2661c9919f8d996ffab51c66`
- [x] production Space blobs remain exact P11W
- [x] historical workflow manual-only and pinned to the proven technical head

Decision: authorize P11Y **proof only** for replacing the two fixed ±1 dot products with explicit fixed add/sub accumulation. The generic opcode histogram is not treated as semantic JieLi-ISA evidence; the raw disassembly lines are the authority.

### P11Y — fixed FDN output add/sub equivalence proof

Status: **[ ] next. Production remains P11W.**

- [ ] prove loop sign-multiply and fixed add/sub arithmetic identical over boundary/exhaustive/random mixed vectors
- [ ] prove complete FDN stereo output/state/delay-storage equivalence across cold/warm/legal/random/reset/adversarial trajectories
- [ ] optimized + ASAN/UBSAN PASS
- [ ] patch only an ephemeral generated target; committed Space remains P11W
- [ ] compare exact image/RAM/POOL/package and FDN render code shape to P11W
- [ ] reject if target shape/resources regress without physical timing evidence
- [ ] authorize production validation only if the real-target projection is non-regressive and justified

'''
if marker not in s: raise SystemExit('ROADMAP Later anchor mismatch')
s = s.replace(marker, insert + marker, 1)
old_next = '''## Exact next action

Promote frozen P11W production state to `p11-optimization`, then return to measurement before selecting P11X. Do not infer physical CPU/ISR improvement from static code-shape proxies. Any further production optimization requires a new measured hotspot, explicit structural bounds where relevant, an equivalence proof and non-regressive real-target evidence.'''
new_next = '''## Exact next action

Promote frozen P11X measurement state to `p11-optimization`, then create one P11Y proof branch from that freeze. Keep production at P11W. Prove fixed add/sub output mixing against the frozen sign-multiply authority and project it ephemerally on the real JieLi target before any production-validation decision.'''
if old_next not in s: raise SystemExit('ROADMAP next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11W are complete. P11W is exact-head validated and is the accepted production baseline; further production optimization returns to measurement-first selection.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11X are complete. P11W remains the accepted production baseline. P11X measured explicit target sign multiplies in the FDN output mix and authorizes P11Y proof-only.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
old_next = '''## Exact next phase

Return to measurement before P11X. Select no new production optimization unless a concrete remaining workload/resource hotspot is measured, bounded where necessary, proved equivalent and shown non-regressive on the real JieLi target. Physical ISR timing, stack high-water, LCD/audio contention, NOR timing and DAC/listening quality remain Phase-12 hardware work.
'''
new_next = '''## P11X — FDN output sign-mix census complete

Authority:
- technical head `a28fb0047550b729c15cc7e5bc3413a71a13819b`
- run `41396085585` — PASS
- evidence artifact ID `13119167571`
- evidence ZIP SHA-256 `5ab3d57b84c7fb941cabc46ae478144763d8cc6e2661c9919f8d996ffab51c66`
- production Space source/header remain exact P11W

Measured facts:
- left/right sign vectors are fixed ±1 only, each **5 positive / 3 negative**
- eight-term Q15 signed-sum absolute bound **262,136**, safely inside int32
- source performs **16 sign products/frame**
- cold 16,384 frames → **262,144** sign products
- warm 65,536 frames → **1,048,576** sign products
- optimized + ASAN/UBSAN PASS

Exact target authority:
- P11W package regenerated exactly: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, package `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- render remains **364** static instruction lines
- raw JieLi loop loads signed sign-vector bytes and executes `r1 *= r0` and `r0 *= r2` for each line
- therefore the compiler retains **two general sign multiplications/line = 16/frame**
- the generic opcode classifier is not semantic authority for JieLi assignment-style disassembly; raw disassembly is

P11X authorizes only a P11Y proof candidate: replace the frozen sign products with explicit add/sub using exactly the same sign pattern. No production change is authorized.

## Exact next phase — P11Y fixed output add/sub proof

Production remains P11W. Prove arithmetic and complete FDN behavior bit-identical, normal + ASAN/UBSAN, then patch only an ephemeral generated target and compare exact resource/package/render shape. Reject any regressive target candidate absent physical timing evidence. Physical ISR timing, stack high-water, LCD/audio contention, NOR timing and DAC/listening quality remain Phase-12 hardware work.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE exact-next anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11X roadmap/project state updated')
