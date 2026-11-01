#include "apsis_string_manual_audio.h"

static uint16_t manual_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t manual_brightness_add(uint16_t value, int32_t milli)
{
    int32_t v = (int32_t)value;
    int32_t delta = (int32_t)APSIS_Q15_MILLI(milli < 0 ? -milli : milli);
    v += milli < 0 ? -delta : delta;
    if (v < (int32_t)APSIS_Q15_MILLI(150)) v = APSIS_Q15_MILLI(150);
    if (v > (int32_t)APSIS_Q15_MILLI(880)) v = APSIS_Q15_MILLI(880);
    return (uint16_t)v;
}

static uint32_t manual_side_frequency(const ApsisState *s,
                                      ApsisStringPitchMemory *memory,
                                      uint8_t side)
{
    int32_t step = apsis_string_target_step_milli(s, side, memory, 1u);
    return apsis_string_dsp_frequency_millihz(s, step);
}

static uint8_t manual_body(ApsisStringTransientPool *pool,
                           const ApsisState *s,
                           uint32_t freq,
                           uint16_t level,
                           uint16_t brightness,
                           uint32_t start_offset_samples)
{
    return apsis_string_dsp_trigger_body(pool, s, freq, level, 1180u,
                                         (int16_t)-APSIS_Q15_MILLI(300),
                                         brightness, 48u,
                                         start_offset_samples);
}

static uint8_t manual_wire(ApsisStringTransientPool *pool,
                           const ApsisState *s,
                           uint32_t freq,
                           uint16_t level,
                           uint16_t brightness,
                           uint32_t start_offset_samples)
{
    ApsisState local = *s;
    uint16_t root_gain = s->string.foundation ? APSIS_Q15_MILLI(50) : 0u;
    local.string.brightness_q15 = brightness;
    /* Study-126 stringSideTransient(right) applies .92 before classicStringNote. */
    level = manual_mul_q15(level, APSIS_Q15_MILLI(920));
    return apsis_string_dsp_trigger_classic_offset(pool, &local, freq, level,
                                                    660u,
                                                    (int16_t)APSIS_Q15_MILLI(300),
                                                    1u, root_gain,
                                                    start_offset_samples);
}

static uint8_t manual_classic(ApsisStringTransientPool *pool,
                              const ApsisState *s,
                              ApsisStringPitchMemory *memory,
                              uint16_t level)
{
    uint32_t freq = manual_side_frequency(s, memory, APSIS_SIDE_RIGHT);
    uint16_t body_level = manual_mul_q15(level, APSIS_Q15_MILLI(660));
    uint16_t wire_level = manual_mul_q15(level, APSIS_Q15_MILLI(780));
    uint16_t body_brightness = manual_brightness_add(s->string.brightness_q15, -100);
    uint16_t wire_brightness = manual_brightness_add(s->string.brightness_q15, 180);
    uint8_t accepted = 0u;

    accepted = (uint8_t)(accepted + manual_body(pool, s, freq, body_level,
                                                body_brightness, 0u));
    /* Study-126 stringCompositeTransient() delays WIRE by 4 ms. */
    accepted = (uint8_t)(accepted + manual_wire(pool, s, freq, wire_level,
                                                wire_brightness, 176u));
    return accepted;
}

uint8_t apsis_string_manual_trigger(ApsisStringTransientPool *pool,
                                    const ApsisState *s,
                                    ApsisStringPitchMemory *pitch_memory,
                                    uint8_t side)
{
    uint16_t level, side_level, voice_level;
    uint32_t freq;

    if (!pool || !s || !pitch_memory || !s->string.sound ||
        !s->string.level_q15)
        return 0u;

    level = manual_mul_q15(APSIS_STRING_MANUAL_KEY_LEVEL_Q15,
                           s->string.level_q15);
    if (!level) return 0u;

    if (s->string.surface_mode == APSIS_STRING_CLASSIC)
        return manual_classic(pool, s, pitch_memory, level);

    side = side == APSIS_SIDE_LEFT ? APSIS_SIDE_LEFT : APSIS_SIDE_RIGHT;
    side_level = side == APSIS_SIDE_LEFT
        ? s->string.left_level_q15 : s->string.right_level_q15;
    voice_level = manual_mul_q15(level, side_level);
    if (!voice_level) return 0u;

    freq = manual_side_frequency(s, pitch_memory, side);
    if (side == APSIS_SIDE_LEFT)
        return manual_body(pool, s, freq, voice_level,
                           manual_brightness_add(s->string.brightness_q15, -100),
                           0u);
    return manual_wire(pool, s, freq, voice_level,
                       manual_brightness_add(s->string.brightness_q15, 180),
                       0u);
}
