#ifndef APSIS_BLOOM_H
#define APSIS_BLOOM_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_BLOOM_VOICES_MAX 5u

typedef struct {
    uint8_t tonic;
    uint8_t scale;
    uint8_t degree;
    uint8_t voicing;
    uint8_t inversion;
    uint8_t voices;
} ApsisBloomHarmony;

typedef struct {
    uint8_t count;
    uint8_t tone[APSIS_BLOOM_VOICES_MAX];
    int8_t octave[APSIS_BLOOM_VOICES_MAX];
} ApsisBloomPlan;

typedef struct {
    ApsisBloomPlan plan;
    int16_t last_steps[APSIS_BLOOM_VOICES_MAX];
    uint8_t last_count;
    int16_t drift_milli;
    uint32_t serial;
} ApsisBloomVoicingState;

void apsis_bloom_harmony_from_state(const ApsisState *state, ApsisBloomHarmony *harmony);
uint8_t apsis_bloom_voice_count(const ApsisState *state, const ApsisBloomHarmony *harmony);
uint8_t apsis_bloom_base_chord_steps(const ApsisState *state, const ApsisBloomHarmony *harmony,
                                     int16_t out[APSIS_BLOOM_VOICES_MAX]);
ApsisBloomPlan apsis_bloom_legacy_plan(const ApsisState *state, const ApsisBloomHarmony *harmony);
uint8_t apsis_bloom_plan_steps(const ApsisState *state, const ApsisBloomHarmony *harmony,
                               const ApsisBloomPlan *plan, int16_t out[APSIS_BLOOM_VOICES_MAX]);
uint16_t apsis_bloom_spread_target_q15(const ApsisState *state, const ApsisBloomHarmony *harmony);
void apsis_bloom_voicing_reset(ApsisBloomVoicingState *voice_state);
ApsisBloomPlan apsis_bloom_choose_voicing(const ApsisState *state, const ApsisBloomHarmony *harmony,
                                          ApsisBloomVoicingState *voice_state, uint8_t advance,
                                          uint32_t walk_random, uint32_t pick_random);
uint8_t apsis_bloom_chord_steps(const ApsisState *state, const ApsisBloomHarmony *harmony,
                                const ApsisBloomVoicingState *voice_state,
                                int16_t out[APSIS_BLOOM_VOICES_MAX]);

#endif
