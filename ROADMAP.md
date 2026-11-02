# ROADMAP

APSIS is a parity-first native port/adaptation of the browser instrument to M-VAVE FM-1. The browser/HTML version remains the product and musical reference; FM-1-specific adaptations are allowed only where hardware requires them.

## Completed phases

- [x] Phase 0 — reference/source freeze
- [x] Phase 1 — pinned FM-1 skeleton, build and emulator pipeline
- [x] Phase 2 — pure musical/state core
- [x] Phase 3 — deterministic shared transport/runtime
- [x] Phase 4 — five dry source engines
- [x] Phase 5 — sounding-source structural integration
- [x] Phase 6 — shared Space/Cathedral
- [x] Phase 7 — KEEP/CHAIN persistent storage
- [x] Phase 8 — native 240×240 visual surfaces
- [x] Phase 9 — physical interaction/control parity implementation
- [x] Phase 10 — String Motion + advanced interactive parity
- [x] Phase 11 — measured optimization and pre-hardware closure

Key merge authorities:
- Phase 5 `d677a887cf6dc8552634bde5a71fec0221c299b5`
- Phase 6 `cd321e9f7999d0568d280d89e7e3574c3da66054`
- Phase 7 `e9000b2b2f74d4fafe96df87f030cc55916a8285`
- Phase 8 `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` / PR #18
- Phase 9 `bbfab322557b6760425503268b550b789f796c23` / PR #19
- Phase 10 `378d2023df2afe59475be53c09cc001788586514` / PR #20
- Phase 11 `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c` / PR #21

Phase-11 final authority is documented in `docs/PHASE11_FINAL_CHECKPOINT.md`.

## Active phase — Phase 12 hardware validation/release

Branch: `p12-hardware-validation`

Draft PR: #23

Physical FM-1 hardware is currently unavailable, so these gates are prepared but not yet level-6 verified.

- [ ] **P12A — install and rescue**
  - regenerate exact frozen P11W package
  - verify package SHA-256
  - install on physical FM-1
  - repeated cold/warm boot
  - demonstrate rescue/back-to-official
  - reinstall APSIS

- [ ] **P12B — physical I/O smoke**
  - all 14 buttons
  - all 7 encoders
  - all 27 keys
  - modifier routes
  - LEDs/display
  - stereo audio output

- [ ] **P12C — functional musical parity**
  - MOTHER, GRAVITY, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, KEEP, CHAIN and SPACE/CATHEDRAL
  - String Motion, Attractors and Gravity point editing
  - sparse and dense states

- [ ] **P12D — real callback/ISR timing**
  - diagnostic-only timing instrumentation using a real target timing source
  - worst-case legal musical loads
  - callback high-water/deadline evidence

- [ ] **P12E — LCD/control/audio contention stress**
  - aggressive navigation, controls and display activity under dense audio load

- [ ] **P12F — runtime stack high-water**
  - real target stack boundary/guard/high-water evidence

- [ ] **P12G — real NOR persistence/interruption**
  - requires P12A rescue proof first
  - normal save/reboot plus controlled interruption/recovery matrix

- [ ] **P12H — DAC/listening and perceptual tuning**
  - noise, clicks, balance, clipping, relative levels, Cathedral and performance feel

- [ ] **P12I — release freeze**
  - final clean build
  - exact release commit/package SHA
  - final physical evidence
  - install + rescue instructions
  - final regression CI
  - version/tag/release notes

Full gate procedures and PASS/FAIL rules: `docs/PHASE12_HARDWARE_VALIDATION_PLAN.md`.

## Development rules during Phase 12

- No speculative production optimization while hardware evidence is unavailable.
- Do not reopen rejected Phase-11 candidates without new measured evidence.
- Diagnostic builds remain separate from the release candidate.
- A physical failure must be reproducible before production source is changed.
- Do not perform destructive NOR/power-interruption tests until rescue/back-to-official is proven on the same physical unit.
- Host, sanitizer, target, emulator and physical-hardware evidence must remain clearly distinguished.

## Exact next action

Acquire/connect a physical FM-1 and execute **P12A install + rescue** against the frozen P11W production authority. Until then, keep the release candidate frozen and only perform non-speculative repository/validation preparation.
