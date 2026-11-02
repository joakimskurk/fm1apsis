# PROJECT_STATE

Last updated: 2026-11-02

## Where we are

**v2 restructure** (branch `v2`): the firmware is now real source on Dinghy 0.4 instead of Dinghy 0.3 plus a
chain of fifteen CI patch scripts. Phases 0-11 (`docs/history/`) built the musical core, the sources, the room,
persistence, the surfaces and the controls; v2 keeps all of that and replaces how it is assembled, tested and
measured.

Done in v2 so far:
- Dinghy 0.4 imported as source (`PLATFORM.md`); no HAL, loader or linker-script changes
- the P11W production sources (the ones the old chain generated) are `apsis/`; the runtime's macro-renamed onion
  layers are one `apsis_runtime.c`, dead layer code removed
- `firmware/src/app.c`: the audio graph unchanged; input on Dinghy 0.4's own edges (no HAL activity-bit patch);
  continuous strip-by-strip screen redraw (the old one redrew only on surface change, so knobs showed nothing);
  no placeholder boot screen; LEDs written on change; an audio-load probe
- the project journal on Dinghy's blob sectors 0/1 (same addresses as before), flash behind three primitives so
  it also runs in host mode
- tests: 62 APSIS host tests against the shipping sources (plain + ASan/UBSan); the Phase-11 optimisation proofs
  that tested intermediate candidates are removed; `test_space_room` now tests the room that ships
- **sound unchanged:** the whole firmware in Dinghy host mode is bit-identical to the old P11W app on two scripts
  (`tests/sound.sha256`, see TESTING.md)
- one CI workflow: host tests, host-mode sound, JieLi build + Dinghy's platform tests, fm1-emu FM1_STRICT load

## Not yet verified

- the CI run of this branch (JieLi build, platform tests, emulator) — first run pending
- the audio interrupt's load under FM1_STRICT (the first real measurement of CPU headroom)
- anything on a physical FM-1 (none available)

## Known problems (found in the v2 review)

- **level:** with the transport running the output is around -40 dBFS RMS / -23 dBFS peak before MASTER;
  compare with the browser and fix the gain staging
- **surfaces:** the renderers are static (they ignore `animation_tick`) and simple; parity with the browser's
  visuals is far off
- the room runs at 11.025 kHz to save CPU; with a measured budget and the second core this can be revisited

## Next action

1. Push `v2`, read the CI run; fix what the real build/emulator finds.
2. Give the user `apsis-<sha>.fwsc` (SHA-256 from CI) for the emulator.
3. Then the roadmap: measured CPU budget -> level/gain parity -> living surfaces.

## Authority

- behaviour: Study 126 `index.html`, SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
  (CRLF; the LF copy is `c5b16e6d…`, same file); String timbre only: Study 98 `43553cc1…`
- platform: Dinghy `22acb9165ebc8baa712af3aff8c507e230c1e028` (0.4)
- emulator: fm1-emu `ce0f03e693da80bb5ce03b8a398f0f51e5989d8d` (0.4.0)
- identity: `FM-1_970` dev, `FM-1_97Y` for release X.Y (the old builds were `FM-1_972`)
- last old-pipeline package: P11W `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
