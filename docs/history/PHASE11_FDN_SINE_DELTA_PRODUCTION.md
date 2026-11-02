# Phase 11W — production FDN sine A+delta cache validation

Status: **complete, exact-head validated and frozen.**

P11W applies only the P11V-proved representation change from cached sine endpoints `A+B` to `A+delta`, where `delta = B-A`. The state width stays unchanged: two signed 16-bit values per FDN line. No FDN topology, LFO phase/increment, interpolation, feedback, room, scheduler or control behavior changes.

## Production authority

- technical production head `ac39372d8181c7fb27c9c25b55e825a5db53e10c`
- authoritative validation run `41393330606`
- all six validation gates PASS
- production `apsis_space_dsp.c` blob `3bcd294dd9fb610df888ddd0517da7de3bf4c086`
- production `apsis_space_dsp.h` blob `ec221e22aeb6670ec628cbb0cc7c7eea91038554`
- Cathedral room source remains blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`
- evidence artifact `p11w-fdn-sine-delta-production-evidence`
- artifact ID `13118836564`
- artifact ZIP SHA-256 `c866cf8b844b80e31d070c934f9c26dc7378f21fe7acbae36f9c1ac550bb1c05`

The committed production source was regenerated independently from the frozen P11R authority plus `ci/p11v_apply_sine_delta_candidate.py`, then compared byte-for-byte with the committed P11W source. It matched exactly.

## State and behavioral invariants

- `sizeof(ApsisSpaceFdnLine)` remains **28 B**
- `sizeof(ApsisSpaceFdn)` remains **264 B**
- cached `B` is replaced by exact signed-16 `delta=B-A`; no state growth
- reset/refresh stores exact `A` and `delta`
- interpolation remains the proved exact `A + ((delta * frac) >> 8)` form
- all historical host and ASAN/UBSAN behavior PASS

## Frozen production target

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**
- RAM headroom **10,780 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined `apsis_space_fdn_render` **1,094 B / 364** static instruction lines

The real JieLi production build matches the P11V ephemeral target projection exactly.

## Exact idle regression

- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor **123,392**
- queue high-water **28/512**
- queue overflow **0**
- late flags **0**
- late trace `811c9dc5`
- peak **856**
- RMS **144/144**
- stereo delta **126**

## Exact active Motion regression

- image **137,736 B**
- RAM **87,572 B**
- RAM headroom **10,732 B**
- POOL **319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- seen flags `ff`; final flags `14`
- left/right points **149/149**
- record start **220,672**, finish **441,088**, cycle **220,416** samples
- manual override **450,560**, resume **466,176**
- runtime cursor **481,024**
- queue high-water **30/512**
- queue overflow **0**
- late flags **0**
- WAV peak **1,171**; RMS **173**

## Decision

P11W replaces P11R as the accepted production baseline. It preserves image/RAM/POOL size relative to P11R while reducing the target FDN render from **1,098 B / 366** static instruction lines to **1,094 B / 364** through the zero-growth A+delta representation.

This is still a static/workload-proxy optimization. Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention, NOR timing and DAC/listening quality remain unverified until Phase 12 hardware work.
