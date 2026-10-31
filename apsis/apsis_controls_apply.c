#include "apsis_controls_apply.h"

static int32_t p9h_clamp_i32(int32_t value, int32_t lo, int32_t hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static uint16_t p9h_clamp_u16(int32_t value, uint16_t lo, uint16_t hi)
{
    if (value < (int32_t)lo) return lo;
    if (value > (int32_t)hi) return hi;
    return (uint16_t)value;
}

static uint16_t p9h_span_value(int32_t value)
{
    static const uint16_t spans[4] = {
        APSIS_Q15_MILLI(720), APSIS_Q15_ONE,
        APSIS_Q15_MILLI(1280), APSIS_Q15_MILLI(1560)
    };
    uint8_t i, best = 0u;
    int32_t best_error = 0x7fffffff;
    for (i = 0u; i < 4u; ++i) {
        int32_t error = value - (int32_t)spans[i];
        if (error < 0) error = -error;
        if (error < best_error) {
            best_error = error;
            best = i;
        }
    }
    return spans[best];
}

static uint8_t p9h_key_valid(uint16_t key)
{
    switch (key) {
    case APSIS_P9_PARAM_STRING_RESONANCE:
    case APSIS_P9_PARAM_STRING_BRIGHTNESS:
    case APSIS_P9_PARAM_STRING_GLIDE:
    case APSIS_P9_PARAM_STRING_SPAN:
    case APSIS_P9_PARAM_BLOOM_UNFURL:
    case APSIS_P9_PARAM_BLOOM_SPREAD:
    case APSIS_P9_PARAM_BLOOM_LENGTH:
    case APSIS_P9_PARAM_BLOOM_BRIGHTNESS:
    case APSIS_P9_PARAM_SPACE_RETURN:
    case APSIS_P9_PARAM_SPACE_DECAY_MS:
    case APSIS_P9_PARAM_SPACE_PREDELAY_MS:
    case APSIS_P9_PARAM_SPACE_DAMPING_HZ:
    case APSIS_P9_PARAM_SPACE_ECHO_TIME_MS:
    case APSIS_P9_PARAM_SPACE_ECHO_FEEDBACK:
    case APSIS_P9_PARAM_SEND_STRING:
    case APSIS_P9_PARAM_SEND_DRIFT:
    case APSIS_P9_PARAM_SEND_CLOCK:
    case APSIS_P9_PARAM_SEND_SPIRAL:
    case APSIS_P9_PARAM_SEND_BLOOM:
        return 1u;
    default:
        return 0u;
    }
}

static uint8_t p9h_apply_key(ApsisState *state, uint16_t key, int32_t value)
{
    if (!state) return 0u;
    switch (key) {
    case APSIS_P9_PARAM_STRING_RESONANCE:
        state->string.resonance_q15 = p9h_clamp_u16(
            value, APSIS_Q15_MILLI(300), APSIS_Q15_MILLI(880));
        return 1u;
    case APSIS_P9_PARAM_STRING_BRIGHTNESS:
        state->string.brightness_q15 = p9h_clamp_u16(
            value, APSIS_Q15_MILLI(200), APSIS_Q15_MILLI(820));
        return 1u;
    case APSIS_P9_PARAM_STRING_GLIDE:
        state->string.glide_q15 = p9h_clamp_u16(
            value, APSIS_Q15_MILLI(18), APSIS_Q15_MILLI(380));
        return 1u;
    case APSIS_P9_PARAM_STRING_SPAN:
        state->string.span_q15 = p9h_span_value(value);
        return 1u;

    case APSIS_P9_PARAM_BLOOM_UNFURL:
        state->bloom.unfurl_ms = (uint16_t)p9h_clamp_i32(value, 0, 3600);
        return 1u;
    case APSIS_P9_PARAM_BLOOM_SPREAD:
        state->bloom.voicing_spread_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    case APSIS_P9_PARAM_BLOOM_LENGTH:
        state->bloom.length_ms = (uint16_t)p9h_clamp_i32(value, 650, 6000);
        return 1u;
    case APSIS_P9_PARAM_BLOOM_BRIGHTNESS:
        state->bloom.brightness_q15 = p9h_clamp_u16(
            value, APSIS_Q15_MILLI(120), APSIS_Q15_ONE);
        return 1u;

    case APSIS_P9_PARAM_SPACE_RETURN:
        state->space.mix_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_MILLI(720));
        return 1u;
    case APSIS_P9_PARAM_SPACE_DECAY_MS:
        state->space.decay_ms = (uint16_t)p9h_clamp_i32(value, 4000, 12000);
        return 1u;
    case APSIS_P9_PARAM_SPACE_PREDELAY_MS:
        state->space.predelay_ms = (uint16_t)p9h_clamp_i32(value, 15, 140);
        return 1u;
    case APSIS_P9_PARAM_SPACE_DAMPING_HZ:
        state->space.damping_hz = (uint16_t)p9h_clamp_i32(value, 2200, 11000);
        return 1u;
    case APSIS_P9_PARAM_SPACE_ECHO_TIME_MS:
        state->space.echo_time_ms = (uint16_t)p9h_clamp_i32(value, 80, 1200);
        return 1u;
    case APSIS_P9_PARAM_SPACE_ECHO_FEEDBACK:
        state->space.echo_feedback_q15 = p9h_clamp_u16(
            value, 0u, APSIS_Q15_MILLI(580));
        return 1u;

    case APSIS_P9_PARAM_SEND_STRING:
        state->string.send_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    case APSIS_P9_PARAM_SEND_DRIFT:
        state->drift.send_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    case APSIS_P9_PARAM_SEND_CLOCK:
        state->clock.send_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    case APSIS_P9_PARAM_SEND_SPIRAL:
        state->spiral.send_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    case APSIS_P9_PARAM_SEND_BLOOM:
        state->bloom.send_q15 = p9h_clamp_u16(value, 0u, APSIS_Q15_ONE);
        return 1u;
    default:
        return 0u;
    }
}

