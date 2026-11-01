#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11P are complete; P11Q is authorized as a proof-only phase for the measured FDN sine-endpoint cache candidate.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11Q are complete; P11R is authorized as the separate production-validation phase for the proved FDN sine-endpoint cache.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
old = '''### P11Q — FDN sine endpoint-cache equivalence proof

Status: **[ ] next. No production patch until proof and projection pass.**

- [ ] exhaustive 65,536-case top-16 sine equivalence
- [ ] all 256 top-byte transitions including `255 → 0` wrap and reset initialization
- [ ] complete-FDN output/state/storage equivalence across cold, warm, randomized trajectories and resets
- [ ] optimized + ASAN/UBSAN proof
- [ ] exact host and target state/layout cost
- [ ] ephemeral real-target image/RAM/POOL/package and inlined-render projection
- [ ] only after all gates pass, decide whether a separate production-validation phase is justified

### Later Phase-11 work
'''
new = '''### P11Q — FDN sine endpoint-cache equivalence proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production Space remains P11O.**

- [x] complete top-16 sine domain **65,536** cases bit-identical
- [x] all old→new top-byte transitions × fractions {0,1,127,255}: **262,144** cases, including `255 → 0`
- [x] complete-FDN comparison **5,376 blocks / 164,767 frames** with cold, warm, randomized trajectories, resets and randomized phases
- [x] cold hot-path endpoint refreshes **88** after exact reset initialization; warm **367** over 65,536 frames
- [x] optimized + ASAN/UBSAN PASS
- [x] full historical host + sanitizer behavior PASS with ephemeral candidate
- [x] host `ApsisSpaceFdnLine` **24 → 28 B**; `ApsisSpaceFdn` **232 → 264 B** (**+32 B**)
- [x] exact P11O target regenerated before projection
- [x] candidate target image **140,472 B** (**+32 B**), RAM **87,524 B** (**+32 B**), POOL **319,176 B** (**0 B**)
- [x] candidate package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] inlined FDN render **1,114 → 1,098 B**, **374 → 366** static instruction lines
- [x] `space_sine_point` remains **48 B / 14** static instruction lines; dead `space_sine_q15` wrapper removed in candidate
- [x] authoritative run `41360291892` on technical head `a0b0826c9f0951b780d84404cfdbe9543ba45b49`
- [x] evidence artifact ID `13104793891`; ZIP SHA-256 `7e7699b82e6540c64e099d4f8ea4d5a6d508e6a88d402a8c91c1d0aaa0cfcd05`
- [x] repo production Space blobs remained exact P11O throughout P11Q
- [x] proof/projection workflow manual-only and pinned to the proven technical head

Decision: authorize P11R production validation. The candidate spends exactly 32 B RAM / 32 B image, no POOL, while P11P measured endpoint-pair reuse on 99.929% of natural sine calls and P11Q produces a smaller target render hot path. Physical CPU/ISR improvement remains unverified.

### P11R — production FDN sine endpoint-cache validation

Status: **[ ] next.**

- [ ] apply only the exact P11Q endpoint-cache candidate to production `apsis_space_dsp.c/.h`
- [ ] keep reset phase, phase increment, interpolation, quadrant/wrap and all other FDN arithmetic/topology exact
- [ ] actual-production P11Q equivalence against frozen P11O authority
- [ ] complete historical host + ASAN/UBSAN behavior
- [ ] exact real JieLi target resources matching P11Q projection
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] narrow-diff verification and frozen final package hashes before promotion

### Later Phase-11 work
'''
if old not in s: raise SystemExit('ROADMAP P11Q anchor mismatch')
s = s.replace(old, new, 1)
old_next = '''## Exact next action

Promote frozen P11P measurement state to `p11-optimization`, then create one P11Q proof branch. Prove the per-line FDN sine-endpoint cache against frozen P11O authority and project it ephemerally on the real JieLi target; do not modify production Space until all equivalence and target-cost gates pass.'''
new_next = '''## Exact next action

Promote frozen P11Q proof/projection state to `p11-optimization`, then create one P11R production-validation branch. Apply only the proved FDN sine-endpoint cache and require actual-production equivalence, full historical host/sanitizer, exact real-target, exact idle and exact active-Motion gates before promotion.'''
if old_next not in s: raise SystemExit('ROADMAP next-action anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11P are complete. P11O remains the accepted production baseline; P11P measured the remaining FDN LFO/sine endpoint redundancy and authorizes P11Q proof-only.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11Q are complete. P11O remains the accepted production baseline; P11Q proved and target-projected the FDN sine-endpoint cache and authorizes P11R production validation.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
old_next = '''## Exact next phase — P11Q FDN sine endpoint-cache equivalence proof

Before any production source modification:
1. prove exhaustive top-16 helper equivalence and every top-byte transition including `255 → 0`;
2. prove reset initialization and complete-FDN output/state/storage equivalence across cold, warm, randomized trajectories and resets;
3. run optimized + ASAN/UBSAN;
4. measure exact host/target state-size cost;
5. project the candidate ephemerally on the real JieLi target for image/RAM/POOL/package and inlined render code shape;
6. authorize a separate production-validation phase only if every gate passes and the target tradeoff remains justified.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
new_next = '''## P11Q — FDN sine endpoint-cache proof complete

Proof authority:
- technical head `a0b0826c9f0951b780d84404cfdbe9543ba45b49`
- run `41360291892` — PASS
- artifact ID `13104793891`
- artifact ZIP SHA-256 `7e7699b82e6540c64e099d4f8ea4d5a6d508e6a88d402a8c91c1d0aaa0cfcd05`
- repository production Space remained exact P11O
- historical workflow manual-only and pinned to the proven technical head

Exact equivalence coverage:
- top-16 helper domain **65,536** cases
- index-transition matrix **262,144** cases including `255 → 0`
- complete FDN **5,376 blocks / 164,767 frames**
- cold **256 blocks / 16,384 frames**, **88** hot-path refreshes after reset initialization
- warm **1,024 blocks / 65,536 frames**, **367** refreshes
- randomized continuation **4,096 blocks** with resets and randomized LFO phases
- stereo output, every pre-existing FDN state field and full delay storage bit-identical
- optimized + ASAN/UBSAN PASS
- full historical host + sanitizer behavior PASS with the ephemeral candidate

State/layout cost:
- host `ApsisSpaceFdnLine` **24 → 28 B**
- host `ApsisSpaceFdn` **232 → 264 B**
- target ordinary RAM delta **+32 B**, confirming exactly 4 B per line

Ephemeral real-target projection:
- baseline P11O: image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**, package `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- candidate: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**
- candidate RAM headroom **10,780 B**; POOL headroom **24,888 B**
- package bytes **610,110 B**
- candidate package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- inlined FDN render **1,114 → 1,098 B**, **374 → 366** static instruction lines
- `space_sine_point` stays **48 B / 14** static instruction lines
- dead `space_sine_q15` wrapper is removed by the candidate

Combined with P11P, the candidate replaces repeated endpoint evaluation on more than 99.9% of the measured natural sine calls for a bounded 32-B RAM/image cost and a smaller render code shape. P11Q therefore authorizes P11R production validation. It does not claim measured physical CPU or ISR improvement.

## Exact next phase — P11R production FDN sine endpoint-cache validation

Apply exactly the P11Q candidate to production `apsis_space_dsp.c/.h` and nothing else in Space. Require actual-production equivalence against frozen P11O authority, complete historical host + sanitizer behavior, exact real-target match to the P11Q projection, exact idle PCM/runtime and exact active Motion lifecycle before promotion.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE P11Q anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11Q roadmap/project state updated')
