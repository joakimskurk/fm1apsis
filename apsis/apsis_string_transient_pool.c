/* M4b keeps the finite transient implementation mechanically separate from
 * the frozen M4a sustain struct. The original Study-98 Classic/pizz engine is
 * included unchanged and compiled against ApsisStringTransientPool. M4b2 then
 * layers Study-126 BODY voices into the same physical/logical pool so there is
 * still one global finite-voice ceiling and one steal policy. */
#include "apsis_string_dsp.h"

#define ApsisStringDsp ApsisStringTransientPool
#define apsis_string_transient_render strx_classic_transient_render
#include "apsis_string_transient.c"
#undef apsis_string_transient_render
#undef ApsisStringDsp

#define STRX_BODY_HOLD_SAMPLES 3087u /* round(44.1k * 70 ms) */
#define STRX_BODY_ENV_ONE (64u << 16)
#define STRX_BODY_PAN_LIMIT_Q15 18022 /* 0.55 */

static const uint16_t STRX_BODY_COEFF_Q15[7] = {
    19919u, 6175u, 2988u, 1693u, 1036u, 598u, 359u
};

/* One-pole low-pass coefficient 1-exp(-2*pi*f/44100), 1300..2500 Hz in
 * 100-Hz steps. This is the native low-cost counterpart to Study-126's
 * brightness LP and intentionally reuses the M4a BODY oscillator identity. */
static const uint16_t STRX_BODY_LP_Q15[13] = {
    5540u,5926u,6305u,6680u,7049u,7412u,7771u,
    8125u,8473u,8817u,9156u,9490u,9819u
};

void apsis_string_transient_pool_reset(ApsisStringTransientPool *pool)
{
    if (!pool) return;
    strx_zero(pool, (uint32_t)sizeof(*pool));
}

