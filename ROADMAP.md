# ROADMAP

`[x]` done, `[~]` active, `[ ]` next. The old phases 0-12 are in `docs/history/ROADMAP_P0-P12.md`.

## v2.0 — real source, one pipeline
- [x] Dinghy 0.4 as source, no patch chain
- [x] app on 0.4's own input/screen/flash API
- [x] host tests against the shipping sources; host-mode sound pinned to P11W
- [x] CI: JieLi build, platform tests, host tests (gcc, clang, ASan/UBSan, MSan)
- [~] emulator load gate: red (188 % avg), see v2.1
- [ ] first `.fwsc` from v2 to the user's emulator testing

## v2.1 — measured CPU budget, the String from Study 98
- [x] FM1_STRICT load per part, per function (FM1_TPROF) and the voices' high water
- [x] a load budget in CI that fails the build (a block over 100 %)
- [x] the String from Study 98: one string
- [x] worst block under 70 % (69.8 %)
- [ ] remove the unreachable Dual code (controls, Motion lanes, surfaces); Study 98's hybrid/bowed characters

## v2.2 — margin by construction (docs/MARGIN_PLAN.md)
- [ ] A: stress script, the bound (sum of the parts' worsts) in CI, a load-meter build for a real FM-1
- [ ] B: one oscillator budget for String notes, Spiral and Bloom; least-audible stealing; same-pitch lanes if they pass A/B
- [ ] C: planning out of the audio interrupt into sample-stamped events
- [ ] D: the room on core 1 (fm1-dock's screening), a cheaper one-core fallback
- [ ] E: the mix into the buses directly; the String drone and Drift at block-rate coefficients
- [ ] F: the load-meter build on a real FM-1, the gap to FM1_STRICT in TESTING.md
- [ ] the bound at 50 % or less, measured worst at 40 % or less, average at 30 % or less (core 0, FM1_STRICT)

## v2.3 — sounds like APSIS
- [ ] gain staging against the browser (per source and master), then re-pin the sound
- [ ] the room: full-rate or better quarter-rate (aliasing, brightness), against the browser's Cathedral

## v2.4 — looks and feels like APSIS
- [ ] surfaces that move (Clock pulses, Spiral path, Bloom petals, String vibration, Drift beating)
- [ ] parameter feedback on every knob turn
- [ ] a short manual (the panel map, the surfaces)

## Later
- [ ] MIDI in/out and clock sync (Dinghy 0.4 has TRS MIDI, clock and transport)
- [ ] SMK-37 build
- [ ] listing on the FM-1 Firmware Hub / mutant.cloud
- [ ] physical FM-1 verification when hardware exists
