#ifndef APSIS_HARMONY_H
#define APSIS_HARMONY_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_SCALE_DEGREES 7u
#define APSIS_GRAVITY_VOICES_MAX 5u

const int8_t *apsis_scale_intervals(uint8_t scale_id);
int32_t apsis_scale_at(uint8_t tonic, uint8_t scale_id, int32_t index);
int32_t apsis_quantize_step_to_scale(int32_t step, uint8_t tonic, uint8_t scale_id, uint8_t scale_lock);
uint8_t apsis_gravity_ring_intervals(const ApsisGravityState *gravity, int16_t out[APSIS_GRAVITY_VOICES_MAX]);
uint8_t apsis_gravity_chord_steps(const ApsisGravityState *gravity, int16_t out[APSIS_GRAVITY_VOICES_MAX]);
int32_t apsis_tuning_cents_milli(uint8_t pitch_class, uint8_t tonic, uint8_t mode, uint16_t strength_q15);
int32_t apsis_tuning_deviation_milli(uint8_t pitch_class, uint8_t tonic, uint8_t mode, uint16_t strength_q15);

#endif
