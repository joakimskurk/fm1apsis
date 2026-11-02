# Phase 11N — FDN line-read invariant simplification proof

Status: **complete proof/projection phase. Production Space source remains exact P11L authority.**

P11N proves a narrowly specialized `space_line_read()` candidate before production modification. It preserves startup/read/interpolation behavior while exploiting only the production-caller invariants established by P11M.

## Candidate

Compared with P11L authority, the candidate:
- retains the defensive `fdn/line/storage_bound` helper guard;
- retains the exact startup `needed = (delay_q8 + 255) >> 8` calculation while a line is not full;
- bypasses that readiness calculation once `fill_count == capacity`, where P11M proved failure impossible;
- removes the structurally unreachable minimum-delay clamp;
- removes the structurally unreachable upper ring-wrap correction;
- leaves the negative-wrap correction, fractional interpolation and `i1` ring-end wrap unchanged.

No LFO, modulation-depth, line-delay, line-capacity, damping, feedback-cache, Hadamard, input/output or storage semantics are changed.

## Equivalence authority

- branch `p11n-fdn-line-read-proof-staging`
- combined proof/projection head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- authoritative proof re-run `41355949491` — PASS
- earlier clean proof run `41355845072` — PASS
- optimized proof PASS
- ASAN/UBSAN proof PASS
- production `apsis_space_dsp.c` remained P11L blob `0c472f3844210f0dfb3d31bf6dacf007272ac078`
- production `apsis_space_dsp.h` remained P11L blob `71afae3c4c52bbd735121597dfa7ad6cea4f32f5`
- production `apsis_space_room.c` remained blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`

The proof workflow is frozen manual-only and pinned to the combined technical head.

## Helper-level proof

The current production helper is compiled directly from `apsis_space_dsp.c` as the authority. The candidate exists only inside the test translation unit.

Coverage includes:
- all eight frozen FDN line topologies;
- every integer legal production modulation value **−2,304 .. +2,303 Q8**;
- readiness boundaries around zero, one, `needed-1`, `needed` and `capacity-1`;
- full-line write positions around both ring ends and the raw-read zero crossing;
- deterministic storage contents that expose index/interpolation mismatches;
- **500,000** deterministic randomized valid partial/full line states.

Total helper comparisons: **905,504**.

Every authority/candidate return value is bit-identical.

## Complete-FDN proof

Two independent complete FDNs with separate delay storage are then rendered block-for-block:
- authority: current P11L production `apsis_space_fdn_render()`;
- candidate: the same render with only the proved candidate line reader substituted.

After every block the proof requires:
- bit-identical stereo output;
- equality of every FDN line/state field;
- equality of feedback-cache state;
- byte-identical complete FDN delay storage.

Trajectories:
- cold startup: **16,384 frames**;
- warm continuation: **65,536 frames**;
- **2,048** deterministic random parameter/frame blocks;
- reset every 257 random blocks;
- deterministic randomized LFO phases after reset.

Total complete-FDN proof: **3,328 blocks / 210,677 frames**.

Optimized and ASAN/UBSAN runs are both exact.

## Ephemeral real-target projection

The candidate was patched only into the generated JieLi target worktree after exact P11L regeneration. Repository production files remained unchanged.

Authority:
- projection head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- authoritative target-projection run `41355949486`
- evidence artifact `p11n-line-read-target-projection`
- artifact ID `13103071866`
- artifact ZIP SHA-256 `6d39137a817dd8a2d842790c40b4f51f6d0c0ef12ce7b33c56733f6aaa84711b`
- artifact size **879,221 B**

Exact P11L baseline regenerated first:
- image **140,440 B**
- RAM **87,492 B**
- POOL **319,176 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`

Ephemeral P11N candidate:
- image **140,440 B** — delta **0 B**
- RAM **87,492 B** — delta **0 B**
- RAM headroom **10,812 B** — unchanged
- POOL **319,176 B** — delta **0 B**
- package bytes **610,110 B** — unchanged
- projection package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`

`space_line_read()` is inlined by the JieLi build, so the effect is measured in `apsis_space_fdn_render`:
- P11L baseline: **1,118 B / 379** static instruction lines;
- P11N candidate: **1,114 B / 374** static instruction lines;
- delta: **−4 B / −5 static instruction lines**.

The overall image remains the same size because the four-byte local reduction does not cross the final image-size/alignment boundary.

## Why a production-validation phase is justified

The static code-shape reduction is intentionally modest. The repeated-work argument comes from P11M:
- warm FDN operation is **100% full-line** across 524,288 measured reads;
- even the cold 16,384-frame census is already **85.37% full-line**;
- full-line reads can skip the `needed` add/round/shift/readiness calculation while preserving startup semantics;
- the removed minimum-delay and upper-wrap branches are algebraically unreachable under the frozen production caller.

The candidate therefore has:
1. a broad bit-identical helper and full-FDN proof;
2. a strictly smaller target FDN-render code shape;
3. zero RAM, POOL or total-image cost;
4. no change to musical/DSP topology;
5. a clear repeated-work reduction in the dominant warm line-read path.

## Decision

P11N **authorizes P11O production validation**, not direct production promotion.

P11O may modify only production `space_line_read()` to match the proved candidate. It must then pass:
- actual-production helper/full-FDN equivalence against frozen P11L authority;
- complete historical host + ASAN/UBSAN behavior;
- exact real JieLi target projection match;
- exact idle PCM/runtime;
- exact active Motion lifecycle;
- narrow production-diff verification.

No physical FM-1 CPU-cycle or ISR-margin improvement is claimed by P11N. The measured result is a workload/code-shape proxy only.
