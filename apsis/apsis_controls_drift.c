#include "apsis_controls_drift.h"

static uint16_t p9g3_step_q15(uint16_t current, int32_t detents,
                              uint16_t step, uint16_t lo, uint16_t hi)
{
    int64_t v = (int64_t)current + (int64_t)detents * step;
    if (v < (int64_t)lo) return lo;
    if (v > (int64_t)hi) return hi;
    return (uint16_t)v;
}

uint8_t apsis_controls_drift_apply_live(ApsisState *state,
                                        const ApsisFm1InputFrame *input)
{
    int32_t d;
    uint8_t changed = 0u;
    if (!state || !input) return 0u;

    d = input->encoder_steps[APSIS_FM1_ENC_SELECT];
    if (d) {
        uint16_t next = p9g3_step_q15(state->drift.amount_q15, d,
                                     APSIS_DRIFT_AMOUNT_STEP_Q15,
                                     0u, APSIS_Q15_ONE);
        if (next != state->drift.amount_q15) {
            state->drift.amount_q15 = next;
            ++changed;
        }
    }

    /* Source station geometry is PYTH, JUST, EQUAL from left to right. Lowest
     * simultaneous note edge wins deterministically, matching other P9 maps. */
    if (input->notes_pressed & (1u << 0)) {
        if (state->temper.mode != APSIS_TUNING_PYTH) {
            state->temper.mode = APSIS_TUNING_PYTH;
            ++changed;
        }
    } else if (input->notes_pressed & (1u << 1)) {
        if (state->temper.mode != APSIS_TUNING_JUST) {
            state->temper.mode = APSIS_TUNING_JUST;
            ++changed;
        }
    } else if (input->notes_pressed & (1u << 2)) {
        if (state->temper.mode != APSIS_TUNING_EQUAL) {
            state->temper.mode = APSIS_TUNING_EQUAL;
            ++changed;
        }
    }
    return changed;
}

uint8_t apsis_controls_drift_param_edits(const ApsisState *state,
                                         const ApsisFm1InputFrame *input,
                                         ApsisDriftParamEdit out[3])
{
    uint8_t count = 0u;
    int32_t d;
    if (!state || !input || !out) return 0u;

    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) {
        out[count].param = APSIS_DRIFT_PARAM_DEPTH;
        out[count++].value = p9g3_step_q15(state->drift.depth_q15, d,
                                           APSIS_DRIFT_PARAM_STEP_Q15,
                                           APSIS_DRIFT_DEPTH_MIN_Q15,
                                           APSIS_Q15_ONE);
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) {
        out[count].param = APSIS_DRIFT_PARAM_WIDTH;
        out[count++].value = p9g3_step_q15(state->drift.width_q15, d,
                                           APSIS_DRIFT_PARAM_STEP_Q15,
                                           0u, APSIS_Q15_ONE);
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) {
        out[count].param = APSIS_DRIFT_PARAM_TUNING_STRENGTH;
        out[count++].value = p9g3_step_q15(state->temper.strength_q15, d,
                                           APSIS_DRIFT_PARAM_STEP_Q15,
                                           0u, APSIS_Q15_ONE);
    }
    return count;
}
