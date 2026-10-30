# Phase 8 — native 240×240 visual contract

Status: **P8A contract / primitive foundation**

Authority: Study 126 / `apsis.html` SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`, plus the frozen APSIS behaviour map and UI mapping.

The goal is not to fit a generic synth UI onto FM-1. The 240×240 panel must still read as APSIS from its geometry, source colour language and shared-state behaviour.

## Hardware rendering facts

Pinned Dinghy/FM-1 provides:
- 240×240 RGB565 display;
- SPI1 display link at 12 MHz;
- DMA-backed `lcd_blit()` / `lcd_fill()`;
- one existing 240×124 RGB565 canvas scratch allocation (`59,520` bytes in POOL);
- `cv_begin(y,h)` clips canvas strips to at most 124 rows.

Phase 8 therefore **must reuse Dinghy's existing canvas scratch**. It may not add a second framebuffer or another large static pixel store.

Raw transfer lower bounds at 12 MHz, before protocol/CPU overhead:
- full 240×240 RGB565 frame: 115,200 bytes, 76.8 ms;
- one maximum 240×124 canvas: 59,520 bytes, 39.68 ms;
- one 240×32 dynamic strip: 15,360 bytes, 10.24 ms.

A full-screen animated redraw loop is therefore forbidden. Surface changes may rebuild the full screen in two bounded strips; ongoing animation must use dirty strips/tiles and derived runtime state only.

## Coordinate contract

Native screen coordinates are always `x=0..239`, `y=0..239`.

Logical zones:
- `y=0..15`: minimal surface identity / transport / pending-state witnesses;
- `y=16..223`: identity-bearing APSIS geometry field;
- `y=224..239`: normally quiet; reserved for transient value confirmation/hints.

These are layout guides, not boxed chrome. Geometry may cross the guide boundaries when the original APSIS surface needs it.

A full rebuild is exactly two target strips:
- top `y=0..119`, 240×120;
- bottom `y=120..239`, 240×120.

The renderer uses global screen coordinates and clips against the caller-provided strip/tile. Rendering the same surface as one 240×240 host raster or as two target strips must produce identical pixels.

## Native palette

Study 126 source tokens are frozen and converted directly to RGB565:

| role | source hex | RGB565 |
| --- | --- | --- |
| background | `#090909` | `0x0841` |
| foreground | `#f0ede5` | `0xF77C` |
| muted | `#827f77` | `0x83EE` |
| dim | `#4c4a45` | `0x4A48` |
| STRING | `#f2bd52` | `0xF5EA` |
| DRIFT | `#ff6f62` | `0xFB6C` |
| CLOCK | `#c8ef6a` | `0xCF6D` |
| SPIRAL | `#c878ff` | `0xCBDF` |
| BLOOM | `#f3ad8f` | `0xF571` |
| TUNING | `#ff78b7` | `0xFBD6` |
| GRAVITY | `#efe3c9` | `0xEF19` |
| CHAIN | `#8fd7ff` | `0x8EBF` |
| MOTHER | `#d5c8aa` | `0xD655` |

SPACE is one shared room, not a sixth source. Its view uses the MOTHER/neutral family plus the five actual source-send colours rather than inventing a new source colour.

## Rendering architecture

`apsis_visual.*` is pure rendering logic:
- no allocation;
- no ownership of musical state;
- no clocks, random state or parameter copies;
- no floating point or target trig dependency;
- caller-owned RGB565 raster only;
- fixed 64-step Q10 polar LUT for arcs/spirals/orbits;
- integer clipping and primitives;
- existing `ApsisState` and `ApsisRuntime` are read-only truth.

On host, the raster may be a full 240×240 test buffer. On FM-1 it binds to Dinghy's already-budgeted canvas scratch. A later target adapter may use the same scratch memory as a narrow local tile for `lcd_blit(x,y,w,h,...)`; it must not allocate another pixel buffer.

## Surface identity locks

Primary surfaces may simplify glow, gradients and antialiasing, but may not become lists of generic knobs/boxes.

### MOTHER
- five coloured source arcs around a living central core;
- arc extent represents source mix;
- mute visibly collapses/dims that source;
- structural sync / pending `->1` state is structural, not a popup;
- five-source topology is always legible.

### GRAVITY
- exactly three concentric rings × eight angular degrees;
- selected/current/pending point feedback must be distinguishable;
- tonic/scale may be textual metadata but may not replace the orbit.

### STRING
- horizontal String field;
- visible `1:1` centre / ratio structure;
- one or two handles according to Classic/Dual;
- Motion/Attractor layers remain visual layers, not menu rows.

### DRIFT / TUNING
- paired beating centres;
- three Tuning stations remain part of the composition.

### CLOCK
- outer MAIN, middle BETWEEN, inner SNAP rings;
- grouping arcs outside;
- moving playhead/probe belongs to the orbit.

### SPIRAL
- true 36-point, three-turn spiral;
- ordered selected route drawn on that geometry;
- outer attractor ring + travelling probe;
- a step list is explicitly not an acceptable replacement.

### BLOOM
- interval-derived flower with core/petals/stem/leaf;
- unfurl/probability activity must read spatially;
- currently active petals can brighten without requiring full-screen redraw.

### KEEP
- eight memories remain direct/visual;
- occupied, selected and current/chain-source states are distinguishable;
- not a generic preset browser.

### CHAIN
- memory orbit / curved phrase progression up to 32 cells;
- selected editor node, current play node and phrase progress are distinct.

### SPACE
- one shared room view;
- show five source sends into the common space;
- never render five independent reverbs.

## Redraw rules

1. Surface entry / major structural layout change: full screen, two 240×120 strips.
2. Ordinary parameter change: redraw only the smallest deterministic band/tile containing the changed geometry.
3. Continuous animation: target provisional goal is <=32 full-width rows per update or a smaller direct tile, not a full frame.
4. Text/value confirmations are transient overlays and then return to geometry.
5. Rendering may observe runtime counters/probes but must never advance or mutate musical runtime.
6. LEDs and screen derive from the same APSIS state; neither owns a second state language.

Animation rate and exact dirty-tile limits remain provisional until physical FM-1 display/audio coexistence can be measured.

## Phase 8A implementation checkpoint

The first renderer foundation implements:
- caller-owned clipped RGB565 raster;
- line / rect / circle / disc / 64-step polar arc primitives;
- exact Study-126 RGB565 palette;
- MOTHER reference geometry driven directly by `ApsisState.mother`;
- GRAVITY 3×8 reference geometry driven directly by `ApsisState.gravity` plus runtime pending Gravity;
- host proof that one full render equals two 240×120 renders bit-for-bit;
- clipping guard tests so target strips cannot write outside their caller-owned buffer.

P8B expands this same renderer across STRING / DRIFT-TUNING / CLOCK / SPIRAL / BLOOM, then P8C covers KEEP / CHAIN / SPACE and target integration.