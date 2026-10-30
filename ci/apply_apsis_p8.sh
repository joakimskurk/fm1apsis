#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

# Phase 8 is visual-only. Generate the exact frozen Phase-7 target first, then
# add the native APSIS renderer without changing audio/runtime/persistence.
bash ci/apply_apsis_p7.sh

cp apsis/apsis_visual.h apsis/apsis_visual.c work/apsis/firmware/src/

python3 - <<'PY'
from pathlib import Path

p = Path('work/apsis/firmware/src/dinghy.c')
s = p.read_text()

h_anchor = '#include "apsis_project_fm1.h"'
if h_anchor not in s:
    raise SystemExit('FAIL: P8 Phase-7 header anchor missing')
s = s.replace(h_anchor, h_anchor + '\n#include "apsis_visual.h"', 1)

c_anchor = '#include "apsis_project_fm1.c"'
if c_anchor not in s:
    raise SystemExit('FAIL: P8 Phase-7 source anchor missing')
s = s.replace(c_anchor, c_anchor + '\n#include "apsis_visual.c"', 1)

start = s.find('static void draw_surface(void)')
end = s.find('static void leds(void)', start)
if start < 0 or end < 0:
    raise SystemExit('FAIL: P8 draw_surface anchors missing')
legacy = s[start:end]
legacy = legacy.replace('static void draw_surface(void)',
                        'static void draw_legacy_surface(void)', 1)

native = r'''static uint8_t apsis_native_visual_surface(uint8_t ui_surface,
                                           ApsisVisualSurface *visual_surface)
{
    if (!visual_surface) return 0u;
    if (ui_surface == SURF_MOTHER) {
        *visual_surface = APSIS_VISUAL_SURFACE_MOTHER;
        return 1u;
    }
    if (ui_surface == SURF_GRAVITY) {
        *visual_surface = APSIS_VISUAL_SURFACE_GRAVITY;
        return 1u;
    }
    return 0u;
}

static void draw_native_visual(ApsisVisualSurface visual_surface)
{
    ApsisVisualRaster raster;
    ApsisVisualContext context;
    uint32_t y, i;
    context.surface = visual_surface;
    context.state = &apsis;
    context.runtime = &apsis_runtime;
    context.animation_tick = 0u;

    /* Dinghy already owns one 240x124 RGB565 canvas in POOL. Reuse it as two
     * 240x120 strips; never allocate a second framebuffer. gfx.c stores panel
     * pixels byte-swapped, while APSIS keeps canonical RGB565 for host hashes,
     * so swap each completed strip exactly once before the DMA blit. */
    for (y = 0u; y < APSIS_VISUAL_HEIGHT; y += 120u) {
        cv_begin(240u, 120u, C_BLACK);
        apsis_visual_bind(&raster, cv_px, 240u, 120u, 240u, 0, (int16_t)y);
        apsis_visual_render(&raster, &context);
        for (i = 0u; i < 240u * 120u; ++i)
            cv_px[i] = swap16(cv_px[i]);
        cv_blit(0u, y);
    }
}

static void draw_surface(void)
{
    ApsisVisualSurface visual_surface;
    if (apsis_native_visual_surface(surface, &visual_surface))
        draw_native_visual(visual_surface);
    else
        draw_legacy_surface();
}

'''
s = s[:start] + legacy + native + s[end:]
p.write_text(s)
PY

# Structural guards: Phase 8 reuses Dinghy's existing canvas, exposes no new
# framebuffer, and remains outside the audio callback.
grep -q '#include "apsis_visual.c"' work/apsis/firmware/src/dinghy.c
grep -q 'apsis_visual_bind(&raster, cv_px, 240u, 120u' work/apsis/firmware/src/dinghy.c
grep -q 'draw_native_visual' work/apsis/firmware/src/dinghy.c
python3 - <<'PY'
from pathlib import Path
s = Path('work/apsis/firmware/src/dinghy.c').read_text()
a = s.index('static void app_audio(')
b = s.index('static const char *surface_name', a)
audio = s[a:b]
for forbidden in ('apsis_visual_render(', 'draw_native_visual(', 'cv_begin(', 'cv_blit('):
    if forbidden in audio:
        raise SystemExit(f'FAIL: P8 display work leaked into app_audio: {forbidden}')
if 'static uint16_t full_frame[' in s or 'static uint16_t strip_frame[' in s:
    raise SystemExit('FAIL: P8 test framebuffer leaked into target')
print('PASS: P8 renderer reuses Dinghy canvas and stays outside app_audio')
PY
