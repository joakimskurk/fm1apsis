#ifndef APSIS_VISUAL_H
#define APSIS_VISUAL_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_runtime.h"

#define APSIS_VISUAL_WIDTH 240u
#define APSIS_VISUAL_HEIGHT 240u
#define APSIS_VISUAL_DINGHY_STRIP_MAX 124u
#define APSIS_VISUAL_ANGLE_STEPS 64u

/* Study-126 native palette, converted once to RGB565. */
#define APSIS_VISUAL_BG       0x0841u /* #090909 */
#define APSIS_VISUAL_FG       0xF77Cu /* #f0ede5 */
#define APSIS_VISUAL_MUTED    0x83EEu /* #827f77 */
#define APSIS_VISUAL_DIM      0x4A48u /* #4c4a45 */
#define APSIS_VISUAL_STRING   0xF5EAu /* #f2bd52 */
#define APSIS_VISUAL_DRIFT    0xFB6Cu /* #ff6f62 */
#define APSIS_VISUAL_CLOCK    0xCF6Du /* #c8ef6a */
#define APSIS_VISUAL_SPIRAL   0xCBDFu /* #c878ff */
#define APSIS_VISUAL_BLOOM    0xF571u /* #f3ad8f */
#define APSIS_VISUAL_TUNING   0xFBD6u /* #ff78b7 */
#define APSIS_VISUAL_GRAVITY  0xEF19u /* #efe3c9 */
#define APSIS_VISUAL_CHAIN    0x8EBFu /* #8fd7ff */
#define APSIS_VISUAL_MOTHER   0xD655u /* #d5c8aa */

typedef enum {
    APSIS_VISUAL_SURFACE_MOTHER = 0,
    APSIS_VISUAL_SURFACE_STRING,
    APSIS_VISUAL_SURFACE_DRIFT,
    APSIS_VISUAL_SURFACE_CLOCK,
    APSIS_VISUAL_SURFACE_SPIRAL,
    APSIS_VISUAL_SURFACE_BLOOM,
    APSIS_VISUAL_SURFACE_GRAVITY,
    APSIS_VISUAL_SURFACE_CHAIN,
    APSIS_VISUAL_SURFACE_KEEP,
    APSIS_VISUAL_SURFACE_SPACE,
    APSIS_VISUAL_SURFACE_COUNT
} ApsisVisualSurface;

/* Caller-owned raster. On FM-1 this binds to Dinghy's existing canvas scratch;
 * APSIS never allocates another framebuffer. x0/y0 are global-screen origins,
 * allowing the exact same renderer to draw full frames on host and clipped
 * strips/tiles on target. */
typedef struct {
    uint16_t *pixels;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    int16_t x0;
    int16_t y0;
} ApsisVisualRaster;

typedef struct {
    ApsisVisualSurface surface;
    const ApsisState *state;
    const ApsisRuntime *runtime;
    uint32_t animation_tick;
} ApsisVisualContext;

void apsis_visual_bind(ApsisVisualRaster *raster, uint16_t *pixels,
                       uint16_t width, uint16_t height, uint16_t stride,
                       int16_t x0, int16_t y0);
void apsis_visual_clear(ApsisVisualRaster *raster, uint16_t colour);
void apsis_visual_pixel(ApsisVisualRaster *raster, int16_t x, int16_t y,
                        uint16_t colour);
void apsis_visual_hline(ApsisVisualRaster *raster, int16_t x0, int16_t x1,
                        int16_t y, uint16_t colour);
void apsis_visual_line(ApsisVisualRaster *raster, int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1, uint16_t colour);
void apsis_visual_fill_rect(ApsisVisualRaster *raster, int16_t x, int16_t y,
                            int16_t width, int16_t height, uint16_t colour);
void apsis_visual_rect(ApsisVisualRaster *raster, int16_t x, int16_t y,
                       int16_t width, int16_t height, uint16_t colour);
void apsis_visual_circle(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                         int16_t radius, uint16_t colour);
void apsis_visual_disc(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                       int16_t radius, uint16_t colour);
void apsis_visual_polar(int16_t cx, int16_t cy, int16_t radius,
                        uint8_t angle64, int16_t *x, int16_t *y);
void apsis_visual_arc(ApsisVisualRaster *raster, int16_t cx, int16_t cy,
                      int16_t radius, uint8_t start64, uint8_t count64,
                      uint16_t colour);

/* Phase 8A reference surfaces. More surfaces join this dispatcher in P8B/P8C.
 * Rendering is a pure read of the existing APSIS state/runtime graph. */
void apsis_visual_render_mother(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context);
void apsis_visual_render_gravity(ApsisVisualRaster *raster,
                                 const ApsisVisualContext *context);
void apsis_visual_render(ApsisVisualRaster *raster,
                         const ApsisVisualContext *context);

#endif
