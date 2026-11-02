#ifndef APSIS_STRING98_H
#define APSIS_STRING98_H

/* The finite String sounds of Study 98 (old_index.html), the String's acoustic authority: one string, every note
 * a classicStringNote() (3-4 additive partials, optionally a root shadow), into the shared transient pool.
 *   stringPluck(position)      apsis_string98_pluck
 *   stringRhythm(when, mode)   apsis_string98_rhythm   (bow / pizz / motion)
 *   turingStrum(value, pos)    apsis_string98_strum    (Study 126 calls the Turing register "Attractor")
 * Study 98's CLASSIC character only: hybrid and bowed (stringSolo's body/noise path) are not ported yet. */

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string_attractor.h"
#include "apsis_transport.h"

#define APSIS_STRING98_PLUCK_LEVEL_Q15 APSIS_Q15_MILLI(82)   /* keyboardStringStrum(level=.082): the keys */
#define APSIS_STRING98_PLUCK_GUARD_SAMPLES 1544u              /* t - lastStringPluck < .035 s: ignored */

/* tensionFrequency(): the string's current pitch */
uint32_t apsis_string98_tension_millihz(const ApsisState *s, ApsisStringPitchMemory *memory);

/* now_sample: the runtime's sample cursor, for the 35 ms guard between plucks */
uint8_t apsis_string98_pluck(ApsisStringTransientPool *pool, const ApsisState *s,
                             ApsisStringPitchMemory *memory, uint64_t now_sample);

uint8_t apsis_string98_rhythm(ApsisStringTransientPool *pool, const ApsisState *s,
                              ApsisStringPitchMemory *memory, const ApsisTransportEvent *event);

/* render_origin_sample: the block's first sample (the strum starts at its bound target inside the block) */
uint8_t apsis_string98_strum(ApsisStringTransientPool *pool, const ApsisState *s,
                             ApsisStringPitchMemory *memory, const ApsisStringAttractorBinding *binding,
                             uint64_t render_origin_sample);

#endif
