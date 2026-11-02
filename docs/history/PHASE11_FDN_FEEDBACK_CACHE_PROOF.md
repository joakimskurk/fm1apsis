# Phase 11K — FDN feedback-gain cache proof

Status: **complete proof/projection phase. Production FDN source and state layout remain exact P11I/P11J authority.**

P11K proves a narrowly keyed cache candidate before production modification. It does not itself change the shipping/source FDN.

## Equivalence authority

- proof branch `p11k-fdn-feedback-cache-proof-staging`
- proof head `ffb436dd87247af23c37e7e533cfd87fc2d161d0`
- authoritative proof run `41350818792`
- optimized build PASS
- ASAN/UBSAN PASS
- production `apsis_space_dsp.c` remained blob `72210e122b68bff77c56805618b2703d363a25ef`
- production `apsis_space_dsp.h` remained blob `ae843d3289a8ff839cfe061861311a5620ab839a`

The proof runs two complete independent FDNs with separate delay storage after every render block:
- baseline: current always-recomputed production `apsis_space_fdn_render()`;
- candidate: identical render with an external eight-value feedback-gain cache keyed only by rounded `decay_ms`.

After every block it requires exact stereo-output equality, exact existing FDN state equality and byte-identical full FDN delay storage.

Covered cases:
- reset and partial-fill behavior;
- 256-block steady state;
- exact room-style 4 s → 12 s smoothed decay transition;
- adversarial 4 s / 12 s alternating every block;
- every integer legal decay key 4000..12000, with a deliberate cache hit at each key and varied damping;
- 4,096 deterministic random blocks, 92,927 frames total, varied frame counts/decay/damping, deliberate cache reuse and reset every 257 blocks.

Measured proof behavior:
- host external cache representation: **20 B**;
- steady: 256 blocks / 16,384 frames → **1** recompute block / **8** gain evaluations;
- sweep: **192** rounded-decay changes → **193** recompute blocks / **1,544** gain evaluations;
- adversarial: **256** recompute blocks / **2,048** gain evaluations;
- all legal decay keys: 16,002 blocks → **8,001** recompute blocks / **64,008** gain evaluations;
- random/reset: 4,096 blocks / 92,927 frames → **3,840** recompute blocks / **30,720** gain evaluations.

The candidate preserves `space_feedback_gain()` arithmetic exactly and changes only when those exact values are recomputed.

## Ephemeral real-target projection

The candidate was then patched **only** into the generated target worktree after exact P11I regeneration. Repository production files remained unchanged.

Authority:
- projection head `72b48c499ce7619fd3a077edac664ea2c65c4ca5`
- authoritative run `41351123630`
- evidence artifact `p11k-target-projection-evidence`
- artifact ID `13101765292`
- artifact ZIP SHA-256 `a5cf89df0e49c8d79c5118db8f444f74a65808e6ca0f820e43bfda4d40ff0ef3`

Exact baseline regeneration:
- image **140,392 B**
- RAM **87,476 B**
- POOL **319,176 B**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`

Ephemeral cache candidate:
- image **140,440 B** — delta **+48 B**
- RAM **87,492 B** — delta **+16 B**
- RAM headroom **10,812 B**
- POOL **319,176 B** — delta **0**
- package bytes **610,110 B** — unchanged
- projection package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`

Target FDN render code shape:
- baseline `apsis_space_fdn_render`: **1,082 B / 353 static instruction lines**
- candidate: **1,118 B / 379 static instruction lines**
- delta: **+36 B / +26 static instruction lines**

This is therefore not a static-size optimization. It intentionally spends 16 B ordinary RAM and a small amount of code to avoid expensive block-rate coefficient recomputation when rounded decay is unchanged.

## Workload tradeoff inherited from P11J

For the 256-block census, current code always performs 2,048 `space_feedback_gain()` evaluations, each containing one 32-bit division and three wide fixed-point multiplies.

The proved cache model changes that to:
- steady: 8 evaluations → **2,040 fewer divisions + 6,120 fewer wide multiplies**;
- one legal 4 s → 12 s sweep: 1,544 evaluations → **504 fewer divisions + 1,512 fewer wide multiplies**;
- adversarial decay change every block: 2,048 evaluations → **zero savings**, plus the bounded cache-key check.

## Decision

P11K **authorizes P11L production validation** because:
1. cached and always-recomputed FDN behavior are bit-identical under the proof matrix;
2. the target cost is small and explicitly measured: +48 B image, +16 B RAM, 0 POOL;
3. steady and normal-transition coefficient workload reductions are substantial;
4. worst-case churn is explicit and bounded at zero coefficient-computation savings;
5. no production source was modified during proof/projection.

P11L must be a separate production patch and must pass exact host equivalence, full historical host/sanitizer behavior, real JieLi target, exact idle PCM/runtime and exact active Motion lifecycle before promotion.

P11K still makes **no physical FM-1 CPU-cycle or ISR-margin claim**. That remains hardware-only validation.
