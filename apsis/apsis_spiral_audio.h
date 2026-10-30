#ifndef APSIS_SPIRAL_AUDIO_H
#define APSIS_SPIRAL_AUDIO_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_SPIRAL_AUDIO_RATE 44100u
#define APSIS_SPIRAL_AUDIO_LEGS_MAX 2u

typedef struct {
    uint32_t start_offset_samples;
    uint32_t duration_samples;
    uint16_t gain_q15;
    int16_t pan_q15;
    int8_t drift_sign;
    uint8_t shadow;
} ApsisSpiralLegPlan;

typedef struct {
    uint8_t valid;
    uint8_t voice;
    uint8_t leg_count;
    uint16_t energy_q15;
    uint16_t drift_delta_millihz;
    uint16_t width_q15;
    uint32_t duration_samples;
    ApsisSpiralLegPlan leg[APSIS_SPIRAL_AUDIO_LEGS_MAX];
} ApsisSpiralAcousticPlan;

/* Pure Study-126 occurrence-time acoustic planning. interval_q32 is the full
 * current swung Clock interval in sample-Q32 and attractor_rate is 1/2/4.
 * Pitch is intentionally absent: M3c owns the later target-80 ms bind. */
void apsis_spiral_audio_plan(const ApsisState *state,
                             uint16_t energy_q15,
                             uint64_t interval_q32,
                             uint8_t attractor_rate,
                             ApsisSpiralAcousticPlan *out);

#endif