static uint16_t strx_body_lp_alpha(uint16_t brightness_q15)
{
    uint32_t b = brightness_q15;
    uint32_t hz, pos, rem;
    uint16_t a, z;
    if (b < APSIS_Q15_MILLI(150)) b = APSIS_Q15_MILLI(150);
    if (b > APSIS_Q15_MILLI(750)) b = APSIS_Q15_MILLI(750);
    hz = 1100u + (b * 1800u + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    if (hz <= 1300u) return STRX_BODY_LP_Q15[0];
    if (hz >= 2500u) return STRX_BODY_LP_Q15[12];
    pos = (hz - 1300u) / 100u;
    rem = (hz - 1300u) - pos * 100u;
    a = STRX_BODY_LP_Q15[pos];
    z = STRX_BODY_LP_Q15[pos + 1u];
    return (uint16_t)(a + ((uint32_t)(z - a) * rem + 50u) / 100u);
}

static int16_t strx_body_q15(uint32_t phase)
{
    int64_t sum = 0;
    uint8_t h;
    for (h = 1u; h <= 7u; ++h) {
        uint32_t hp = phase * (uint32_t)h + 0x40000000u;
        sum += (int64_t)strx_sine_q15(hp) * STRX_BODY_COEFF_Q15[h - 1u];
    }
    sum >>= 15;
    if (sum > 32767) sum = 32767;
    if (sum < -32767) sum = -32767;
    return (int16_t)sum;
}

uint8_t apsis_string_dsp_trigger_classic_offset(ApsisStringTransientPool *pool,
                                                const ApsisState *state,
                                                uint32_t base_freq_millihz,
                                                uint16_t level_q15,
                                                uint32_t duration_ms,
                                                int16_t pan_q15,
                                                uint8_t pizz,
                                                uint16_t root_gain_q15,
                                                uint32_t start_offset_samples)
{
    ApsisStringTransientPlan plan;
    uint8_t i, accepted = 0u;
    if (!pool || !state) return 0u;
    apsis_string_dsp_transient_plan(state, base_freq_millihz, level_q15,
                                    duration_ms, pan_q15, pizz, root_gain_q15,
                                    &plan);
    pool->transient_trigger_count++;
    for (i = 0u; i < plan.count; ++i) {
        uint32_t start = plan.partial[i].start_offset_samples;
        if (start > 0xFFFFFFFFu - start_offset_samples)
            start = 0xFFFFFFFFu;
        else
            start += start_offset_samples;
        plan.partial[i].start_offset_samples = start;
        accepted = (uint8_t)(accepted + strx_allocate(pool, &plan.partial[i]));
    }
    return accepted;
}

static uint8_t strx_allocate_body(ApsisStringTransientPool *pool,
                                  const ApsisState *state,
                                  uint32_t base_freq_millihz,
                                  uint16_t level_q15,
                                  uint32_t duration_ms,
                                  int16_t pan_q15,
                                  uint16_t brightness_q15,
                                  uint32_t attack_ms,
                                  uint32_t start_offset_samples)
{
    int32_t free_ix;
    ApsisStringTransientVoice *v;
    uint16_t resonance;
    uint32_t attack_samples, hold_samples, decay_samples;

    free_ix = strx_find_free(pool);
    if (free_ix < 0) {
        pool->transient_drop_count++;
        return 0u;
    }
    if (apsis_string_dsp_transient_logical_active(pool) >= APSIS_STRING_TRANSIENT_LOGICAL_CAP)
        (void)strx_steal_oldest(pool);

    if (base_freq_millihz < 55000u) base_freq_millihz = 55000u;
    if (base_freq_millihz > 1250000u) base_freq_millihz = 1250000u;
    if (duration_ms < 250u) duration_ms = 250u;
    if (duration_ms > 3000u) duration_ms = 3000u;
    if (attack_ms < 18u) attack_ms = 18u;
    if (attack_ms > 220u) attack_ms = 220u;
    if (pan_q15 < -STRX_BODY_PAN_LIMIT_Q15) pan_q15 = -STRX_BODY_PAN_LIMIT_Q15;
    if (pan_q15 > STRX_BODY_PAN_LIMIT_Q15) pan_q15 = STRX_BODY_PAN_LIMIT_Q15;

    resonance = state->string.resonance_q15;
    if (resonance < APSIS_Q15_MILLI(250)) resonance = APSIS_Q15_MILLI(250);
    if (resonance > APSIS_Q15_MILLI(780)) resonance = APSIS_Q15_MILLI(780);

    v = &pool->transient[free_ix];
    strx_zero(v, (uint32_t)sizeof(*v));
    v->active = 1u;
    v->waveform = APSIS_STRING_DSP_WAVE_BODY;
    v->envelope_stage = 2u;
    v->serial = ++pool->transient_next_serial;
    v->phase_inc = strx_phase_inc_from_millihz(base_freq_millihz);
    v->start_delay_samples = start_offset_samples;
    v->duration_samples = strx_samples_for_ms(duration_ms);
    v->peak_gain_q15 = strx_mul_q15(level_q15, resonance);
    strx_pan_mix(pan_q15, &v->mix_left_q15, &v->mix_right_q15);
    v->lowpass_alpha_q15 = strx_body_lp_alpha(brightness_q15);
    v->steal_gain_q15 = APSIS_Q15_ONE;

    attack_samples = strx_samples_for_ms(attack_ms);
    if (attack_samples > v->duration_samples) attack_samples = v->duration_samples;
    if (!attack_samples) attack_samples = 1u;
    v->attack_left_samples = (uint16_t)attack_samples;
    v->attack_step_q16 = (STRX_BODY_ENV_ONE + attack_samples / 2u) / attack_samples;
    if (!v->attack_step_q16) v->attack_step_q16 = 1u;

    hold_samples = STRX_BODY_HOLD_SAMPLES;
    if (attack_samples + hold_samples > v->duration_samples)
        hold_samples = v->duration_samples > attack_samples
            ? v->duration_samples - attack_samples : 0u;
    v->body_hold_left_samples = (uint16_t)hold_samples;
    decay_samples = v->duration_samples - attack_samples - hold_samples;
    if (!decay_samples) decay_samples = 1u;
    v->decay_step_q16 = (STRX_BODY_ENV_ONE + decay_samples / 2u) / decay_samples;
    if (!v->decay_step_q16) v->decay_step_q16 = 1u;

    strx_update_high_water(pool);
    return 1u;
}

uint8_t apsis_string_dsp_trigger_body(ApsisStringTransientPool *pool,
                                      const ApsisState *state,
                                      uint32_t base_freq_millihz,
                                      uint16_t level_q15,
                                      uint32_t duration_ms,
                                      int16_t pan_q15,
                                      uint16_t brightness_q15,
                                      uint32_t attack_ms,
                                      uint32_t start_offset_samples)
{
    if (!pool || !state) return 0u;
    pool->transient_trigger_count++;
    return strx_allocate_body(pool, state, base_freq_millihz, level_q15,
                              duration_ms, pan_q15, brightness_q15, attack_ms,
                              start_offset_samples);
}

static uint16_t strx_body_envelope(ApsisStringTransientVoice *v)
{
    uint32_t idx;
    if (v->attack_left_samples) {
        if (v->env_phase_q16 + v->attack_step_q16 >= STRX_BODY_ENV_ONE)
            v->env_phase_q16 = STRX_BODY_ENV_ONE;
        else
            v->env_phase_q16 += v->attack_step_q16;
        --v->attack_left_samples;
        if (!v->attack_left_samples) v->env_phase_q16 = STRX_BODY_ENV_ONE;
        idx = v->env_phase_q16 >> 7;
        return (uint16_t)(idx > 32767u ? 32767u : idx);
    }
    if (v->body_hold_left_samples) {
        --v->body_hold_left_samples;
        v->env_phase_q16 = STRX_BODY_ENV_ONE;
        return 32767u;
    }
    idx = v->env_phase_q16 >> 16;
    if (idx > 64u) idx = 64u;
    if (v->env_phase_q16 > v->decay_step_q16)
        v->env_phase_q16 -= v->decay_step_q16;
    else
        v->env_phase_q16 = 0u;
    return STRX_ENV_Q15[idx];
}

static void strx_mask_body(ApsisStringTransientPool *pool)
{
    uint16_t i;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool->transient[i];
        if (v->active && v->waveform == APSIS_STRING_DSP_WAVE_BODY) {
            v->active = 0u;
            v->envelope_stage |= 0x80u;
        }
    }
}

