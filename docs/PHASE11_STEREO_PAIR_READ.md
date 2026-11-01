# Phase 11I — Cathedral stereo-pair delay read

P11I targets one narrow repeated Cathedral hot path identified by P11H: duplicated left/right delay-address calculation.

## Pre-patch proof authority

- proof run `41343196262`
- proof head `32bb42c52f666af12065737e14a8846d3d8f01ab`
- **150,412** equivalence cases
- optimized host build PASS
- ASAN/UBSAN PASS
- complete historical host suite PASS
- production `apsis_space_room.c` remained at frozen pre-patch blob `2d7c34e32b48b904623d740a70cf657fee0801a7`

The candidate pair reader was proved bit-identical to the historical single-channel authority before production code was modified. The proof workflow is manual-only and pinned to the proven pre-patch head.

## Authorized production change

Production commit `4629406bb6e417b0bcb544e6a770c0860613e7cd` replaces the single-channel room delay helper with one stereo-pair helper. The four early-reflection stereo taps, predelay and echo delay now perform the common validity/wrap/index/fraction calculation once per stereo location while retaining the exact historical per-channel interpolation arithmetic.

No delay length, coefficient, FDN topology, smoothing rule, state capacity or sample format changes.

The post-patch equivalence harness now compares the actual production pair helper directly against an embedded copy of the frozen pre-P11I single-channel algorithm over the same 150,412 cases.

## Target-generator compatibility

The Phase-6 target generator historically carried a target-only 32-bit convex-interpolation fast path anchored to the old single-reader source text. P11I extends that adapter backward-compatibly:

- historical single-reader source continues to receive its original fast path;
- the proven stereo-pair source receives the corresponding safe 32-bit interpolation independently for left and right;
- host/reference room math is unchanged.

Adapter support is present on branch head commit `5b760bcde63742fc504cce281e59f4b792b8841a`.

## Acceptance still pending

P11I is **not accepted yet**. The post-adapter real-target validation must prove all of the following on one exact head:

1. production pair helper remains bit-identical to frozen single-read authority, normal and ASAN/UBSAN;
2. frozen room hashes and full historical host behavior remain exact;
3. real JieLi target resource use does not regress;
4. aggregate target code shape for `apsis_space_room_render` + delay reader improves versus P11G baseline **2,270 B / 718 static disassembly instruction lines**;
5. idle PCM/runtime remains exact;
6. active Motion lifecycle remains exact;
7. exact production and active target hashes/measurements are recorded before promotion.

Any code-shape result is a target-build proxy only, not proof of physical FM-1 CPU or ISR improvement.
