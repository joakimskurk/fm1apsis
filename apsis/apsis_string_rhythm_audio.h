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

/* Study-126 direct/manual String gesture. This deliberately reuses the same
 * native BODY/WIRE/composite transient topology already frozen for M4b rather
 * than creating a second performance synth. Debounce/timing belongs to the
 * physical-control handoff; this function only emits the acoustic transient.
 * Returns physical transient voices accepted into the finite pool. */
uint8_t apsis_string_manual_trigger(ApsisStringTransientPool *pool,
                                    const ApsisState *state,
                                    ApsisStringPitchMemory *pitch_memory,
                                    uint8_t side);

#endif
