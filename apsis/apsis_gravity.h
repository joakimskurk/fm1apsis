#ifndef APSIS_GRAVITY_H
#define APSIS_GRAVITY_H

#include <stdint.h>
#include "apsis_state.h"

uint8_t apsis_gravity_point_index(uint8_t degree, uint8_t ring);
ApsisGravityPoint apsis_gravity_point_default(uint8_t degree, uint8_t ring);
ApsisGravityPoint *apsis_gravity_point(ApsisGravityState *gravity, uint8_t degree, uint8_t ring);
const ApsisGravityPoint *apsis_gravity_point_const(const ApsisGravityState *gravity, uint8_t degree, uint8_t ring);
void apsis_gravity_sanitize_point(ApsisGravityPoint *point);
void apsis_gravity_reset_point(ApsisGravityState *gravity, uint8_t degree, uint8_t ring);
int32_t apsis_gravity_point_step(const ApsisState *state, uint8_t degree, uint8_t ring);

#endif
