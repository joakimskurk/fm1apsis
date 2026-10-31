#ifndef APSIS_VISUAL_P8B_H
#define APSIS_VISUAL_P8B_H

#include "apsis_visual.h"

/* P8B extends the frozen P8A renderer without changing its MOTHER/GRAVITY
 * implementation or reference hashes. Unsupported surfaces fall through to
 * apsis_visual_render(). */
void apsis_visual_render_string(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context);
void apsis_visual_render_drift(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context);
void apsis_visual_render_clock(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context);
void apsis_visual_render_spiral(ApsisVisualRaster *raster,
                                const ApsisVisualContext *context);
void apsis_visual_render_bloom(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context);
void apsis_visual_render_p8(ApsisVisualRaster *raster,
                            const ApsisVisualContext *context);

#endif