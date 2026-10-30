#ifndef APSIS_BLOOM_AUDIO_H
#define APSIS_BLOOM_AUDIO_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_bloom.h"

#define APSIS_BLOOM_AUDIO_RATE 44100u

typedef struct {
    uint32_t target_offset_samples;
    uint32_t duration_samples;
    uint16_t amp_q15;
    int16_t pan_q15;
} ApsisBloomPetalAcoustic;

typedef struct {
    uint8_t valid;
    uint8_t count;
    uint32_t unfurl_samples;
    ApsisBloomPetalAcoustic petal[APSIS_BLOOM_VOICES_MAX];
} ApsisBloomAcousticPlan;

/* Pure Study-126 occurrence-time acoustic geometry.
 *
 * frozen_voice_count comes from the occurrence-frozen M3c Bloom plan/slot.
 * This function intentionally does NOT resolve pitch, voice topology, attack
 * or brightness; those are per-petal bind-time data in Study 126. */
void apsis_bloom_audio_plan(const ApsisState *state,
                            uint8_t frozen_voice_count,
                            ApsisBloomAcousticPlan *out);

#endif
