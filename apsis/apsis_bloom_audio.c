#include "apsis_bloom_audio.h"

static void bloom_audio_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint32_t bloom_audio_ms_samples(uint32_t ms)
{
    /* Exact 44.1 kHz integer conversion without 64-bit division helpers. */
    return 44u * ms + (100u * ms + 500u) / 1000u;
}

static uint16_t bloom_audio_level_q15(uint16_t level_q15)
{
    const uint32_t max_level = ((uint32_t)APSIS_Q15_ONE * 5u + 2u) / 4u;
    return level_q15 > max_level ? (uint16_t)max_level : level_q15;
}

static int16_t bloom_audio_pan(uint16_t width_q15, uint8_t index, uint8_t count)
{
    int32_t num, scaled, mag;
    uint16_t width = width_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : width_q15;
    uint8_t den;
    if (count <= 1u) return 0;
    den = (uint8_t)(count - 1u);
    num = (int32_t)(2u * index) - (int32_t)den;
    scaled = (int32_t)width * num;
    mag = scaled < 0 ? -scaled : scaled;
    mag = (mag + den / 2u) / den;
    return (int16_t)(scaled < 0 ? -mag : mag);
}

void apsis_bloom_audio_plan(const ApsisState *state,
                            uint8_t frozen_voice_count,
                            ApsisBloomAcousticPlan *out)
{
    uint32_t length_ms, unfurl_ms, unfurl_samples, level;
    uint8_t count, i, den;

    if (!out) return;
    bloom_audio_zero(out, (uint32_t)sizeof(*out));
    if (!state || !frozen_voice_count) return;

    count = frozen_voice_count > APSIS_BLOOM_VOICES_MAX
        ? APSIS_BLOOM_VOICES_MAX : frozen_voice_count;
    length_ms = state->bloom.length_ms;
    if (length_ms < 650u) length_ms = 650u;
    if (length_ms > 6000u) length_ms = 6000u;
    unfurl_ms = state->bloom.unfurl_ms;
    if (unfurl_ms > 3600u) unfurl_ms = 3600u;
    unfurl_samples = bloom_audio_ms_samples(unfurl_ms);
    level = bloom_audio_level_q15(state->bloom.level_q15);
    den = count > 1u ? (uint8_t)(count - 1u) : 1u;

    out->valid = 1u;
    out->count = count;
    out->unfurl_samples = unfurl_samples;

    for (i = 0u; i < count; ++i) {
        ApsisBloomPetalAcoustic *p = &out->petal[i];
        uint32_t factor_milli = 1000u - 65u * i;
        uint32_t duration_ms = length_ms + 60u * i;

        p->target_offset_samples = count > 1u
            ? (unfurl_samples * i + den / 2u) / den : 0u;
        p->duration_samples = bloom_audio_ms_samples(duration_ms);
        /* Source: 0.010 * sourceLevel * (1 - 0.065*i). The direct rational
         * keeps the source law exact relative to the state's Q15 level. */
        p->amp_q15 = (uint16_t)((level * factor_milli + 50000u) / 100000u);
        p->pan_q15 = bloom_audio_pan(state->bloom.width_q15, i, count);
    }
}
