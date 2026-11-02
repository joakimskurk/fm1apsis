#!/usr/bin/env python3
from pathlib import Path

RUN='41391510585'
HEAD='e7b91e768eb44a07a2149cb4588da65cab653bfa'
ARTIFACT='13116934949'
ARTIFACT_SHA='3a884a48d8fc14ad6147b09c9f902819c90a085d4dc89e05f9cfedb6a87bf93f'
CAND_SHA='f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168'

phase_doc = Path('docs/PHASE11_FDN_SINE_DELTA_CACHE_PROOF.md')
phase_doc.write_text(f'''# Phase 11V — FDN sine A+delta cache proof and target projection

Status: **complete proof/projection phase. Committed production remains exact P11R.**

P11V tests one zero-state-growth representation change selected by P11U: retain cached sine endpoint `A`, replace cached endpoint `B` with signed delta `B-A`, and preserve interpolation as `A + ((delta * frac) >> 8)`.

## Authority

- technical head `{HEAD}`
- authoritative run `{RUN}` — PASS
- evidence artifact ID `{ARTIFACT}`
- artifact ZIP SHA-256 `{ARTIFACT_SHA}`
- committed production `apsis_space_dsp.c` remained P11R blob `e970aeaff9264c37188c7d3fbcad2960bad03a69`
- committed production `apsis_space_dsp.h` remained P11R blob `e5d93117d6bab1552532c6a7f6bd9af6758cab9f`
- Cathedral room remained blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`

## Exact equivalence proof

Optimized and ASAN/UBSAN runs both PASS.

Coverage:
- **65,536** complete top-16 interpolation cases
- **65,536** old→new index transitions; **65,280** refreshes / **256** reuses, including `255 → 0`
- cold: **256 blocks / 16,384 frames**
- warm: **1,024 blocks / 65,536 frames**
- legal sweep: **423 blocks / 13,536 frames**
- deterministic random: **4,096 blocks / 211,779 frames**
- adversarial: **512 blocks / 32,768 frames**
- complete FDN total: **6,311 blocks / 340,003 frames**

For the full-FDN trajectories, stereo output, all pre-existing FDN state, feedback-cache state and full delay storage remain bit-identical to frozen P11R authority after every block.

State shape is unchanged:
- `sizeof(ApsisSpaceFdnLine)` = **28 B**
- `sizeof(ApsisSpaceFdn)` = **264 B**

## Historical behavior

The ephemeral candidate passes the complete historical host + sanitizer preparation after the P11F runtime-size adaptation. Existing core, scheduler, String, Clock, Gravity/Bloom and Space hashes/invariants remain green.

## Exact P11R baseline regeneration

Before applying the candidate, the workflow regenerated the accepted P11R target exactly:
- product `FM-1_972`
- image **140,472 B**
- RAM **87,524 B**; headroom **10,780 B**
- POOL **319,176 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- `apsis_space_fdn_render` **1,098 B / 366** static instruction lines

## Ephemeral JieLi target projection

The A+delta candidate was applied only to the working tree and built through the real JieLi target path:
- image **140,472 B** — delta **0 B**
- RAM **87,524 B** — delta **0 B**
- POOL **319,176 B** — delta **0 B**
- package bytes **610,110 B**
- projected package SHA-256 `{CAND_SHA}`
- `apsis_space_fdn_render` **1,094 B / 364** static instruction lines
- render delta **−4 B / −2 static instruction lines**
- `space_sine_point` remains **48 B / 14** static instruction lines

The candidate therefore improves the measured static target render shape slightly without any image/RAM/POOL growth.

## Decision

P11V **authorizes P11W production validation**.

P11W may change only the proved representation:
1. rename the second signed-16 cache field from cached `B` to cached `delta`;
2. reset initializes `A` and exact `B-A`;
3. endpoint refresh computes the exact new `A` and `B-A`;
4. interpolation becomes `A + ((delta * frac) >> 8)` with the same signed arithmetic;
5. no other FDN/room/state/topology change is authorized.

P11W must require actual-production equivalence, complete historical host/sanitizer behavior, exact real-target values, exact idle PCM/runtime and exact active Motion lifecycle before promotion.

P11V makes **no physical FM-1 CPU-cycle or ISR-margin claim**. Physical timing remains Phase 12 work.
''')

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old = '''### P11V — cached sine A+delta equivalence proof

Status: **[ ] next. Proof only; production remains P11R.**

- [ ] prove `A + (((B-A)*frac)>>8)` equals `A + ((delta*frac)>>8)` for all **65,536** top-16 phase combinations with exact signed semantics
- [ ] cover all 256 old→new index transitions, including wrap, reset initialization and randomized phase resets
- [ ] retain exactly two signed-16 cache fields per FDN line and prove host/target state size unchanged
- [ ] compare complete stereo output, state, feedback cache and delay storage across cold/warm/legal/random/reset/adversarial trajectories
- [ ] optimized + ASAN/UBSAN and complete historical host behavior
- [ ] regenerate exact P11R target before applying candidate only ephemerally
- [ ] measure image/RAM/POOL/package and inlined render code-shape delta; reject if target shape is neutral/worse without stronger evidence
'''
new = f'''### P11V — cached sine A+delta equivalence proof

Status: **[x] complete, exact proof + real-target projection frozen. Production remains P11R.**

- [x] **65,536** top-16 interpolation cases bit-identical
- [x] **65,536** index-transition cases; 65,280 refreshes / 256 reuses including `255 → 0`
- [x] complete FDN **6,311 blocks / 340,003 frames** across cold/warm/legal/random/adversarial trajectories
- [x] stereo output, existing FDN state, feedback-cache state and full delay storage bit-identical after every block
- [x] optimized + ASAN/UBSAN PASS
- [x] complete historical host + sanitizer behavior PASS with ephemeral candidate
- [x] state unchanged: `ApsisSpaceFdnLine` **28 B**, `ApsisSpaceFdn` **264 B**
- [x] exact P11R target regenerated first: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** lines
- [x] ephemeral candidate target: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B** — **0 B resource delta**
- [x] candidate render **1,094 B / 364** static instruction lines = **−4 B / −2 lines**
- [x] candidate projection package SHA-256 `{CAND_SHA}`
- [x] authoritative run `{RUN}` on technical head `{HEAD}`
- [x] evidence artifact ID `{ARTIFACT}`; ZIP SHA-256 `{ARTIFACT_SHA}`
- [x] committed production Space remained exact P11R throughout

Decision: **authorize P11W production validation.** Apply only the proved A+B → A+delta representation change; require actual-production equivalence, full historical behavior, exact JieLi target, idle PCM/runtime and active Motion before promotion.

### P11W — production FDN sine A+delta cache validation

Status: **[ ] next.**

- [ ] replace only cached `B` with exact signed `delta=B-A`; preserve two-int16 state width
- [ ] reset/refresh store exact A and delta; interpolation remains `A + ((delta*frac)>>8)`
- [ ] actual-production equivalence against frozen P11R A+B authority, optimized + ASAN/UBSAN
- [ ] complete historical host + sanitizer behavior
- [ ] exact real JieLi target must match P11V projection: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final package hashes before promotion
'''
if old not in s:
    raise SystemExit('ROADMAP P11V anchor mismatch')
