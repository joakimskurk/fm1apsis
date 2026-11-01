#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "apsis_controls_drift.h"
#include "apsis_state.h"

static void reset(ApsisState *state, ApsisFm1InputFrame *input)
{
    apsis_state_default(state);
    memset(input, 0, sizeof(*input));
}

static void test_live_amount_and_tuning(void)
{
    ApsisState state;
    ApsisFm1InputFrame input;
    uint16_t start;
    reset(&state, &input);
    start = state.drift.amount_q15;

    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 2;
    assert(apsis_controls_drift_apply_live(&state, &input) == 1u);
    assert(state.drift.amount_q15 == (uint16_t)(start + 2u * APSIS_Q15_MILLI(10)));

    memset(&input, 0, sizeof input);
    input.notes_pressed = (1u << 2) | (1u << 0);
    assert(apsis_controls_drift_apply_live(&state, &input) == 1u);
    assert(state.temper.mode == APSIS_TUNING_PYTH);

    input.notes_pressed = 1u << 1;
    assert(apsis_controls_drift_apply_live(&state, &input) == 1u);
    assert(state.temper.mode == APSIS_TUNING_JUST);

    input.notes_pressed = 1u << 2;
    assert(apsis_controls_drift_apply_live(&state, &input) == 1u);
    assert(state.temper.mode == APSIS_TUNING_EQUAL);
}

static void test_live_amount_clamp(void)
{
    ApsisState state;
    ApsisFm1InputFrame input;
    reset(&state, &input);
    state.drift.amount_q15 = APSIS_Q15_ONE;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 100;
    assert(apsis_controls_drift_apply_live(&state, &input) == 0u);
    assert(state.drift.amount_q15 == APSIS_Q15_ONE);

    state.drift.amount_q15 = 0u;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = -100;
    assert(apsis_controls_drift_apply_live(&state, &input) == 0u);
    assert(state.drift.amount_q15 == 0u);
}

static void test_structural_param_intents(void)
{
    ApsisState state;
    ApsisFm1InputFrame input;
    ApsisDriftParamEdit edits[3];
    uint8_t n;
    reset(&state, &input);

    input.encoder_steps[APSIS_FM1_ENC_K1] = -2;
    input.encoder_steps[APSIS_FM1_ENC_K2] = 3;
    input.encoder_steps[APSIS_FM1_ENC_K3] = -4;
    n = apsis_controls_drift_param_edits(&state, &input, edits);
    assert(n == 3u);
    assert(edits[0].param == APSIS_DRIFT_PARAM_DEPTH);
    assert(edits[0].value == (int32_t)(state.drift.depth_q15 - 2u * APSIS_Q15_MILLI(10)));
    assert(edits[1].param == APSIS_DRIFT_PARAM_WIDTH);
    assert(edits[1].value == (int32_t)(state.drift.width_q15 + 3u * APSIS_Q15_MILLI(10)));
    assert(edits[2].param == APSIS_DRIFT_PARAM_TUNING_STRENGTH);
    assert(edits[2].value == (int32_t)(state.temper.strength_q15 - 4u * APSIS_Q15_MILLI(10)));

    state.drift.depth_q15 = APSIS_DRIFT_DEPTH_MIN_Q15;
    state.drift.width_q15 = APSIS_Q15_ONE;
    state.temper.strength_q15 = 0u;
    memset(&input, 0, sizeof input);
    input.encoder_steps[APSIS_FM1_ENC_K1] = -100;
    input.encoder_steps[APSIS_FM1_ENC_K2] = 100;
    input.encoder_steps[APSIS_FM1_ENC_K3] = -100;
    n = apsis_controls_drift_param_edits(&state, &input, edits);
    assert(n == 3u);
    assert(edits[0].value == APSIS_DRIFT_DEPTH_MIN_Q15);
    assert(edits[1].value == APSIS_Q15_ONE);
    assert(edits[2].value == 0);
}

int main(void)
{
    test_live_amount_and_tuning();
    test_live_amount_clamp();
    test_structural_param_intents();
    puts("PASS: P9G3 source-locked DRIFT/TUNING physical-control contract");
    return 0;
}
