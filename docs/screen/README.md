# The FM-1 screen, designed for it

The FM-1's screen is not a small browser. The goal is surfaces made for it that a player still recognises as
APSIS. This folder holds the references and, later, the design.

## Reference: Study 126 while it plays (2026-11-02)

`reference/` holds Study 126 (`index.html`) in play, at its defaults, viewport 1280x960 (captured at 800x600):

| file | surface | what carries it |
| --- | --- | --- |
| 1-string.png | STRING | the horizontal string with its 1:1 centre and ratio ticks, the two handles, the orbit above, the motion line and gates below |
| 2-drift.png | DRIFT / TUNING | two rings beating around a centre dot, a red glow; three tuning nodes below |
| 3-clock.png | CLOCK | concentric rings of steps (main, inner, snap), the bounds as gaps, the playing step bright |
| 4-spiral.png | SPIRAL | the spiral of 36 points, the route as lines inside, the probe moving out to the playing point |
| 5-bloom.png | BLOOM | petals that unfurl from a stem, a leaf; each petal a voice |
| 6-gravity.png | GRAVITY | the ring of eight degrees, the sources as coloured planets, "C · DORIAN · LOCKED" |
| 7-chain.png | CHAIN | the KEEP cells and the chain ring (empty here) |
| 8-mother.png | MOTHER | five arcs in the sources' colours around a fractal living core, "five sources · one living core" |

Each source has a colour, and the screens keep to it: String amber, Drift red, Clock lime, Spiral violet, Bloom
peach. Everything sits on near-black, with thin lines and soft glows.

## What the FM-1 gives and costs

- 240 x 240 RGB565, square. The browser's tabs and KEEP strip become buttons and LEDs, so the whole square can be
  the surface.
- The screen is drawn in the main loop in 8-row strips (app.c `draw_step`), never in the audio interrupt. Its
  cost competes with control work, not with sound, but a frame must still be cheap enough to move smoothly
  (about 20 fps or better).
- There is no alpha compositing for free. Glows have to be precomputed (radial ramps in a table), and thin lines
  have to be drawn in 1-2 px with a dimmed neighbour colour instead of antialiasing.
- The small font is 16 px. Text is for the one line that matters (tonic and scale, a value being turned), not for
  labels.

## The work (ROADMAP v2.4)

1. For each surface, a 240x240 design sketch next to its reference. It answers three questions: what moves, what
   the knobs show while they turn, and what is left out.
2. Shared drawing pieces: glow tables, the colour ramps, 1-2 px lines, and arcs at RGB565 cost.
3. The surfaces one by one, each checked in host mode (PNG frames) and in fm1-emu (a screenshot and the frame
   rate), against its reference.
