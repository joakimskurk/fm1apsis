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

## Measured (CI, 2026-11-02, commit 92dbd5e)

- real JieLi build: image 114,320 / 581,564 B, RAM 87,796 / 98,304 B, POOL 319,176 / 344,064 B, `FM-1_970`;
  Dinghy's platform tests pass; package `apsis-92dbd5e.fwsc` SHA-256
  `f243a0e153ac9273185cca3794e131b2c85c65422e3ad184b2cc34d88e5d3c3d`
- host: 62 tests pass with gcc (plain, ASan/UBSan) and clang (plain, MSan); sound pinned
- **fm1-emu FM1_STRICT, 20 s performance: the audio interrupt averages 10.9 ms per 5.8 ms block (188 %),
  worst 12.1 ms (209 %); only 1556 of ~3445 blocks are rendered; 97 % of instructions run in interrupts, so
  the panel and screen starve too.** This is the old P11W audio graph, unchanged; it cannot keep real time on
  the emulator's cycle model. The user's interactive emulator (not strict) hides this.
- where (the -fno-inline build, 6-8 s, Bloom sounding): Bloom DSP ~15 %, String transients ~20 %, the room
  ~10 %, Clock/String/Drift a few % each

## Known problems

- **CPU:** see above; the emulator job is red until the audio fits (v2.1)
- **level:** with the transport running the output is around -40 dBFS RMS / -23 dBFS peak before MASTER;
  compare with the browser and fix the gain staging
- **surfaces:** the renderers are static (they ignore `animation_tick`) and simple
- the room runs at 11.025 kHz to save CPU

## Next action

v2.1, make the audio fit: per-voice cost of Bloom and String transients (algorithm first: per-sample work that
can be per-block, voice caps against the browser's), hot DSP in RAM (`.ram_text`) against flash-cache misses,
then the second core (fm1-dock's screened, fall-back-to-one-core layer) for the room or a source group. Gate:
`tests/emu.sh` green with worst < 100 %, target < 70 %.

## Authority

- behaviour: Study 126 `index.html`, SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
  (CRLF; the LF copy is `c5b16e6d…`, same file); String timbre only: Study 98 `43553cc1…`
- platform: Dinghy `22acb9165ebc8baa712af3aff8c507e230c1e028` (0.4)
- emulator: fm1-emu `ce0f03e693da80bb5ce03b8a398f0f51e5989d8d` (0.4.0)
- identity: `FM-1_970` dev, `FM-1_97Y` for release X.Y (the old builds were `FM-1_972`)
- last old-pipeline package: P11W `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
