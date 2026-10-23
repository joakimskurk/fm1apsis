#ifndef APSIS_STRING_H
#define APSIS_STRING_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_STRING_SPLIT_MIN_Q15 APSIS_Q15_MILLI(150)
#define APSIS_STRING_SPLIT_MID_Q15 APSIS_Q15_MILLI(500)
#define APSIS_STRING_SPLIT_MAX_Q15 APSIS_Q15_MILLI(850)
#define APSIS_STRING_HYSTERESIS_MILLI 100

typedef struct {
    uint32_t raw_ratio_q16;
    uint32_t effective_ratio_q16;
    uint8_t numerator;
    uint8_t denominator;
    uint8_t near_simple;
} ApsisStringRatio;

typedef struct {
    int16_t left_target_step;
    int16_t right_target_step;
    uint8_t left_valid;
    uint8_t right_valid;
} ApsisStringPitchMemory;

void apsis_string_pitch_memory_reset(ApsisStringPitchMemory *memory);
void apsis_string_set_side_split(ApsisStringState *string, uint8_t side, uint16_t split_q15, uint8_t mirror);
ApsisStringRatio apsis_string_ratio(uint16_t split_q15);
int32_t apsis_string_magnitude_milli(uint16_t split_q15, uint16_t span_q15);
int32_t apsis_string_side_degree(const ApsisState *state, uint8_t side);
int32_t apsis_string_anchor_step(const ApsisState *state, uint8_t side);
int32_t apsis_string_raw_step_milli(const ApsisState *state, uint8_t side);
int32_t apsis_string_target_step_milli(const ApsisState *state, uint8_t side,
                                       ApsisStringPitchMemory *memory, uint8_t stable);

#endif