uint8_t apsis_controls_apply_structural_param(ApsisState *state,
                                              ApsisMotherPending *pending,
                                              uint16_t key,
                                              int32_t value,
                                              int32_t last_abs_step)
{
    ApsisMotherPendingItem item;
    if (!state || !p9h_key_valid(key)) return 0u;
    if (apsis_mother_should_hold(&state->mother, pending,
                                 APSIS_MOTHER_DOMAIN_STRUCTURAL)) {
        if (!pending) return 0u;
        return apsis_mother_queue_param(pending, key, value, last_abs_step);
    }
    item.kind = APSIS_MOTHER_PENDING_PARAM;
    item.key = key;
    item.value = value;
    return apsis_controls_apply_pending_item(state, &item, 0);
}

uint8_t apsis_controls_apply_pending_item(ApsisState *state,
                                          const ApsisMotherPendingItem *item,
                                          void *user)
{
    (void)user;
    if (!state || !item || item->kind != APSIS_MOTHER_PENDING_PARAM ||
        !p9h_key_valid(item->key)) return 0u;
    return p9h_apply_key(state, item->key, item->value);
}

uint8_t apsis_controls_apply_string_edit(ApsisState *state,
                                         ApsisMotherPending *pending,
                                         const ApsisControlParamEdit *edit,
                                         int32_t last_abs_step)
{
    uint16_t key;
    if (!edit) return 0u;
    switch (edit->param) {
    case APSIS_CONTROL_PARAM_STRING_RESONANCE: key = APSIS_P9_PARAM_STRING_RESONANCE; break;
    case APSIS_CONTROL_PARAM_STRING_BRIGHTNESS: key = APSIS_P9_PARAM_STRING_BRIGHTNESS; break;
    case APSIS_CONTROL_PARAM_STRING_GLIDE: key = APSIS_P9_PARAM_STRING_GLIDE; break;
    case APSIS_CONTROL_PARAM_STRING_SPAN: key = APSIS_P9_PARAM_STRING_SPAN; break;
    default: return 0u;
    }
    return apsis_controls_apply_structural_param(state, pending, key,
                                                  edit->value, last_abs_step);
}

uint8_t apsis_controls_apply_bloom_edit(ApsisState *state,
                                        ApsisMotherPending *pending,
                                        const ApsisBloomParamEdit *edit,
                                        int32_t last_abs_step)
{
    uint16_t key;
    if (!edit) return 0u;
    switch (edit->param) {
    case APSIS_BLOOM_PARAM_UNFURL: key = APSIS_P9_PARAM_BLOOM_UNFURL; break;
    case APSIS_BLOOM_PARAM_SPREAD: key = APSIS_P9_PARAM_BLOOM_SPREAD; break;
    case APSIS_BLOOM_PARAM_LENGTH: key = APSIS_P9_PARAM_BLOOM_LENGTH; break;
    case APSIS_BLOOM_PARAM_BRIGHTNESS: key = APSIS_P9_PARAM_BLOOM_BRIGHTNESS; break;
    default: return 0u;
    }
    return apsis_controls_apply_structural_param(state, pending, key,
                                                  edit->value, last_abs_step);
}

uint8_t apsis_controls_apply_space_edit(ApsisState *state,
                                        ApsisMotherPending *pending,
                                        const ApsisSpaceControlEdit *edit,
                                        int32_t last_abs_step)
{
    uint16_t key;
    if (!edit) return 0u;
    switch (edit->param) {
    case APSIS_SPACE_PARAM_RETURN: key = APSIS_P9_PARAM_SPACE_RETURN; break;
    case APSIS_SPACE_PARAM_DECAY_MS: key = APSIS_P9_PARAM_SPACE_DECAY_MS; break;
    case APSIS_SPACE_PARAM_PREDELAY_MS: key = APSIS_P9_PARAM_SPACE_PREDELAY_MS; break;
    case APSIS_SPACE_PARAM_DAMPING_HZ: key = APSIS_P9_PARAM_SPACE_DAMPING_HZ; break;
    case APSIS_SPACE_PARAM_ECHO_TIME_MS: key = APSIS_P9_PARAM_SPACE_ECHO_TIME_MS; break;
    case APSIS_SPACE_PARAM_ECHO_FEEDBACK: key = APSIS_P9_PARAM_SPACE_ECHO_FEEDBACK; break;
    case APSIS_SPACE_PARAM_SOURCE_SEND:
        if (edit->source >= APSIS_MOTHER_SOURCE_COUNT) return 0u;
        key = (uint16_t)(APSIS_P9_PARAM_SEND_STRING + edit->source);
        break;
    default:
        return 0u;
    }
    return apsis_controls_apply_structural_param(state, pending, key,
                                                  edit->value, last_abs_step);
}
