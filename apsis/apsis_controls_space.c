#include "apsis_controls_space.h"

#define P9G2_RETURN_MAX_Q15 APSIS_Q15_MILLI(720)
#define P9G2_FEEDBACK_MAX_Q15 APSIS_Q15_MILLI(580)
#define P9G2_Q15_STEP APSIS_Q15_MILLI(10)

static int32_t p9g2_clamp_i32(int64_t value, int32_t lo, int32_t hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return (int32_t)value;
}

static uint16_t p9g2_source_send(const ApsisState *state, uint8_t source)
{
    if (!state) return 0u;
    switch (source) {
    case APSIS_MOTHER_SOURCE_STRING: return state->string.send_q15;
    case APSIS_MOTHER_SOURCE_DRIFT: return state->drift.send_q15;
    case APSIS_MOTHER_SOURCE_CLOCK: return state->clock.send_q15;
    case APSIS_MOTHER_SOURCE_SPIRAL: return state->spiral.send_q15;
    case APSIS_MOTHER_SOURCE_BLOOM: return state->bloom.send_q15;
    default: return 0u;
    }
}

static uint8_t p9g2_emit(ApsisSpaceControlEdit out[4], uint8_t count,
                         uint8_t param, uint8_t source, int32_t value)
{
    if (count >= 4u) return count;
    out[count].param = param;
    out[count].source = source;
    out[count].value = value;
    return (uint8_t)(count + 1u);
}

void apsis_controls_space_reset(ApsisSpaceControlState *control)
{
    if (!control) return;
    control->page = APSIS_SPACE_CONTROL_ROOM;
    control->source = APSIS_MOTHER_SOURCE_STRING;
}

uint8_t apsis_controls_space_page_step(ApsisSpaceControlState *control,
                                       int32_t detents)
{
    int32_t page;
    if (!control || !detents) return 0u;
    page = control->page < APSIS_SPACE_CONTROL_PAGE_COUNT ? control->page : 0;
    page = (page + (detents % APSIS_SPACE_CONTROL_PAGE_COUNT)) % APSIS_SPACE_CONTROL_PAGE_COUNT;
    if (page < 0) page += APSIS_SPACE_CONTROL_PAGE_COUNT;
    control->page = (uint8_t)page;
    return 1u;
}

uint8_t apsis_controls_space_source_step(ApsisSpaceControlState *control,
                                         int32_t detents)
{
    int32_t source;
    if (!control || !detents || control->page != APSIS_SPACE_CONTROL_ECHO_SEND) return 0u;
    source = control->source < APSIS_MOTHER_SOURCE_COUNT ? control->source : 0;
    source = (source + (detents % APSIS_MOTHER_SOURCE_COUNT)) % APSIS_MOTHER_SOURCE_COUNT;
    if (source < 0) source += APSIS_MOTHER_SOURCE_COUNT;
    control->source = (uint8_t)source;
    return 1u;
}

uint8_t apsis_controls_space_edits(const ApsisState *state,
                                   const ApsisSpaceControlState *control,
                                   const ApsisFm1InputFrame *input,
                                   ApsisSpaceControlEdit out[4])
{
    uint8_t count = 0u, source;
    int32_t d;
    if (!state || !control || !input || !out) return 0u;

    if (control->page == APSIS_SPACE_CONTROL_ROOM) {
        d = input->encoder_steps[APSIS_FM1_ENC_K1];
        if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_RETURN, 0u,
            p9g2_clamp_i32((int64_t)state->space.mix_q15 + (int64_t)d * P9G2_Q15_STEP,
                           0, P9G2_RETURN_MAX_Q15));
        d = input->encoder_steps[APSIS_FM1_ENC_K2];
        if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_DECAY_MS, 0u,
            p9g2_clamp_i32((int64_t)state->space.decay_ms + (int64_t)d * 100,
                           4000, 12000));
        d = input->encoder_steps[APSIS_FM1_ENC_K3];
        if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_PREDELAY_MS, 0u,
            p9g2_clamp_i32((int64_t)state->space.predelay_ms + d, 15, 140));
        d = input->encoder_steps[APSIS_FM1_ENC_K4];
        if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_DAMPING_HZ, 0u,
            p9g2_clamp_i32((int64_t)state->space.damping_hz + (int64_t)d * 100,
                           2200, 11000));
        return count;
    }

    if (control->page != APSIS_SPACE_CONTROL_ECHO_SEND) return 0u;
    source = control->source < APSIS_MOTHER_SOURCE_COUNT ? control->source : 0u;
    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_ECHO_TIME_MS, 0u,
        p9g2_clamp_i32((int64_t)state->space.echo_time_ms + d, 80, 1200));
    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_ECHO_FEEDBACK, 0u,
        p9g2_clamp_i32((int64_t)state->space.echo_feedback_q15 + (int64_t)d * P9G2_Q15_STEP,
                       0, P9G2_FEEDBACK_MAX_Q15));
    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) count = p9g2_emit(out, count, APSIS_SPACE_PARAM_SOURCE_SEND, source,
        p9g2_clamp_i32((int64_t)p9g2_source_send(state, source) + (int64_t)d * P9G2_Q15_STEP,
                       0, APSIS_Q15_ONE));
    return count;
}
