#ifndef APSIS_VISUAL_P8C_H
#define APSIS_VISUAL_P8C_H

#include "apsis_visual_p8b.h"
#include "apsis_keep.h"
#include "apsis_chain.h"

/* P8C reads the already-existing KEEP/CHAIN objects directly. These pointers
 * are visual observers only; no musical/project state is copied or owned by
 * the renderer. SPACE needs only ApsisState because it is one shared room. */
void apsis_visual_render_keep(ApsisVisualRaster *raster,
                              const ApsisVisualContext *context,
                              const ApsisKeepBank *keep_bank,
                              const ApsisChainSequence *chain_sequence);
void apsis_visual_render_chain(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context,
                               const ApsisKeepBank *keep_bank,
                               const ApsisChainSequence *chain_sequence);
void apsis_visual_render_space(ApsisVisualRaster *raster,
                               const ApsisVisualContext *context);
void apsis_visual_render_p8c(ApsisVisualRaster *raster,
                             const ApsisVisualContext *context,
                             const ApsisKeepBank *keep_bank,
                             const ApsisChainSequence *chain_sequence);

#endif