static void strx_restore_body(ApsisStringTransientPool *pool)
{
    uint16_t i;
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool->transient[i];
        if (v->waveform == APSIS_STRING_DSP_WAVE_BODY && (v->envelope_stage & 0x80u)) {
            v->envelope_stage &= 0x7Fu;
            v->active = 1u;
        }
    }
}

static void strx_body_render(ApsisStringTransientPool *pool, int32_t *out,
                             uint32_t frames)
{
    uint32_t n;
    uint16_t i;
    for (n = 0u; n < frames; ++n) {
        int64_t mix_l = 0, mix_r = 0;
        for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
            ApsisStringTransientVoice *v = &pool->transient[i];
            int32_t raw, filtered, voiced;
            uint16_t env, gain;
            if (!v->active || v->waveform != APSIS_STRING_DSP_WAVE_BODY) continue;
            if (v->start_delay_samples) {
                --v->start_delay_samples;
                continue;
            }
            if (v->age_samples >= v->duration_samples) {
                v->active = 0u;
                continue;
            }
            v->phase += v->phase_inc;
            raw = strx_body_q15(v->phase);
            v->lowpass_q15 += (int32_t)(((int64_t)(raw - v->lowpass_q15) *
                                           v->lowpass_alpha_q15) >> 15);
            filtered = v->lowpass_q15;
            env = strx_body_envelope(v);
            gain = strx_mul_q15(v->peak_gain_q15, env);
            if (v->stealing)
                gain = strx_mul_q15(gain, (uint16_t)v->steal_gain_q15);
            voiced = (int32_t)(((int64_t)filtered * gain) >> 15);
            mix_l += ((int64_t)voiced * v->mix_left_q15) >> 15;
            mix_r += ((int64_t)voiced * v->mix_right_q15) >> 15;
            ++v->age_samples;
            strx_advance_steal(v);
            if (v->age_samples >= v->duration_samples) v->active = 0u;
        }
        out[n * 2u] = strx_sat_q15((int64_t)out[n * 2u] + mix_l);
        out[n * 2u + 1u] = strx_sat_q15((int64_t)out[n * 2u + 1u] + mix_r);
    }
}

void apsis_string_transient_render(ApsisStringTransientPool *pool, int32_t *out,
                                   uint32_t frames)
{
    if (!pool || !out) return;
    /* The already-proven M4b1 renderer must never see BODY as a sine. Mask
     * those slots for its pass, restore them, then run the BODY pass. */
    strx_mask_body(pool);
    strx_classic_transient_render(pool, out, frames);
    strx_restore_body(pool);
    strx_body_render(pool, out, frames);
}
