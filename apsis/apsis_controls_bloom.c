#include "apsis_controls_bloom.h"

static uint32_t bloom_button_bit(uint8_t button)
{
    return 1u << button;
}

static void bloom_control_clear(ApsisBloomControlFrame *out)
{
    uint8_t *p;
    uint32_t n;
    if (!out) return;
    p = (uint8_t *)out;
    n = (uint32_t)sizeof(*out);
    while (n--) *p++ = 0u;
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
    bloom_control_clear(out);
    if (!state || !input) return 0u;

    rec = bloom_button_bit(APSIS_FM1_BTN_REC);
    out->grow_begin = (input->buttons_pressed & rec) ? 1u : 0u;
    out->grow_end = (input->buttons_released & rec) ? 1u : 0u;
    if (out->grow_begin || out->grow_end) active = 1u;

    /* REC owns the Bloom performance gesture while held. Do not let petal/core
     * note edges leak through during grow. Preserve the raw SELECT detent delta
     * so hardware sensitivity can be tuned later without changing music state. */
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
