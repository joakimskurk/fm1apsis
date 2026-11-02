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
- [x] A: stress build, the bound (sum of the parts' worsts) in CI, a load-meter build for a real FM-1
- [x] B: per-voice cost down bit for bit; Bloom as 2-op FM (A/B approved); hard caps for String, Spiral, Bloom with least-audible fades
- [ ] Clock's voice cost; the boot blocks warmed before the audio starts
- [~] C: planning cut in place (Spiral's route map, Bloom's voicing): 2.1 -> 1.0 ms; moving it out is not needed for the bound now
- [x] D: two cores (fm1-dock's HAL, screening, one-core fallback); drone, Drift, notes and the room (one block late) on core 1
- [ ] E: the mix into the buses directly; the String drone and Drift at block-rate coefficients
- [ ] F: the load-meter build on a real FM-1, the gap to FM1_STRICT in TESTING.md
- [x] the densest settings within the block: stress bound 90 %, no drop-out after boot (CI gate)
- [x] normal play under half: 30.9 % average, 46.7 % worst
- [ ] the first block after boot (~8 ms)
- [ ] stress under half (the bound 90 %): Clock, the commits, the planning

## v2.3 — sounds like APSIS
- [ ] gain staging against the browser (per source and master), then re-pin the sound
- [ ] the room: full-rate or better quarter-rate (aliasing, brightness), against the browser's Cathedral

## v2.4 — the screen, designed for the FM-1 (docs/screen/README.md)
- [x] references: Study 126 in play, every surface (docs/screen/reference/)
- [ ] a 240x240 design sketch per surface next to its reference (what moves, what the knobs show, what is left out)
- [ ] shared drawing pieces: glow tables, source colour ramps, 1-2 px lines and arcs at RGB565 cost
- [ ] the surfaces one by one, checked in host mode and fm1-emu (screenshot, frame rate) against the reference
- [ ] parameter feedback on every knob turn
- [ ] a short manual (the panel map, the surfaces)

## Later
- [ ] MIDI in/out and clock sync (Dinghy 0.4 has TRS MIDI, clock and transport)
- [ ] SMK-37 build
- [ ] listing on the FM-1 Firmware Hub / mutant.cloud
- [ ] physical FM-1 verification when hardware exists
