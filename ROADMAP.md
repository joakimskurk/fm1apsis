# ROADMAP

`[x]` done, `[~]` active, `[ ]` next. The old phases 0-12 are in `docs/history/ROADMAP_P0-P12.md`.

## v2.0 — real source, one pipeline
- [x] Dinghy 0.4 as source, no patch chain
- [x] app on 0.4's own input/screen/flash API
- [x] host tests against the shipping sources; host-mode sound pinned to P11W
- [x] CI: JieLi build, platform tests, host tests (gcc, clang, ASan/UBSan, MSan)
- [~] emulator load gate: red (188 % avg), see v2.1
- [ ] first `.fwsc` from v2 to the user's emulator testing

## v2.1 — measured CPU budget
- [ ] FM1_STRICT load per source and for the room (`fm1-live` FM1_TPROF for where the cycles go)
- [ ] worst-case scenario script (all sources, full CHAIN, Motion, Bloom grow)
- [ ] a load budget in CI that fails the build

## v2.2 — sounds like APSIS
- [ ] gain staging against the browser (per source and master), then re-pin the sound
- [ ] the room: full-rate or better quarter-rate (aliasing, brightness), against the browser's Cathedral
- [ ] the second core for the room, with fm1-dock's screening and a one-core fallback

## v2.3 — looks and feels like APSIS
- [ ] surfaces that move (Clock pulses, Spiral path, Bloom petals, String vibration, Drift beating)
- [ ] parameter feedback on every knob turn
- [ ] a short manual (the panel map, the surfaces)

## Later
- [ ] MIDI in/out and clock sync (Dinghy 0.4 has TRS MIDI, clock and transport)
- [ ] SMK-37 build
- [ ] listing on the FM-1 Firmware Hub / mutant.cloud
- [ ] physical FM-1 verification when hardware exists
