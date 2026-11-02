# CHANGELOG

Human-readable milestone history. Git and phase checkpoints remain the detailed evidence history.

## 2026-11-02 — Phase 12 hardware validation prepared

Opened draft PR #23 from `p12-hardware-validation`.

Added:
- `docs/PHASE12_HARDWARE_HANDOFF.md`
- `docs/PHASE12_HARDWARE_VALIDATION_PLAN.md`

Phase 12 is hardware-led and currently blocked on physical FM-1 availability. The first physical gate is P12A install/rescue. No speculative production optimization is authorized while hardware is unavailable.

The documentation-only Phase-12 head `0cf6a277613eb20689199800886384bf6f8c93c3` passed all 18 active PR workflows before the continuity-documentation refresh.

## 2026-11-02 — Phase 11 optimization/pre-hardware closure merged

PR #21 merged to `main` as `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c`.

Accepted production baseline: **P11W**.

Key accepted changes:
- compact String Motion storage **19,208 → 9,644 B**
- Bloom late slots **64 → 48** after exact legal max 44 proof
- Cathedral stereo-pair delay-read sharing
- FDN feedback-gain caching
- FDN line-read specialization
- exact sine endpoint cache with final A+delta representation

Final production target:
- image **140,472 B**
- RAM **87,524 B**
- POOL **319,176 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`

The final merge candidate passed 18/18 active PR workflows and P11Z run `41403698227` passed all eight closure gates.

Rejected candidates remain documented negative results: P11T selective narrow Q15 and P11Y fixed add/sub output mixing.

See `docs/PHASE11_FINAL_CHECKPOINT.md` for full evidence.

## 2026-11-01 — Phase 10 advanced interactive parity merged

PR #20 merged to `main` as `378d2023df2afe59475be53c09cc001788586514`.

Completed String Motion and advanced interaction parity including String Attractor, Spiral Attractor and selected Gravity-point editing. Final exact-head matrix: 22/22 PASS.

## 2026-11-01 — Phase 9 physical interaction parity implementation merged

PR #19 merged to `main` as `bbfab322557b6760425503268b550b789f796c23`.

Completed the frozen FM-1 interaction vocabulary, surface navigation, deterministic control routes and manual String/Bloom performance integration. Physical hardware behavior remained explicitly unverified.

## 2026-10-31 — Phase 8 native visuals merged

PR #18 merged to `main` as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457`.

Completed ten deterministic native 240×240 APSIS surfaces, 8-row DMA-gated LCD updates and exact visual/emulator regressions without a second framebuffer.

## 2026-10-30 — Phase 7 persistence merged

PR #17 merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285`.

Completed versioned CRC-protected KEEP/CHAIN persistence with a two-sector journal, fake-NOR recovery coverage and audio-safe deferred writes.

## 2026-10-30 — Phase 6 shared Space/Cathedral merged

PR #16 merged as `cd321e9f7999d0568d280d89e7e3574c3da66054`.

Completed one shared Cathedral room, compact modulated 8-line FDN and quarter-rate target integration.

## 2026-10-30 — Phase 5 sounding integration merged

Merged as `d677a887cf6dc8552634bde5a71fec0221c299b5` after end-to-end structural integration verification.

## Earlier milestones

Phases 0–4 established reference authority, the pinned FM-1/Dinghy pipeline, pure musical state, the shared deterministic runtime and the five dry source engines. Detailed history remains available in Git, `DECISIONS.md`, `TESTING.md` and phase-specific documents under `docs/`.
