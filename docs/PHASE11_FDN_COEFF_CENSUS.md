# Phase 11J — FDN coefficient recomputation census

Status: **complete measurement phase. Production Space sources remain byte-identical to P11I.**

## Why this phase

P11H measured 256 FDN render calls over the 65,536-output-frame census and 655,360 FDN Q15 multiplies. Inspection after P11I identified another narrower block-rate cost: `apsis_space_fdn_render()` recomputes all eight `space_feedback_gain()` values on every render call, even when the rounded `decay_ms` argument is unchanged.

Each feedback-gain evaluation contains one 32-bit division and three 64-bit fixed-point multiplies before the normal per-frame FDN work begins.

## Authority

- branch `p11j-fdn-coeff-census-staging`
- technical head `17030232e5f21f95a81b834e831d9021683d81bf`
- authoritative run `41350487323`
- optimized census PASS
- ASAN/UBSAN census PASS
- production `apsis_space_dsp.c` frozen at blob `72210e122b68bff77c56805618b2703d363a25ef`
- production `apsis_space_room.c` frozen at P11I blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`

Instrumentation is generated into `/tmp`; no P11J probe enters production firmware.

## Measured scenarios

All scenarios execute 256 room/FDN blocks of 64 quarter-rate room frames. Current code performs 8 feedback-gain evaluations per block, therefore **2,048 gain evaluations**, **2,048 32-bit divisions** and **6,144 wide fixed-point multiplies** in every scenario.

### Steady room

- FDN renders: **256**
- gain evaluations: **2,048**
- rounded decay changes: **0**; unique rounded decay values: **1**
- rounded damping changes: **0**; unique rounded damping values: **1**
- gain evaluations required by a recompute-only-on-decay-change model: **8**
- redundant gain evaluations: **2,040 / 2,048 = 99.6%**

### One legal 4 s → 12 s transition

The state changes once after callback 64 and then follows the existing room smoothing law.

- FDN renders: **256**
- gain evaluations: **2,048**
- rounded decay changes: **192**; unique rounded decay values: **193**
- rounded damping changes: **192**; unique rounded damping values: **193**
- recompute-on-decay-change gain evaluations: **1,544**
- redundant gain evaluations: **504 / 2,048 = 24.6%**

### Adversarial extreme churn

The requested decay/damping alternates between legal extremes every callback.

- FDN renders: **256**
- gain evaluations: **2,048**
- rounded decay changes: **255**; unique rounded decay values: **247**
- rounded damping changes: **255**; unique rounded damping values: **233**
- recompute-on-decay-change gain evaluations: **2,048**
- redundant gain evaluations: **0**

This adversarial case is important: a cache cannot be described as reducing every callback. Its worst-case coefficient-computation count is the current count plus a cheap equality/validity decision.

## Decision

P11J **authorizes a P11K proof candidate**, not a production patch:

- cache the eight feedback gains together with the rounded `decay_ms` that produced them;
- recompute all eight gains exactly when the rounded decay value changes or the cache is invalid;
- do not cache or approximate anything else;
- preserve the existing `space_feedback_gain()` arithmetic exactly.

The candidate is attractive because steady state eliminates 2,040 repeated divisions and 6,120 repeated wide multiplies over this census, while a normal one-way sweep still eliminates 504 divisions and 1,512 wide multiplies. The adversarial case demonstrates the explicit zero-savings bound.

## P11K acceptance boundary

Before production modification, P11K must prove cached and always-recomputed FDN output/state bit-identical across reset, partial-fill, steady, smoothed transitions and adversarial decay sequences. A later production patch must then pass the full real-target, idle PCM/runtime and active Motion lifecycle gates and must measure the exact RAM/image cost of the cache.

No physical FM-1 CPU/ISR improvement is claimed by P11J.
