#ifndef APSIS_STRING_MOTION_DSP_H
#define APSIS_STRING_MOTION_DSP_H

#include "apsis_string_motion.h"
#include "apsis_string_dsp.h"

/* P10-only persistent-pitch override. Call immediately after the frozen M4a
 * apsis_string_dsp_update(). Foundation, gain/timbre and render topology stay
 * owned by M4a; only primary pitch targets are replaced by recorded motion. */
void apsis_string_motion_dsp_override(ApsisStringDsp *dsp,
                                      const ApsisState *state,
                                      const ApsisStringMotionPlayback *playback);

#endif
