#ifndef APSIS_STRESS_H
#define APSIS_STRESS_H

/* The audio's worst case, for measuring (docs/MARGIN_PLAN.md step A): every source on, each at the densest setting
 * the panel can reach (Clock 180 BPM in quarters of a pulse with every main/inner/snap step, the String's Turing
 * strum at every half pulse with every gate open and pizz rhythm on every pulse, Spiral at 4 ticks a pulse, Bloom
 * every cycle with five voices of ninths, the longest room). Only a diagnostic build applies it (APSIS_STRESS). */

#include "apsis_state.h"

void apsis_state_stress(ApsisState *s);

#endif
