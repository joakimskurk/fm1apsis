#include "apsis_string_attractor_audio.h"

#define ATTR_AUDIO_ATTACK_SAMPLES 706u       /* round(44.1k * 16 ms) */
#define ATTR_AUDIO_ENV_ONE (64u << 16)
#define ATTR_AUDIO_STEAL_SAMPLES 4630u       /* frozen M4b 105 ms tail */
#define ATTR_AUDIO_ACCENT_DURATION_SAMPLES 7779u /* round(44.1k * 176.4 ms) */
#define ATTR_AUDIO_4MS_SAMPLES 176u
#define ATTR_AUDIO_6MS_SAMPLES 265u
#define ATTR_AUDIO_ACCENT_PAN_Q15 7078       /* +0.216 */
/* Frozen M4b equal-power pan-table interpolation at +0.216. */
#define ATTR_AUDIO_ACCENT_MIX_L_Q15 18918u
#define ATTR_AUDIO_ACCENT_MIX_R_Q15 26742u

static uint16_t attr_audio_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t attr_audio_brightness_left(uint16_t value)
{
    int32_t v = (int32_t)value - (int32_t)APSIS_Q15_MILLI(40);
    if (v < (int32_t)APSIS_Q15_MILLI(150)) v = APSIS_Q15_MILLI(150);
    /* stringSolo() clamps its explicit bowed brightness to .75. */
    if (v > (int32_t)APSIS_Q15_MILLI(750)) v = APSIS_Q15_MILLI(750);
    return (uint16_t)v;
}

static uint32_t attr_audio_frequency(const ApsisState *s,
                                     ApsisStringPitchMemory *memory,
                                     uint8_t side)
{
    int32_t step = apsis_string_target_step_milli(s, side, memory, 1u);
    return apsis_string_dsp_frequency_millihz(s, step);
}

static uint32_t attr_audio_start_delay(uint64_t target, uint64_t render_origin)
{
    uint64_t delta;
    if (target <= render_origin) return 0u;
    delta = target - render_origin;
    return delta > 0xffffffffu ? 0xffffffffu : (uint32_t)delta;
}

static int32_t attr_audio_find_free(const ApsisStringTransientPool *pool)
{
    uint16_t i;
    if (!pool) return -1;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
        if (!pool->transient[i].active) return (int32_t)i;
    return -1;
}

static uint8_t attr_audio_steal_oldest(ApsisStringTransientPool *pool)
{
    uint16_t i;
    int32_t oldest = -1;
    uint32_t serial = 0xffffffffu;
    if (!pool) return 0u;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool->transient[i];
        if (v->active && !v->stealing && v->serial < serial) {
            serial = v->serial;
            oldest = (int32_t)i;
        }
    }
    if (oldest < 0) return 0u;
    if (pool->transient[oldest].start_delay_samples) {
        pool->transient[oldest].active = 0u;
    } else {
        pool->transient[oldest].stealing = 1u;
        pool->transient[oldest].steal_gain_q15 = APSIS_Q15_ONE;
        pool->transient[oldest].steal_left_samples = ATTR_AUDIO_STEAL_SAMPLES;
    }
    pool->transient_steal_count++;
    return 1u;
}

static void attr_audio_update_high_water(ApsisStringTransientPool *pool)
{
    uint16_t logical, physical;
    if (!pool) return;
    logical = apsis_string_dsp_transient_logical_active(pool);
    physical = apsis_string_dsp_transient_physical_active(pool);
    if (logical > pool->transient_logical_high_water)
        pool->transient_logical_high_water = logical;
    if (physical > pool->transient_physical_high_water)
        pool->transient_physical_high_water = physical;
}

/* One generic Study-126 tone(...,'sine','string') accent, using the frozen
 * M4b finite-pool lifecycle. This intentionally duplicates only the tiny
 * allocator shell because M4b's strx_allocate() is translation-unit private. */
static uint8_t attr_audio_trigger_accent(ApsisStringTransientPool *pool,
                                         uint32_t freq_millihz,
                                         uint16_t level_q15,
                                         uint32_t start_offset_samples)
{
    ApsisStringTransientVoice *v;
    int32_t free_ix;
    uint32_t decay_samples;
    if (!pool || !level_q15) return 0u;
    free_ix = attr_audio_find_free(pool);
    if (free_ix < 0) {
        pool->transient_drop_count++;
        return 0u;
    }
    if (apsis_string_dsp_transient_logical_active(pool) >= APSIS_STRING_TRANSIENT_LOGICAL_CAP)
        (void)attr_audio_steal_oldest(pool);

    v = &pool->transient[free_ix];
    {
        uint8_t *p = (uint8_t *)v;
        uint32_t n = (uint32_t)sizeof(*v);
        while (n--) *p++ = 0u;
    }
    v->active = 1u;
    v->serial = ++pool->transient_next_serial;
    v->phase_inc = (uint32_t)(((uint64_t)freq_millihz * 6382653u + 32768u) >> 16);
    v->start_delay_samples = start_offset_samples;
    v->duration_samples = ATTR_AUDIO_ACCENT_DURATION_SAMPLES;
    v->peak_gain_q15 = level_q15;
    v->mix_left_q15 = ATTR_AUDIO_ACCENT_MIX_L_Q15;
    v->mix_right_q15 = ATTR_AUDIO_ACCENT_MIX_R_Q15;
    v->lowpass_alpha_q15 = 0u; /* source sine bypasses tone() low-pass */
    v->waveform = APSIS_STRING_DSP_WAVE_SINE;
    v->steal_gain_q15 = APSIS_Q15_ONE;
    v->attack_left_samples = ATTR_AUDIO_ATTACK_SAMPLES;
    decay_samples = v->duration_samples - v->attack_left_samples;
    if (!decay_samples) decay_samples = 1u;
    v->decay_step_q16 = (ATTR_AUDIO_ENV_ONE + decay_samples / 2u) / decay_samples;
    if (!v->decay_step_q16) v->decay_step_q16 = 1u;
    pool->transient_trigger_count++;
    attr_audio_update_high_water(pool);
    return 1u;
}

