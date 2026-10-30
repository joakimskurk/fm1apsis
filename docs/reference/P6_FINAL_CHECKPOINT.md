# Phase 6 Final Checkpoint — Shared Space / Cathedral

Date: 2026-10-30

This file records the final technical Phase-6 state before the guarded PR #16 merge.

## Source/topology

Study 126 remains authority. There is one shared room fed by post-source/post-Mother sends:

| Source | Send |
|---|---:|
| String | 0.72 |
| Drift | 0.20 |
| Clock | 0.06 |
| Spiral | 0.62 |
| Bloom | 0.58 |

Space defaults: wet 0.48, decay 8.4 s, predelay 72 ms, damping 6200 Hz, echo 362 ms / 18% feedback.

## Selected algorithm

Compact deterministic 8-line modulated FDN with normalized Hadamard feedback, one-pole damping and decorrelated stereo return. The tested nested allpass/Schroeder candidate was rejected for substantially higher metallicity.

Host/reference room remains 44.1 kHz and retains the frozen deterministic reference tests.

## FM-1 target adaptation

The source engines, scheduler and final output stay at 44.1 kHz.

The generated FM-1 Cathedral executes internally at 11.025 kHz:
- 4:1 stereo room-input averaging
- one wet sample held over four 44.1-kHz output frames
- target-only retiming of early taps, predelay, echo delay, FDN line lengths, decay law, damping/filter coefficients, smoothing and modulation increments

This is a target execution adaptation, not a change to the musical/source timing model.

## Memory layout

Large history/echo/FDN arrays live in `.apsis_space_pool`, a dedicated NOLOAD section immediately before Dinghy's ordinary `.pool` in the same hardware POOL region.

This avoids Dinghy's startup clear of `[_pool_start,_pool_end)` while keeping linker/resource accounting honest. Space tracks fill/validity and never reads stale unwritten NOLOAD memory.

## Final technical target evidence

Technical verification head before documentation freeze:
`0ec9915f9d994454aa079a5be107765844534666`

CI:
- core #416 PASS
- staged #405 PASS all gates
- P6-final #14 PASS all gates

Real target:
- product `FM-1_972`
- image 107,352 / 581,564 B
- RAM 87,780 / 98,304 B
- POOL 309,532 / 344,064 B
- RAM headroom 10,524 B
- POOL headroom 34,532 B

Audible emulator:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS L/R 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

P6 dry-only variant:
- PCM exactly equals Phase 5: `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

Historical M4g/M4f staged baselines retain cursor 123,648. Phase 6's 123,392 is an explicitly frozen scenario baseline because both audible and dry-only P6 variants agree while scheduler telemetry remains healthy.

## CI reporting correction

The staged workflow appends several target builds to one log. `inspect_target_build.py` now consumes the final resource summary so bypass/probe reports describe the package just built. This corrected a reporting-only false failure where the M4f Bloom-bypass binary was correctly 95,804 B but inherited the previous M4g 100,944 B figure in JSON.

## Verification boundary

Verified through target build + pinned emulator. Physical FM-1 remains unavailable, so real ISR margin, physical UI behaviour and listening quality remain open Phase-11/12 work.

## Closure

Run core + staged + P6-final on the documentation head. If all pass, guarded-merge PR #16 and move continuity to Phase 7 persistent KEEP/CHAIN storage.