s = s.replace(old, new, 1)
old_next = '''## Exact next action

Promote frozen P11U measurement state to `p11-optimization`, then create one P11V proof branch. Prove the zero-growth sine-cache representation change from A+B to A+delta, and perform an ephemeral real-target projection; production remains P11R unless every equivalence/historical/target gate justifies a later validation phase.'''
new_next = '''## Exact next action

Promote frozen P11V proof/projection state to `p11-optimization`, then create one P11W production-validation branch. Apply only the proved zero-growth A+B → A+delta cache representation and require complete actual-production equivalence, historical, real-target, idle and active-Motion gates before promotion.'''
if old_next not in s:
    raise SystemExit('ROADMAP exact-next anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11U are complete. P11R remains the accepted production baseline. P11U identified a zero-state-growth A+delta sine-cache candidate from measured residual work and authorizes P11V proof-only.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11V are complete. P11R remains the accepted production baseline. P11V proved and real-target-projected the zero-growth A+delta sine cache and authorizes P11W production validation.**'
if old_m not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
marker = '\n## Do not regress\n'
block = f'''
## P11V — FDN sine A+delta cache proof complete

Authority:
- technical head `{HEAD}`
- run `{RUN}` — PASS
- artifact ID `{ARTIFACT}`
- artifact ZIP SHA-256 `{ARTIFACT_SHA}`
- committed production Space remained exact P11R throughout

Proof coverage:
- **65,536** complete top-16 interpolation cases
- **65,536** index transitions, 65,280 refreshes / 256 reuses including wrap
- cold 256 blocks / 16,384 frames
- warm 1,024 blocks / 65,536 frames
- legal sweep 423 blocks / 13,536 frames
- random 4,096 blocks / 211,779 frames
- adversarial 512 blocks / 32,768 frames
- total **6,311 blocks / 340,003 frames**
- output, existing FDN state, feedback-cache state and full delay storage bit-identical
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS

State remains exactly **28 B/line / 264 B FDN**.

Real-target projection:
- exact P11R baseline first: **140,472 B image / 87,524 B RAM / 319,176 B POOL**, package `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- candidate: **140,472 / 87,524 / 319,176 B** — zero resource delta
- candidate package SHA-256 `{CAND_SHA}`
- FDN render **1,098 → 1,094 B**, **366 → 364** static instruction lines
- `space_sine_point` remains **48 B / 14** lines

Decision: P11W may apply only the proved A+B → A+delta representation to production. No other FDN, room or state change is authorized. P11V proves equivalence/static target properties, not physical FM-1 cycle or ISR improvement.

## Exact next phase — P11W production sine A+delta validation

Apply the exact P11V candidate to production, then require actual-production equivalence, complete historical host/sanitizer behavior, exact JieLi target matching the P11V projection, exact idle PCM/runtime and exact active Motion lifecycle before promotion. Until that matrix is green, P11R remains the accepted production baseline.
'''
if marker not in s:
    raise SystemExit('PROJECT_STATE do-not-regress anchor missing')
s = s.replace(marker, block + marker, 1)
state.write_text(s)
print('PASS: P11V phase doc, roadmap and project state finalized')
