#ifndef APSIS_SPIRAL_H
#define APSIS_SPIRAL_H

#include <stdint.h>
#include "apsis_state.h"

uint8_t apsis_spiral_sanitize_route(ApsisSpiralState *spiral);
uint8_t apsis_spiral_derived_sequence(const ApsisState *state,
                                      int16_t out[APSIS_SPIRAL_ROUTE_MAX]);
uint8_t apsis_spiral_attractor_length(uint8_t length);
void apsis_spiral_reset_attractor(ApsisSpiralState *spiral, uint8_t regenerate);

#endif
