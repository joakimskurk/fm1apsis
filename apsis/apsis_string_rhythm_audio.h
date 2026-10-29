#ifndef APSIS_STRING_RHYTHM_AUDIO_H
#define APSIS_STRING_RHYTHM_AUDIO_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_transport.h"

/* Bind one already-consumed shared STRING_RHYTHM event to the current String
 * audio state. The event freezes Rhythm mode at planning time; surface mode,
 * pitch, side levels, source level and harmony are re-read at bind time.
 * Returns physical transient voices accepted into the finite pool. */
uint8_t apsis_string_rhythm_trigger_event(ApsisStringTransientPool *pool,
                                          const ApsisState *state,
                                          ApsisStringPitchMemory *pitch_memory,
                                          const ApsisTransportEvent *event);

#endif
