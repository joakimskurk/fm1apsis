# Phase 11V — FDN sine A+delta cache proof and target projection

Status: **complete proof/projection phase. Committed production remains exact P11R.**

P11V tests one zero-state-growth representation change selected by P11U: retain cached sine endpoint `A`, replace cached endpoint `B` with signed delta `B-A`, and preserve interpolation as `A + ((delta * frac) >> 8)`.

## Authority

- technical head `e7b91e768eb44a07a2149cb4588da65cab653bfa`
- authoritative run `41391510585` — PASS
- evidence artifact ID `13116934949`
- artifact ZIP SHA-256 `3a884a48d8fc14ad6147b09c9f902819c90a085d4dc89e05f9cfedb6a87bf93f`
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
- projected package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
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
