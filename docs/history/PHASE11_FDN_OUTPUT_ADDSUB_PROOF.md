# Phase 11Y — fixed FDN output add/sub proof

Status: **complete proof/projection phase; candidate rejected for production. P11W remains the accepted production baseline.**

P11Y tested the P11X-selected idea of replacing the two fixed ±1 output dot products with explicit add/sub accumulation. The candidate is mathematically and behaviorally exact, but the real JieLi target grows enough that it is rejected without physical timing evidence.

## Authority

- technical head `ca46ac5a5d3db9b9f0e43adb061ba780fab39c9a`
- authoritative run `41397297203` — all gates PASS
- evidence artifact `p11y-fdn-output-addsub-proof-evidence`
- artifact ID `13119718531`
- artifact ZIP SHA-256 `dde978fd3a521dac64c3c5cb1a66e417b39a3a70508be2569481e765c228f1ec`
- historical workflow manual-only and pinned to the proven head
- committed Space source/header remain exact P11W throughout

## Arithmetic equivalence

Baseline:
- two eight-term dot products using frozen ±1 sign vectors.

Candidate:
- left: `+m0 +m1 -m2 +m3 -m4 +m5 +m6 -m7`
- right: `+m0 -m1 +m2 +m3 +m4 -m5 +m6 -m7`

Proof coverage:
- all **256** ±32767 extreme sign combinations;
- one-hot boundary sweep over `{-32767,-32766,-1,0,1,32766,32767}` for every field;
- **1,000,000** deterministic random legal-Q15 vectors;
- total **1,000,312 arithmetic cases**;
- exact signed-32 output equality in every case.

## Complete FDN equivalence

Two independent FDNs with separate delay storage were compared after every block:
- baseline: exact P11W sign-multiply render;
- candidate: identical render except fixed output add/sub.

Trajectories include cold and warm steady state, legal parameter sweep, adversarial extremes, 4,096 random blocks and reset every 257 blocks.

Measured coverage:
- **6,311 FDN blocks**
- **386,191 FDN frames**
- exact stereo output equality
- exact FDN state equality
- byte-identical full FDN delay storage after every block
- optimized PASS
- ASAN/UBSAN PASS

## Exact P11W baseline regeneration

Before applying the candidate, the real JieLi target regenerated P11W exactly:
- image **140,472 B**
- RAM **87,524 B**
- POOL **319,176 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- `apsis_space_fdn_render` **1,094 B / 364** static instruction lines

## Ephemeral candidate projection

The add/sub transform was applied only to the generated target worktree. Repository production source remained unchanged.

Candidate target:
- image **140,520 B** — **+48 B**
- RAM **87,524 B** — unchanged
- POOL **319,176 B** — unchanged
- package bytes **610,110 B** — unchanged
- candidate package SHA-256 `01d9f3fbc225973b6f2dcb861a86c2ee62af49594a288ba43c7cd2fac4ba61d0`
- `apsis_space_fdn_render` **1,154 B / 385** static instruction lines
- render delta **+60 B / +21 static instruction lines**

The sign tables are not separately represented as retained target symbols in either build, so removing their C declarations does not buy a measurable target-table reduction.

## Decision

**Reject fixed output add/sub for production.**

Although P11X demonstrated 16 explicit sign multiplications per FDN frame and P11Y proves add/sub exact, the actual JieLi compiler produces a materially larger render and a +48 B image regression. Without physical FM-1 cycle/ISR measurements showing that the larger form is faster enough to justify that cost, promotion would violate Phase 11's evidence-first acceptance rule.

P11W therefore remains the accepted production baseline:
- image **140,472 B**
- RAM **87,524 B**
- POOL **319,176 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- render **1,094 B / 364** static instruction lines

P11Y is intentionally preserved as a negative result so the same plausible-looking transformation is not rediscovered and retried later without new hardware timing evidence.
