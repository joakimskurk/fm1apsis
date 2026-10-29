#ifndef APSIS_STRING_ATTRACTOR_AUDIO_H
#define APSIS_STRING_ATTRACTOR_AUDIO_H

#include <stdint.h>
#include "apsis_string_attractor.h"
#include "apsis_string_dsp.h"

/* Resolve one already-planned Attractor side at bind time. Planning-time route,
 * S&H value/position/base level remain frozen in binding; current String state
 * supplies sound/pitch/source level/side level/brightness/Foundation. Voices
 * are allocated into the existing M4b pool with a start delay from
 * render_origin_sample to the frozen musical target. Returns accepted pool
 * voices/partials. */
uint8_t apsis_string_attractor_trigger_binding(
    ApsisStringTransientPool *pool,
    const ApsisState *state,
    ApsisStringPitchMemory *pitch_memory,
    const ApsisStringAttractorBinding *binding,
    uint64_t render_origin_sample);

#endif
