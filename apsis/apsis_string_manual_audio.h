#ifndef APSIS_STRING_MANUAL_AUDIO_H
#define APSIS_STRING_MANUAL_AUDIO_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"

/* P9C maps the FM-1 keys onto Study-126 keyboardStringPress(), whose source
 * level is .080. Keep this P9-only so frozen M4b/M4f code size cannot move. */
#define APSIS_STRING_MANUAL_KEY_LEVEL_Q15 APSIS_Q15_MILLI(80)

uint8_t apsis_string_manual_trigger(ApsisStringTransientPool *pool,
                                    const ApsisState *state,
                                    ApsisStringPitchMemory *pitch_memory,
                                    uint8_t side);

#endif
