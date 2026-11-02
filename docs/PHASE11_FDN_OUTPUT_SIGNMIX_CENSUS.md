# Phase 11X — FDN output sign-mix census

Status: **complete measurement phase. Production remains exact P11W.**

## Question

The FDN output projection accumulates two fixed eight-term dot products using `SPACE_OUT_LEFT[]` and `SPACE_OUT_RIGHT[]`. Every coefficient is exactly `+1` or `-1`, but the current source expresses the work as a multiply inside the eight-line loop.

P11X measures whether this is a meaningful remaining workload/code-shape candidate before any proof or production change.

## Authority

- technical head `a28fb0047550b729c15cc7e5bc3413a71a13819b`
- authoritative run `41396085585` — PASS
- evidence artifact `p11x-fdn-output-signmix-evidence`
- artifact ID `13119167571`
- artifact ZIP SHA-256 `5ab3d57b84c7fb941cabc46ae478144763d8cc6e2661c9919f8d996ffab51c66`
- optimized host census PASS
- ASAN/UBSAN census PASS
- exact P11W JieLi target regenerated
- committed Space source/header remain byte-identical to P11W

The historical workflow is manual-only and pinned to the proven technical head.

## Structural result

Both frozen output sign vectors contain only ±1:
- left: **5 × +1 / 3 × −1**
- right: **5 × +1 / 3 × −1**

For any legal Q15-range FDN line value, the absolute eight-term signed sum is bounded by:

`8 × 32767 = 262136`

This leaves **2,147,221,511** counts of signed-32 headroom. Replacing multiplication by ±1 with fixed add/sub therefore needs no wider accumulator than the current `int32_t` output sums.

## Dynamic source-level workload

Current source performs two sign-vector products for each of eight FDN lines:
- **16 sign products / frame**
- cold 16,384-frame census: **262,144** products
- warm 65,536-frame census: **1,048,576** products

The same counts passed under ASAN/UBSAN.

## Real JieLi target result

The exact P11W target regenerated first:
- image **140,472 B**
- RAM **87,524 B**
- POOL **319,176 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- `apsis_space_fdn_render` **364** static instruction lines

The initial generic opcode classifier is not semantically authoritative for JieLi's assignment-style disassembly: it parses all 364 lines but identifies destination/register tokens rather than true mnemonics. Its empty `multiply_like_opcodes` map must **not** be read as evidence that multiplications were optimized away.

The raw render disassembly is authoritative. In the eight-line output loop it shows:

- signed byte load from `SPACE_OUT_RIGHT[k]`
- signed byte load from `SPACE_OUT_LEFT[k]`
- `r1 *= r0`
- `r0 *= r2`

Therefore the compiler retains **two explicit general sign multiplications per FDN line**, exactly matching the source-level count of **16 per frame**.

## Decision

P11X **authorizes P11Y proof-only** for replacing only the two fixed ±1 dot products with explicit fixed add/sub accumulation using the frozen sign pattern:

- left = `+m0 +m1 -m2 +m3 -m4 +m5 +m6 -m7`
- right = `+m0 -m1 +m2 +m3 +m4 -m5 +m6 -m7`

P11Y must:
1. prove arithmetic equivalence across exhaustive/boundary/random mixed vectors;
2. prove complete FDN output/state/delay-storage equivalence across cold, warm, legal, random/reset and adversarial trajectories;
3. run normal + ASAN/UBSAN;
4. patch only an ephemeral generated target and compare exact real JieLi image/RAM/POOL/package and render code shape against P11W;
5. reject the candidate if target shape/resources regress without physical timing evidence.

No production source change is authorized by P11X, and no physical FM-1 cycle or ISR claim is made.
