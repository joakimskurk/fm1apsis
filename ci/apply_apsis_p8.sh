#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 8 is visual-only. Generate the exact frozen Phase-7 target first, then
# add the native APSIS renderer without changing audio/runtime/persistence.
bash ci/apply_apsis_p7.sh

cp apsis/apsis_visual.h apsis/apsis_visual.c \
   apsis/apsis_visual_p8b.h apsis/apsis_visual_p8b.c \
   apsis/apsis_visual_p8c.h apsis/apsis_visual_p8c.c \
   work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

# Keep the raw SPI status register at the hardware-abstraction boundary.  The
# UI adapter only asks whether the current LCD DMA has completed; existing
# lcd_sync() still owns acknowledge/deselect and lcd_busy lifecycle.
hp = Path('work/apsis/firmware/hal/fm1_lcd_hw.h')
h = hp.read_text()
h_anchor = 'FM1_INLINE void fm1_lcd_baud(uint32_t b) { FM1_LCD_SPI_BAUD = b; }\n'
if h_anchor not in h:
    raise SystemExit('FAIL: P8 LCD HAL anchor missing')
h_helper = '''FM1_INLINE void fm1_lcd_baud(uint32_t b) { FM1_LCD_SPI_BAUD = b; }\n\nFM1_INLINE uint8_t fm1_lcd_done(void)\n{\n    return (uint8_t)((FM1_LCD_SPI_CON & 0x8000u) != 0u);\n}\n'''
h = h.replace(h_anchor, h_helper, 1)
hp.write_text(h)

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_project_fm1.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P8 Phase-7 header anchor missing')
s = s.replace(h_anchor,
              h_anchor + '\n#include "apsis_visual.h"\n#include "apsis_visual_p8b.h"\n#include "apsis_visual_p8c.h"', 1)

c_anchor = '#include "apsis_project_fm1.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P8 Phase-7 source anchor missing')
s = s.replace(c_anchor,
              c_anchor + '\n#include "apsis_visual.c"\n#include "apsis_visual_p8b.c"\n#include "apsis_visual_p8c.c"', 1)

start = s.find('static void draw_surface(void)')
end = s.find('static void leds(void)', start)
if start < 0 or end < 0:
    raise SystemExit('FAIL: P8 draw_surface anchors missing')
legacy = s[start:end]
legacy = legacy.replace('static void draw_surface(void)',
                        'static void draw_legacy_surface(void)', 1)

native = r'''#define APSIS_NATIVE_STRIP_ROWS 8u

static uint8_t apsis_native_draw_active;
static uint8_t apsis_native_draw_ui_surface = 0xFFu;
static uint16_t apsis_native_draw_y;

static uint8_t apsis_native_visual_surface(uint8_t ui_surface,
                                           ApsisVisualSurface *visual_surface)
{
    if (!visual_surface) return 0u;
    switch (ui_surface) {
    case SURF_MOTHER:  *visual_surface = APSIS_VISUAL_SURFACE_MOTHER;  return 1u;
    case SURF_STRING:  *visual_surface = APSIS_VISUAL_SURFACE_STRING;  return 1u;
    case SURF_DRIFT:   *visual_surface = APSIS_VISUAL_SURFACE_DRIFT;   return 1u;
    case SURF_CLOCK:   *visual_surface = APSIS_VISUAL_SURFACE_CLOCK;   return 1u;
    case SURF_SPIRAL:  *visual_surface = APSIS_VISUAL_SURFACE_SPIRAL;  return 1u;
    case SURF_BLOOM:   *visual_surface = APSIS_VISUAL_SURFACE_BLOOM;   return 1u;
    case SURF_GRAVITY: *visual_surface = APSIS_VISUAL_SURFACE_GRAVITY; return 1u;
    case SURF_CHAIN:   *visual_surface = APSIS_VISUAL_SURFACE_CHAIN;   return 1u;
    case SURF_KEEP:    *visual_surface = APSIS_VISUAL_SURFACE_KEEP;    return 1u;
    default: return 0u;
    }
}

static uint8_t apsis_lcd_dma_ready(void)
{
    /* lcd_busy is owned by pinned Dinghy. Hardware completion is queried only
     * through the HAL helper; no UI code reaches into SPI registers. When the
     * helper reports done, cv_begin()->lcd_sync() merely acknowledges and
     * deselects an already-complete transfer instead of spin-waiting for it. */
    return (uint8_t)(!lcd_busy || fm1_lcd_done());
}

static uint8_t draw_native_visual_step(ApsisVisualSurface visual_surface)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    uint32_t i;
    uint16_t rows;

    if (!apsis_native_draw_active || apsis_native_draw_ui_surface != surface) {
        apsis_native_draw_active = 1u;
        apsis_native_draw_ui_surface = surface;
        apsis_native_draw_y = 0u;
    }
    if (!apsis_lcd_dma_ready()) return 0u;

    rows = (uint16_t)(APSIS_VISUAL_HEIGHT - apsis_native_draw_y);
    if (rows > APSIS_NATIVE_STRIP_ROWS) rows = APSIS_NATIVE_STRIP_ROWS;

    context.surface = visual_surface;
    context.state = &apsis;
    context.runtime = &apsis_runtime;
    context.animation_tick = 0u;

    cv_begin(APSIS_VISUAL_WIDTH, rows, C_BLACK);
    apsis_visual_bind(&raster, cv_px, APSIS_VISUAL_WIDTH, rows,
                      APSIS_VISUAL_WIDTH, 0, (int16_t)apsis_native_draw_y);
    apsis_visual_render_p8c(&raster, &context, &keep_bank, &chain_sequence);
    for (i = 0u; i < (uint32_t)APSIS_VISUAL_WIDTH * rows; ++i)
        cv_px[i] = swap16(cv_px[i]);
    cv_blit(0u, apsis_native_draw_y);
    apsis_native_draw_y = (uint16_t)(apsis_native_draw_y + rows);

    if (apsis_native_draw_y >= APSIS_VISUAL_HEIGHT) {
        apsis_native_draw_active = 0u;
        return 1u;
    }
    return 0u;
}

static uint8_t draw_surface_step(void)
{
    ApsisVisualSurface visual_surface;
    if (apsis_native_visual_surface(surface, &visual_surface))
        return draw_native_visual_step(visual_surface);
    apsis_native_draw_active = 0u;
    apsis_native_draw_ui_surface = 0xFFu;
    draw_legacy_surface();
    return 1u;
}

'''
s = s[:start] + legacy + native + s[end:]

