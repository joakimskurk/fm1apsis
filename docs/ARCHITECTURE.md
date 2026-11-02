# Architecture

One translation unit: `firmware/src/app.c` includes Dinghy's `platform.c` and every `apsis/*.c` it needs (Dinghy's
unity build, `-Os`). The host tests compile the same `apsis/*.c` files separately; there is one source of every
module, and it is the one the firmware ships.

## Two contexts

| | runs in | owns | may not |
| --- | --- | --- | --- |
| `app_audio()` -> `render_block()` | the ALNK0 interrupt, every 256 frames (5.805 ms) | all DSP state, the runtime's sample cursor, the finite voice pools | touch flash, the screen or the panel |
| `app_main()` | the main loop | the panel, navigation, the editors, saving, the screen | render sound |

The main loop talks to the audio interrupt through narrow mailboxes (`apsis_controls_performance_*`,
`apsis_string_motion_mailbox_*`) written inside a short `fm1_irq_off()`; structural requests (Gravity, MOTHER,
CHAIN) go through the shared runtime's own request functions.

## The state and the runtime

- `apsis_state.[ch]`: `ApsisState`, the one musical state (defaults from Study 126).
- `apsis_runtime.[ch]`: the one sample-referenced scheduler: 64-bit sample cursor, a 512-event shared heap,
  the transport, Clock pulses, Spiral and Bloom occurrences with late binding, the String Attractor, CHAIN and
  MOTHER commits, the shared RNG. No module has a private clock. (Merged from the old M3b/M3c/M4c/M4f/M4g layers.)
- `apsis_transport`, `apsis_late_binding`, `apsis_gravity`, `apsis_harmony`, `apsis_keep`, `apsis_chain`,
  `apsis_mother`: the parts it is built from.

## One audio block (`render_block`)

1. the room's block begins (`apsis_space_target_begin_block`); Motion playback takes its controls
2. String and Drift update from the state at block start (a change sounds at most one block late, never early)
3. Clock events of this block are collected and triggered
4. each source renders into the shared source bus, then commits to the dry output and its Space send:
   String sustain, Drift, Clock, String transients; Spiral and Bloom once their occurrences are due
5. the runtime advances the block (with Bloom/Spiral/Attractor observers as active), binding late events
6. String Attractor, String rhythm, manual String plucks and Bloom gestures trigger their finite voices
7. the shared Space/Cathedral renders its return into the output (`apsis_space_target_render`):
   echo + 8-line modulated FDN, run at 11.025 kHz inside, 44.1 kHz in and out
8. Motion applies its pulses; the probes are written

## The screen

Ten surfaces (`apsis_visual*.c`), drawn by `draw_step()` one 8-row strip per main-loop pass into Dinghy's canvas,
each strip only when the previous strip's DMA is done: the screen follows the state continuously and the loop
never waits on the LCD. `animation_tick` counts full frames.

## The project journal

KEEP (8 snapshots) and CHAIN (32 cells + settings) persist as an 840-byte v1 record (`apsis_project_store.c`:
explicit little-endian, CRC-32 on header and payload) in an A/B journal over Dinghy's blob sectors 0 and 1
(`apsis_project_fm1.c`): payload written and verified first, the header last; the newest valid generation wins.
Saves run in `app_main()` with the transport stopped and the audio quiesced.

## Telemetry

`apsis_runtime_probe[6]` (queue, cursor, late flags) and `apsis_load_probe[4]` (the audio interrupt's last, worst,
count and total time in us, from `fm1_ticks()`) are plain RAM words; `tests/emu.sh` reads them from the emulator.
