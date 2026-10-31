#include "apsis_controls_bloom.h"

static uint32_t p9e_bloom_button_bit(uint8_t button)
{
    return 1u << button;
}

static void p9e_bloom_control_clear(ApsisBloomControlFrame *out)
{
    uint8_t *p;
    uint32_t n;
    if (!out) return;
    p = (uint8_t *)out;
    n = (uint32_t)sizeof(*out);
    while (n--) *p++ = 0u;
}

static int32_t p9e_bloom_clamp_i32(int64_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return (int32_t)v;
}

static uint16_t p9e_bloom_clamp_q15(int64_t v, uint16_t lo, uint16_t hi)
{
    if (v < (int64_t)lo) return lo;
    if (v > (int64_t)hi) return hi;
    return (uint16_t)v;
}

static uint16_t p9e_bloom_threshold_q15(uint8_t voices, uint8_t index)
{
    uint32_t milli;
    if (voices <= 1u) return APSIS_Q15_MILLI(220);
    if (voices > APSIS_BLOOM_VOICES_MAX) voices = APSIS_BLOOM_VOICES_MAX;
    if (index >= voices) index = (uint8_t)(voices - 1u);
    milli = 140u + ((uint32_t)index * 720u + (uint32_t)(voices - 1u) / 2u) /
                   (uint32_t)(voices - 1u);
    return APSIS_Q15_MILLI(milli);
}

uint8_t apsis_controls_bloom_frame(const ApsisState *state,
                                   const ApsisFm1InputFrame *input,
                                   ApsisBloomControlFrame *out)
{
    ApsisBloomHarmony harmony;
    uint32_t rec;
    uint8_t voices, key;
    uint8_t active = 0u;

    if (!out) return 0u;
    p9e_bloom_control_clear(out);
    if (!state || !input) return 0u;

    rec = p9e_bloom_button_bit(APSIS_FM1_BTN_REC);
    out->grow_begin = (input->buttons_pressed & rec) ? 1u : 0u;
    out->grow_end = (input->buttons_released & rec) ? 1u : 0u;
    if (out->grow_begin || out->grow_end) active = 1u;

    /* REC owns the Bloom performance gesture while held. Note edges are
     * suppressed during the gesture so grow cannot accidentally emit a core or
     * petal hit. P9H feeds grow_detents into the transient gesture state below. */
    if (input->buttons_held & rec) {
        out->grow_detents = input->encoder_steps[APSIS_FM1_ENC_SELECT];
        if (out->grow_detents) active = 1u;
        return active;
    }
    if (out->grow_end) return active;

    if (input->notes_pressed & (1u << APSIS_BLOOM_CORE_KEY)) {
        out->trigger = APSIS_BLOOM_CONTROL_TRIGGER_CORE;
        return 1u;
    }

    apsis_bloom_harmony_from_state(state, &harmony);
    voices = apsis_bloom_voice_count(state, &harmony);
    if (voices > APSIS_BLOOM_VOICES_MAX) voices = APSIS_BLOOM_VOICES_MAX;
    for (key = 0u; key < voices; ++key) {
        if (input->notes_pressed & (1u << key)) {
            out->trigger = APSIS_BLOOM_CONTROL_TRIGGER_PETAL;
            out->petal_index = key;
            return 1u;
        }
    }
    return active;
}

void apsis_controls_bloom_grow_reset(ApsisBloomGrowGesture *gesture)
{
    if (!gesture) return;
    gesture->open_q15 = 0u;
    gesture->fired_mask = 0u;
}

uint8_t apsis_controls_bloom_grow_update(ApsisBloomGrowGesture *gesture,
                                         uint8_t voices,
                                         int32_t select_detents)
{
    uint8_t i, newly = 0u;
    int64_t next;
    if (!gesture) return 0u;
    if (voices < 1u) voices = 1u;
    if (voices > APSIS_BLOOM_VOICES_MAX) voices = APSIS_BLOOM_VOICES_MAX;

    next = (int64_t)gesture->open_q15 +
           (int64_t)select_detents * APSIS_BLOOM_GROW_STEP_Q15;
    gesture->open_q15 = p9e_bloom_clamp_q15(next, 0u, APSIS_Q15_ONE);

    for (i = 0u; i < voices; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((gesture->fired_mask & bit) == 0u &&
            gesture->open_q15 >= p9e_bloom_threshold_q15(voices, i)) {
            gesture->fired_mask |= bit;
            newly |= bit;
        }
    }
    return newly;
}

uint8_t apsis_controls_bloom_param_edits(const ApsisBloomState *bloom,
                                         const ApsisFm1InputFrame *input,
                                         ApsisBloomParamEdit out[4])
{
    uint8_t count = 0u;
    int32_t d;
    if (!bloom || !input || !out) return 0u;

    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) {
        out[count].param = APSIS_BLOOM_PARAM_UNFURL;
        out[count++].value = p9e_bloom_clamp_i32((int64_t)bloom->unfurl_ms + (int64_t)d * 10, 0, 3600);
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) {
        out[count].param = APSIS_BLOOM_PARAM_SPREAD;
        out[count++].value = p9e_bloom_clamp_q15((int64_t)bloom->voicing_spread_q15 +
                                                 (int64_t)d * APSIS_Q15_MILLI(10),
                                                 0u, APSIS_Q15_ONE);
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) {
        out[count].param = APSIS_BLOOM_PARAM_LENGTH;
        out[count++].value = p9e_bloom_clamp_i32((int64_t)bloom->length_ms + (int64_t)d * 10, 650, 6000);
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K4];
    if (d) {
        out[count].param = APSIS_BLOOM_PARAM_BRIGHTNESS;
        out[count++].value = p9e_bloom_clamp_q15((int64_t)bloom->brightness_q15 +
                                                 (int64_t)d * APSIS_Q15_MILLI(10),
                                                 APSIS_Q15_MILLI(120), APSIS_Q15_ONE);
    }
    return count;
}