old_loop = '''        if (drawn != surface) {
            drawn = surface;
            draw_surface();
        }
'''
new_loop = '''        if (drawn != surface) {
            if (draw_surface_step()) drawn = surface;
        }
'''
if old_loop not in s:
    raise SystemExit('FAIL: P8 main-loop draw anchor missing')
s = s.replace(old_loop, new_loop, 1)
p.write_text(s)
PY

# Structural guards: Phase 8 reuses Dinghy's existing canvas, exposes no new
# framebuffer, remains outside the audio callback, preserves HOME->MOTHER, and
# never begins a new native strip while the previous LCD DMA is still active.
grep -q 'FM1_INLINE uint8_t fm1_lcd_done(void)' work/apsis/firmware/hal/fm1_lcd_hw.h
grep -q 'FM1_LCD_SPI_CON & 0x8000u' work/apsis/firmware/hal/fm1_lcd_hw.h
grep -q '#include "apsis_visual_p8c.c"' work/apsis/firmware/src/dinghy.c
grep -q '#define APSIS_NATIVE_STRIP_ROWS 8u' work/apsis/firmware/src/dinghy.c
grep -q 'return (uint8_t)(!lcd_busy || fm1_lcd_done());' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_visual_bind(&raster, cv_px, APSIS_VISUAL_WIDTH, rows' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_visual_render_p8c(&raster, &context, &keep_bank, &chain_sequence)' work/apsis/firmware/src/dinghy.c
grep -q 'case SURF_CHAIN:.*APSIS_VISUAL_SURFACE_CHAIN' work/apsis/firmware/src/dinghy.c
grep -q 'case SURF_KEEP:.*APSIS_VISUAL_SURFACE_KEEP' work/apsis/firmware/src/dinghy.c
grep -q 'if (b & (1u << BTN_HOME)) return SURF_MOTHER;' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('apsis_visual_render(', 'apsis_visual_render_p8(',
                  'apsis_visual_render_p8c(', 'draw_native_visual_step(',
                  'cv_begin(', 'cv_blit('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P8 display work leaked into app_audio: {forbidden}')
if 'FM1_LCD_SPI_CON' in s:
    raise SystemExit('FAIL: P8 UI reached past HAL into LCD SPI registers')
if 'static uint16_t full_frame[' in s or 'static uint16_t strip_frame[' in s:
    raise SystemExit('FAIL: P8 test framebuffer leaked into target')
if 'draw_surface();' in s:
    raise SystemExit('FAIL: blocking P8 draw_surface path still present')
print('PASS: P8 renderer is incremental, HAL-gated, reuses Dinghy canvas, and stays outside app_audio')
PY