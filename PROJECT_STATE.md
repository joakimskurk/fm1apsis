# PROJECT_STATE

Last updated: 2026-11-02

## Where we are

**v2.1, on `main`** (fast-forwarded from `v2.1-cpu` 2026-11-02; `v2` holds the v2.0 restructure): the String is
Study 98's (one string), and the audio fits on the emulator's cycle model.

- v2.0 (branch `v2`): APSIS as real source on Dinghy 0.4, one CI pipeline, sound bit-identical to P11W
- the String follows Study 98 (`apsis_string98.c`, DECISIONS.md): one string, every note a classicStringNote;
  Study 126's Dual String is gone from the sound and the default
- audio CPU, all bit-exact except the deliberate String change: voice-outer rendering everywhere, the voices'
  state in locals, one sine table in RAM, call-free inner loops, divide-free Q15 scaling (`apsis_fixed.h`), one
  memoised Q31 root (`apsis_q31.c`), String notes decaying in constant-gain runs

## Measured (CI, 2026-11-02, commit 5918797)

- fm1-emu FM1_STRICT, 20 s performance (`tests/emu.sh`): **avg 44.0 %, worst block 69.8 %**, 0 blocks over 80 %
  (P11W was avg 188 %, worst 209 %, half the blocks lost). The worst block: String notes 1.78 ms, room 1.0 ms,
  String drone 0.65 ms, Drift 0.34 ms (`load.json` in the CI artifact)
- real JieLi build: image ~115 KB / 581,564 B, RAM ~88.9 KB / 98,304 B, POOL 319,176 / 344,064 B, `FM-1_970`;
  `make fw` locally is byte-identical to CI
- package `apsis-5918797.fwsc` SHA-256 `9345d352dee4ee5600544388b42c2f12919e5ce9bcc3c9dc0710a6bec8dba5c4`
- 60 APSIS host tests pass (gcc, clang, ASan/UBSan, MSan); sound pinned (`tests/sound.sha256`)

## Known problems

- **level:** -34 dBFS RMS default before MASTER; compare with the browser
- **surfaces:** the renderers are static and simple; the Dual remnants (left side, anchors) still show in places
- Study 98's hybrid and bowed String characters are not ported; the Dual code in controls/Motion is unreachable
  but not yet removed
- the room still runs at 11.025 kHz; there is headroom now to revisit it
- the worst block is over half the budget (RULES.md asks for under half: USB and the key scan interrupt too)

## Next action

1. The user tests `apsis-5918797.fwsc` in the emulator (the String now sounds like Study 98).
2. v2.2 margin (docs/MARGIN_PLAN.md), branch `v2.2-margin`. Step A is done: the stress build collapses
   (302 % average, 483 % worst). The 17.8 ms planning spike was boot (now measured apart). While playing, the
   costs are Spiral + Bloom sound (15.3 ms) and String notes (10.2 ms). Next: B (one voice budget for String,
   Spiral and Bloom), then a boot warm-up, then C.
3. The load-meter build `apsis-loadmeter-ca04914.fwsc` is waiting for someone with an FM-1 (step F).

## Authority

- behaviour: Study 126 `index.html`, SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
  (CRLF; the LF copy is `c5b16e6d…`, same file); String timbre only: Study 98 `43553cc1…`
- platform: Dinghy `22acb9165ebc8baa712af3aff8c507e230c1e028` (0.4)
- emulator: fm1-emu `ce0f03e693da80bb5ce03b8a398f0f51e5989d8d` (0.4.0)
- identity: `FM-1_970` dev, `FM-1_97Y` for release X.Y (the old builds were `FM-1_972`)
- last old-pipeline package: P11W `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
