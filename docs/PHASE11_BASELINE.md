# Phase 11 — optimisation method and P11A baseline

Phase 11 is **measurement-first optimisation**. It must not change APSIS musical behaviour merely to reduce a number.

## Starting authority

Phase 10 is merged as `378d2023df2afe59475be53c09cc001788586514` after a 22/22 exact-head matrix on documentation head `ce5857a52c6326011e5af7cd7f73819b420550fc`.

The starting stacked production target is:
- product `FM-1_972`
- image 140,152 / 581,564 B
- RAM 89,684 / 98,304 B
- POOL 328,740 / 344,064 B
- package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- frozen inactive PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- frozen inactive cursor 123,392; queue 28/512; overflow 0; late flags 0

## P11A purpose

P11A does not optimise anything. It creates an automatically reproducible inventory of what is actually measurable before optimisation begins.

The workflow must prove:
1. the final Phase-10 production stack regenerates exactly at the frozen resource sizes;
2. `sizeof(ApsisRuntime)` remains 21,928 B;
3. `sizeof(ApsisStringMotionStore)` remains 19,208 B;
4. Phase-9 → Phase-10 POOL growth is exactly 19,208 B, so MotionStore accounts for the whole increase and the Attractor/Gravity editors add no POOL allocation;
5. the exact frozen inactive PCM/runtime emulator baseline remains unchanged.

The generated evidence separates measured facts from quantities that are not yet honestly measurable.

## Measurable now

- target image usage/headroom from the JieLi/Dinghy build
- target `.data + .bss` RAM usage/headroom from the build
- target POOL usage/headroom
- package size/hash/identity
- host C layout sizes for frozen runtime and Motion structures
- deterministic inactive-emulator PCM/runtime/queue telemetry
- later: deterministic stress counters and target/emulator timing proxies, provided they are labelled as proxies rather than physical ISR timing

## Not yet physically measured

Do not infer or claim these from emulator wall time:
- physical FM-1 `app_audio()` ISR/callback cycle margin
- true worst-case stack high-water on device
- LCD DMA contention/timing on device
- real NOR erase/program timing and power-interruption behavior
- DAC/listening quality
- Cathedral perceptual tuning
- worst-case combined control/UI/audio timing on physical hardware

If a non-hardware proxy is added for one of these areas, its report and documentation must name it as a proxy and explain what it does **not** prove.

## Optimisation acceptance rule

Every optimisation candidate must have:
1. a measured baseline;
2. one explicit resource or timing target;
3. a minimal patch;
4. before/after evidence on the same measurement path;
5. all relevant frozen musical/audio/control regressions green;
6. no baseline relaxation just to make the patch pass.

Prefer removing demonstrably redundant work or storage over changing source-derived algorithms. Do not add private schedulers, duplicate state, UI shadow musical state or new heap ownership in the name of optimisation.
