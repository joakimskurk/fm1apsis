#include "apsis_controls_mother.h"

static uint32_t p9g1_button_bit(uint8_t button)
{
    return 1u << button;
}

static uint16_t p9g1_mix_step(uint16_t current, int32_t detents)
{
    int64_t v = (int64_t)current + (int64_t)detents * APSIS_MOTHER_MIX_STEP_Q15;
    if (v < 0) return 0u;
    if (v > APSIS_MOTHER_MIX_MAX_Q15) return APSIS_MOTHER_MIX_MAX_Q15;
    return (uint16_t)v;
}

static uint8_t p9g1_source_sound(const ApsisState *state, uint8_t source)
{
    if (!state) return 0u;
    switch (source) {
    case APSIS_MOTHER_SOURCE_STRING: return state->string.sound ? 1u : 0u;
    case APSIS_MOTHER_SOURCE_DRIFT: return state->drift.sound ? 1u : 0u;
    case APSIS_MOTHER_SOURCE_CLOCK: return state->clock.sound ? 1u : 0u;
    case APSIS_MOTHER_SOURCE_SPIRAL: return state->spiral.sound ? 1u : 0u;
    case APSIS_MOTHER_SOURCE_BLOOM: return state->bloom.sound ? 1u : 0u;
    default: return 0u;
    }
}

static void p9g1_set_source_sound(ApsisState *state, uint8_t source, uint8_t on)
{
    if (!state) return;
    switch (source) {
    case APSIS_MOTHER_SOURCE_STRING: state->string.sound = on; break;
    case APSIS_MOTHER_SOURCE_DRIFT: state->drift.sound = on; break;
    case APSIS_MOTHER_SOURCE_CLOCK: state->clock.sound = on; break;
    case APSIS_MOTHER_SOURCE_SPIRAL: state->spiral.sound = on; break;
    case APSIS_MOTHER_SOURCE_BLOOM: state->bloom.sound = on; break;
    default: break;
    }
}

uint8_t apsis_controls_mother_mix_edits(const ApsisMotherState *mother,
                                        const ApsisFm1InputFrame *input,
                                        ApsisMotherMixEdit out[APSIS_MOTHER_SOURCE_COUNT])
{
    static const uint8_t encoder_for_source[APSIS_MOTHER_SOURCE_COUNT] = {
        APSIS_FM1_ENC_K1, APSIS_FM1_ENC_K2, APSIS_FM1_ENC_K3,
        APSIS_FM1_ENC_K4, APSIS_FM1_ENC_SELECT
    };
    uint8_t source, count = 0u;
    if (!mother || !input || !out) return 0u;
    for (source = 0u; source < APSIS_MOTHER_SOURCE_COUNT; ++source) {
        int32_t d = input->encoder_steps[encoder_for_source[source]];
        if (!d) continue;
        out[count].source = source;
        out[count].value_q15 = p9g1_mix_step(mother->mix_q15[source], d);
        ++count;
    }
    return count;
}

uint8_t apsis_controls_mother_apply_mix(ApsisMotherState *mother,
                                        const ApsisMotherMixEdit *edit)
{
    if (!mother || !edit || edit->source >= APSIS_MOTHER_SOURCE_COUNT) return 0u;
    mother->mix_q15[edit->source] = edit->value_q15 > APSIS_MOTHER_MIX_MAX_Q15
        ? APSIS_MOTHER_MIX_MAX_Q15 : edit->value_q15;
    return 1u;
}

uint8_t apsis_controls_mother_mute_source_from_fm1(const ApsisFm1InputFrame *input,
                                                   uint8_t *source_out)
{
    static const uint8_t button_for_source[APSIS_MOTHER_SOURCE_COUNT] = {
        APSIS_FM1_BTN_ENV, APSIS_FM1_BTN_LFO, APSIS_FM1_BTN_SEQ,
        APSIS_FM1_BTN_ARP, APSIS_FM1_BTN_FX
    };
    uint8_t source;
    if (!input || !source_out) return 0u;
    if (!(input->buttons_held & p9g1_button_bit(APSIS_FM1_BTN_SEL))) return 0u;
    for (source = 0u; source < APSIS_MOTHER_SOURCE_COUNT; ++source) {
        if (input->buttons_pressed & p9g1_button_bit(button_for_source[source])) {
            *source_out = source;
            return 1u;
        }
    }
    return 0u;
}

uint8_t apsis_controls_mother_toggle_source_mute(ApsisState *state,
                                                 uint8_t source)
{
    uint8_t on, bit;
    if (!state || source >= APSIS_MOTHER_SOURCE_COUNT) return 0u;
    on = p9g1_source_sound(state, source);
    p9g1_set_source_sound(state, source, on ? 0u : 1u);
    bit = (uint8_t)(1u << source);
    if (on) state->mother.mute_mask |= bit;
    else state->mother.mute_mask &= (uint8_t)~bit;
    return 1u;
}

uint8_t apsis_controls_mother_sync_from_fm1(ApsisMotherState *mother,
                                            ApsisMotherPending *pending,
                                            const ApsisFm1InputFrame *input)
{
    if (!mother || !pending || !input) return 0u;
    if (!(input->buttons_pressed & p9g1_button_bit(APSIS_FM1_BTN_REC))) return 0u;
    apsis_mother_set_sync(mother, pending, mother->sync ? 0u : 1u);
    return 1u;
}

uint8_t apsis_controls_mother_horizon_from_fm1(ApsisMotherState *mother,
                                               ApsisMotherPending *pending,
                                               const ApsisFm1InputFrame *input,
                                               int32_t last_abs_step)
{
    static const uint8_t horizons[6] = { 1u, 2u, 4u, 8u, 16u, 32u };
    uint32_t down_bit, up_bit;
    uint8_t i, index = 3u;
    int32_t next;
    if (!mother || !pending || !input) return 0u;
    down_bit = p9g1_button_bit(APSIS_FM1_BTN_OCT_DOWN);
    up_bit = p9g1_button_bit(APSIS_FM1_BTN_OCT_UP);
    if (!!(input->buttons_pressed & down_bit) == !!(input->buttons_pressed & up_bit))
        return 0u;
    for (i = 0u; i < 6u; ++i) if (horizons[i] == mother->horizon) { index = i; break; }
    next = (int32_t)index + ((input->buttons_pressed & up_bit) ? 1 : -1);
    if (next < 0) next = 5;
    if (next > 5) next = 0;
    apsis_mother_set_horizon(mother, pending, horizons[(uint8_t)next], last_abs_step);
    return 1u;
}