static uint8_t attr_audio_body(ApsisStringTransientPool *pool,
                               const ApsisState *s,
                               uint32_t freq,
                               uint16_t level,
                               uint32_t start)
{
    /* Study 126 passes rootGain=.08 into stringSolo(), but the bowed stringSolo
     * implementation never reads opts.rootGain. BODY therefore has no root
     * shadow in the actual sounding source. */
    return apsis_string_dsp_trigger_body(pool, s, freq, level, 720u,
        (int16_t)-APSIS_Q15_MILLI(300), attr_audio_brightness_left(s->string.brightness_q15),
        30u, start);
}

static uint8_t attr_audio_wire(ApsisStringTransientPool *pool,
                               const ApsisState *s,
                               uint32_t freq,
                               uint16_t input_level,
                               uint16_t norm,
                               uint32_t start)
{
    uint16_t wire_level = attr_audio_mul_q15(input_level, APSIS_Q15_MILLI(920));
    uint16_t root_gain = s->string.foundation ? APSIS_Q15_MILLI(50) : 0u;
    uint8_t accepted = 0u;

    /* stringSideTransient() computes/passes a +.24 brightness option, but
     * classicStringNote() ignores opts.brightness and reads state.string.
     * Keep current base brightness here to match the actually sounding code. */
    accepted = (uint8_t)(accepted + apsis_string_dsp_trigger_classic_offset(
        pool, s, freq, wire_level, 420u, (int16_t)APSIS_Q15_MILLI(300),
        1u, root_gain, start));

    if (norm > APSIS_Q15_MILLI(520)) {
        uint32_t accent_start = start;
        uint32_t accent_freq = freq > 0x7fffffffu ? 0xffffffffu : freq * 2u;
        uint16_t accent_level = attr_audio_mul_q15(input_level, APSIS_Q15_MILLI(150));
        if (accent_start > 0xffffffffu - ATTR_AUDIO_6MS_SAMPLES)
            accent_start = 0xffffffffu;
        else
            accent_start += ATTR_AUDIO_6MS_SAMPLES;
        accepted = (uint8_t)(accepted + attr_audio_trigger_accent(
            pool, accent_freq, accent_level, accent_start));
    }
    return accepted;
}

uint8_t apsis_string_attractor_trigger_binding(
    ApsisStringTransientPool *pool,
    const ApsisState *s,
    ApsisStringPitchMemory *pitch_memory,
    const ApsisStringAttractorBinding *binding,
    uint64_t render_origin_sample)
{
    uint16_t level, side_level;
    uint32_t freq, start;
    uint8_t accepted = 0u;
    if (!pool || !s || !binding || !binding->valid) return 0u;
    /* Source fire() re-checks sourceOn('string') and source level at bind. */
    if (!s->string.sound || !s->string.level_q15) return 0u;

    level = attr_audio_mul_q15(binding->base_level_q15, s->string.level_q15);
    if (binding->both_scale)
        level = attr_audio_mul_q15(level, APSIS_Q15_MILLI(780));
    start = attr_audio_start_delay(binding->target_sample, render_origin_sample);

    if (binding->route == APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC) {
        freq = attr_audio_frequency(s, pitch_memory, APSIS_SIDE_RIGHT);
        accepted = (uint8_t)(accepted + attr_audio_body(
            pool, s, freq, attr_audio_mul_q15(level, APSIS_Q15_MILLI(660)), start));
        if (start > 0xffffffffu - ATTR_AUDIO_4MS_SAMPLES)
            start = 0xffffffffu;
        else
            start += ATTR_AUDIO_4MS_SAMPLES;
        accepted = (uint8_t)(accepted + attr_audio_wire(
            pool, s, freq, attr_audio_mul_q15(level, APSIS_Q15_MILLI(780)),
            binding->value_q15, start));
        return accepted;
    }

    if (binding->route == APSIS_STRING_ATTRACTOR_ROUTE_LEFT) {
        side_level = s->string.left_level_q15;
        level = attr_audio_mul_q15(level, side_level);
        freq = attr_audio_frequency(s, pitch_memory, APSIS_SIDE_LEFT);
        return attr_audio_body(pool, s, freq, level, start);
    }

    if (binding->route == APSIS_STRING_ATTRACTOR_ROUTE_RIGHT) {
        side_level = s->string.right_level_q15;
        level = attr_audio_mul_q15(level, side_level);
        freq = attr_audio_frequency(s, pitch_memory, APSIS_SIDE_RIGHT);
        return attr_audio_wire(pool, s, freq, level, binding->value_q15, start);
    }
    return 0u;
}
