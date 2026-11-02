# PLATFORM

APSIS runs on **Dinghy 0.4**, imported as source.

- upstream: https://github.com/zvenson/dinghy
- commit: `22acb9165ebc8baa712af3aff8c507e230c1e028` (2026-10-23, "Dinghy 0.4")
- imported: `firmware/` (hal, loader, src platform files, app.ld, crt0.S, gen), `tools/`, `build.sh`, `tests/`
  (platform tests), `assets/`, `docs/` -> `docs/dinghy/`, `LICENSE` (GPL-3.0-only)

## What differs from upstream

Kept deliberately small, so a later Dinghy can be merged by diffing these files.

| file | change |
| --- | --- |
| `firmware/src/dinghy.c` | removed; the app is `firmware/src/app.c` |
| `tools/build.py` | compiles `app.c` with `-Iapsis`; outputs `apsis.*`; identity `FM-1_97N` (`BOARD_PREFIX`), loader screen "APSIS" in brass, data family `APSIS` |
| `tests/run_host.sh` | builds `app.c` (`-Iapsis`) as `build/host/apsis` |
| `tests/run_tests.sh` | the FM-1 app checks only (no SMK-37 app build); `apsis.*` names |
| `tests/install_test.py` | `apsis.*` names |
| `tests/scripts/midi.txt`, `smk37.txt` | removed (Dinghy's demo app scripts) |

Nothing in `firmware/hal/`, `firmware/loader/`, `firmware/app.ld` or the platform's `src/` files is changed.
APSIS uses the platform as Dinghy documents it (`docs/dinghy/API.md`):

- the audio interrupt `app_audio()`, the main loop `app_main()`, `platform_poll()`
- input edges `fm1_input_edges()` / `fm1_input_note_edges()` / `fm1_enc_take()`, LEDs `fm1_led_key()`
- the screen canvas `cv_begin()` / `cv_px` / `cv_blit()` and `fm1_lcd_done()`
- flash: the project journal writes Dinghy's blob sectors 0 and 1 (`BLOBS_AT`, 0xF4000/0xF5000) with
  `st_read()` / `fl_erase4k()` / `fl_write()`; `app.c` asserts the addresses at compile time
- large buffers (the room, Motion) in `.pool.*`, which `app.ld` places in POOL

## Updating Dinghy

1. Diff the new upstream against the commit above for the imported paths.
2. Apply upstream changes; re-apply the table above.
3. `sh tests/apsis/run.sh`, `sh tests/sound.sh` (the sound should not change), CI's firmware and emulator jobs.
4. Update the commit here and in `PROJECT_STATE.md`.
