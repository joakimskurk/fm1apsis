# Phase 11I — Cathedral stereo-pair delay read

P11I targets one narrow repeated Cathedral hot path identified by P11H: duplicated left/right delay-address calculation.

## Status

**Complete, exact-head validated and ready for promotion.**

P11I is accepted as a target code-shape/workload-proxy optimization only. It is **not** proof of physical FM-1 CPU-cycle or ISR-margin improvement; physical timing remains Phase 12 work.

## Pre-patch proof authority

- proof run `41343196262`
- proof head `32bb42c52f666af12065737e14a8846d3d8f01ab`
- **150,412** equivalence cases
- optimized host build PASS
- ASAN/UBSAN PASS
- complete historical host suite PASS
- production `apsis_space_room.c` remained at frozen pre-patch blob `2d7c34e32b48b904623d740a70cf657fee0801a7`

The candidate pair reader was proved bit-identical to the historical single-channel authority before production code was modified. The historical proof workflow is manual-only and pinned to the proven pre-patch authority.

## Authorized production change

Production commit `4629406bb6e417b0bcb544e6a770c0860613e7cd` replaces the single-channel room delay helper with one stereo-pair helper. The four early-reflection stereo taps, predelay and echo delay now perform the common validity/wrap/index/fraction calculation once per stereo location while retaining the exact historical per-channel interpolation arithmetic.

No delay length, coefficient, FDN topology, smoothing rule, state capacity or sample format changes.

The post-patch equivalence harness compares the actual production pair helper directly against an embedded copy of the frozen pre-P11I single-channel algorithm over the same **150,412** cases.

## Target-generator compatibility

The Phase-6 target generator historically carried a target-only 32-bit convex-interpolation fast path anchored to the old single-reader source text. P11I extends that adapter backward-compatibly:

- historical single-reader source continues to receive its original fast path;
- the proven stereo-pair source receives the corresponding safe 32-bit interpolation independently for left and right;
- host/reference room math is unchanged.

Adapter support is present by commit `5b760bcde63742fc504cce281e59f4b792b8841a` and was included in the exact validated technical head below.

## Exact production validation authority

- technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`
- authoritative validation run `41344186184`
- all five validation gates PASS
- evidence artifact `p11i-stereo-pair-evidence`
- artifact ID `13097594559`
- artifact ZIP SHA-256 `2a9b2d6f2adc8a5fd4a4f39dbe88cbd9b7c6bd2122b80ce83178a07a5a94e716`

Validated gates:
1. production pair helper remains bit-identical to frozen single-read authority, optimized and ASAN/UBSAN;
2. frozen room hashes and complete historical host/sanitizer behavior remain exact;
3. real JieLi target resource use does not regress and target code-shape proxy improves;
4. idle PCM/runtime remains exact;
5. active Motion lifecycle remains exact.

The production-apply and validation workflows are frozen manual-only. Their checkouts are pinned to technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`; the former is read-only and can no longer push source changes.

## Measured target result

P11F/P11H baseline room code shape:
- `apsis_space_room_render` + old single-read helper: **2,270 B** text
- **718** static disassembly instruction lines

P11I technical head:
- `apsis_space_room_render` + stereo-pair helper: **2,260 B** text
- **707** static disassembly instruction lines
- delta: **−10 B / −11 static instruction lines**

Frozen P11I production target:
- product `FM-1_972`
- image **140,392 / 581,564 B** — **−16 B** vs P11F
- RAM **87,476 / 98,304 B** — unchanged
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B** — unchanged
- POOL headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`

Frozen active-Motion target:
- image **137,656 B** — **−16 B** vs P11F active baseline
- RAM **87,524 B** — unchanged
- POOL **319,176 B** — unchanged
- package SHA-256 `86c44161d61b17697bebb2af7c3222449a0de1d20b74acc3cbaf2a8635cb7a75`

## Behavioral invariants retained

Idle emulator remains exact:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue high-water 28/512
- queue overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856; RMS 144/144; stereo delta 126

Active Motion lifecycle remains exact:
- seen `ff`; final `14`
- 149 points left / 149 right
- record start 220,672; finish 441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176
- cursor 481,024
- queue high-water 30/512; overflow 0; late flags 0
- WAV peak 1,171; RMS 173

## Interpretation

P11H measured **196,608 historical single-channel delay reads** over 16,384 room frames. P11I converts the six stereo locations per room frame from 12 duplicated address/fraction calculations into six shared pair calculations while retaining the exact channel arithmetic.

The smaller target code shape is real and reproducible. What remains unproven without hardware is whether that translates into a material FM-1 callback-cycle or ISR-margin gain. No such physical-timing claim is made.
