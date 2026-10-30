#include "apsis_bloom_dsp.h"
#include "apsis_string_dsp.h"

#define BLOOM_Q20_ONE 1048576u
#define BLOOM_Q30_ONE 1073741824ll
#define BLOOM_Q31_ONE 0x7fffffffu
#define BLOOM_GAIN_FLOOR_Q15 3u
#define BLOOM_GAIN_MIN_PEAK_Q15 7u
#define BLOOM_GAIN_MIN_SUSTAIN_Q15 4u
#define BLOOM_FILTER_STATE_LIMIT 131071

static const int16_t BLOOM_SINE_QUARTER[65] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static void bloom_dsp_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static int32_t bloom_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static int32_t bloom_filter_state(int64_t v)
{
    if (v > BLOOM_FILTER_STATE_LIMIT) return BLOOM_FILTER_STATE_LIMIT;
    if (v < -BLOOM_FILTER_STATE_LIMIT) return -BLOOM_FILTER_STATE_LIMIT;
    return (int32_t)v;
}

static uint32_t bloom_samples_ms(uint32_t ms)
{
    return 44u * ms + (100u * ms + 500u) / 1000u;
}

static uint16_t bloom_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t bloom_mul_q31(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + (1ull << 30)) >> 31);
}

static uint32_t bloom_pow_q31(uint32_t base, uint32_t exp)
{
    uint32_t out = BLOOM_Q31_ONE;
    while (exp) {
        if (exp & 1u) out = bloom_mul_q31(out, base);
        exp >>= 1u;
        if (exp) base = bloom_mul_q31(base, base);
    }
    return out;
}

static uint32_t bloom_nth_root_q31(uint32_t target, uint32_t n)
{
    uint32_t lo, hi, k;
    if (!n || target >= BLOOM_Q31_ONE) return BLOOM_Q31_ONE;
    if (!target) return 0u;
    lo = target;
    hi = BLOOM_Q31_ONE;
    for (k = 0u; k < 24u; ++k) {
        uint32_t mid = lo + ((hi - lo) >> 1u);
        uint32_t p = bloom_pow_q31(mid, n);
        if (p > target) hi = mid;
        else lo = mid;
    }
    return lo + ((hi - lo) >> 1u);
}

static int16_t bloom_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return BLOOM_SINE_QUARTER[o];
    if (q == 1u) return BLOOM_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-BLOOM_SINE_QUARTER[o];
    return (int16_t)-BLOOM_SINE_QUARTER[64u - o];
}

static int16_t bloom_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = bloom_sine_point(i);
    int32_t b = bloom_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static int16_t bloom_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}

static uint16_t bloom_quarter_sine_q15(uint16_t x_q15)
{
    uint32_t scaled = (uint32_t)x_q15 * 64u;
    uint32_t index = scaled / APSIS_Q15_ONE;
    uint32_t rem;
    int32_t a, b;
    if (index >= 64u) return 32767u;
    rem = scaled - index * APSIS_Q15_ONE;
    a = BLOOM_SINE_QUARTER[index];
    b = BLOOM_SINE_QUARTER[index + 1u];
    return (uint16_t)(a + ((int32_t)(b - a) * (int32_t)rem +
                           (int32_t)APSIS_Q15_ONE / 2) /
                          (int32_t)APSIS_Q15_ONE);
}

static void bloom_pan_q15(int16_t pan_q15, uint16_t *left, uint16_t *right)
{
    int32_t pan = pan_q15;
    uint16_t x;
    if (pan < -(int32_t)APSIS_Q15_ONE) pan = -(int32_t)APSIS_Q15_ONE;
    if (pan > (int32_t)APSIS_Q15_ONE) pan = APSIS_Q15_ONE;
    x = (uint16_t)(((uint32_t)(pan + (int32_t)APSIS_Q15_ONE) *
                    APSIS_Q15_ONE + APSIS_Q15_ONE) /
                   (2u * APSIS_Q15_ONE));
    if (right) *right = bloom_quarter_sine_q15(x);
    if (left) *left = bloom_quarter_sine_q15((uint16_t)(APSIS_Q15_ONE - x));
}

static uint32_t bloom_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static int32_t bloom_sin_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)hz * 97392u;
    return (int32_t)(((int64_t)bloom_sine_q15(phase) * BLOOM_Q30_ONE) / 32767ll);
}

static int32_t bloom_cos_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)hz * 97392u;
    return (int32_t)(((int64_t)bloom_sine_q15(phase + 0x40000000u) *
                      BLOOM_Q30_ONE) / 32767ll);
}

static int32_t bloom_norm_q30(int64_t value_q30, int64_t a0_q30)
{
    return (int32_t)((value_q30 * BLOOM_Q30_ONE) / a0_q30);
}

static void bloom_biquad_lowpass(ApsisBloomDspBiquad *b,
                                 uint16_t hz, uint16_t q_milli)
{
    int64_t s, c, alpha, a0, one_minus_c;
    uint16_t q = q_milli < 100u ? 100u : q_milli;
    if (!b) return;
    s = bloom_sin_q30_for_hz(hz);
    c = bloom_cos_q30_for_hz(hz);
    /* alpha = sin(w0)/(2Q), Q represented as milli-units. */
    alpha = (s * 500ll + q / 2u) / q;
    a0 = BLOOM_Q30_ONE + alpha;
    one_minus_c = BLOOM_Q30_ONE - c;
    b->b0_q30 = bloom_norm_q30(one_minus_c / 2ll, a0);
    b->b1_q30 = bloom_norm_q30(one_minus_c, a0);
    b->b2_q30 = b->b0_q30;
    b->a1_q30 = bloom_norm_q30(-2ll * c, a0);
    b->a2_q30 = bloom_norm_q30(BLOOM_Q30_ONE - alpha, a0);
    b->x1 = b->x2 = b->y1 = b->y2 = 0;
}

static int32_t bloom_biquad_process(ApsisBloomDspBiquad *b, int32_t x)
{
    int64_t acc;
    int32_t y;
    if (!b) return x;
    acc = (int64_t)b->b0_q30 * x + (int64_t)b->b1_q30 * b->x1 +
          (int64_t)b->b2_q30 * b->x2 - (int64_t)b->a1_q30 * b->y1 -
          (int64_t)b->a2_q30 * b->y2;
    y = bloom_filter_state(acc >> 30);
    b->x2 = b->x1;
    b->x1 = x;
    b->y2 = b->y1;
    b->y1 = y;
    return y;
}

static uint32_t bloom_detune_q20(int8_t cents)
{
    switch (cents) {
        case -6: return 1044948u;
        case -5: return 1045552u;
        case -4: return 1046156u;
        case -3: return 1046761u;
        case -2: return 1047365u;
        case -1: return 1047970u;
        case 1: return 1049182u;
        case 2: return 1049788u;
        case 3: return 1050395u;
        case 4: return 1051002u;
        case 5: return 1051609u;
        case 6: return 1052216u;
        default: return BLOOM_Q20_ONE;
    }
}

static uint32_t bloom_osc_freq(uint32_t base_mhz, uint8_t ratio, int8_t detune)
{
    uint64_t v = (uint64_t)base_mhz * (ratio ? ratio : 1u);
    v = (v * bloom_detune_q20(detune) + BLOOM_Q20_ONE / 2u) >> 20;
    return v > 0xffffffffull ? 0xffffffffu : (uint32_t)v;
}

static int32_t bloom_div100_round(int32_t v)
{
    return v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100);
}

static uint32_t bloom_base_frequency(const ApsisState *state,
                                     const ApsisBloomLateBinding *pitch)
{
    ApsisState equal;
    int32_t step_milli;
    if (!state || !pitch || !pitch->valid) return 0u;
    equal = *state;
    equal.gravity.scale_lock = 0u;
    equal.temper.mode = APSIS_TUNING_EQUAL;
    equal.temper.strength_q15 = 0u;
    step_milli = (int32_t)pitch->step * 1000 +
                 bloom_div100_round(pitch->tuning_deviation_milli);
    return apsis_string_dsp_frequency_millihz(&equal, step_milli);
}

static uint16_t bloom_voice_gain_scale(uint8_t voice)
{
    if (voice == APSIS_BLOOM_CHOIR) return APSIS_Q15_MILLI(780);
    if (voice == APSIS_BLOOM_ORGAN) return APSIS_Q15_MILLI(820);
    if (voice == APSIS_BLOOM_GLASS) return APSIS_Q15_MILLI(840);
    return APSIS_Q15_ONE;
}

static uint16_t bloom_voice_sustain(uint8_t voice)
{
    if (voice == APSIS_BLOOM_ORGAN) return APSIS_Q15_MILLI(820);
    if (voice == APSIS_BLOOM_CHOIR) return APSIS_Q15_MILLI(720);
    return APSIS_Q15_MILLI(660);
}

static uint32_t bloom_attack_samples(const ApsisBloomAudioBinding *b)
{
    uint32_t base;
    if (!b) return bloom_samples_ms(18u);
    base = bloom_samples_ms(b->attack_ms);
    if (b->voice == APSIS_BLOOM_CHOIR) {
        uint32_t v = (base * 18u + 5u) / 10u;
        uint32_t floor = bloom_samples_ms(45u);
        return v < floor ? floor : v;
    }
    if (b->voice == APSIS_BLOOM_ORGAN) {
        uint32_t floor = bloom_samples_ms(18u);
        return base < floor ? floor : base;
    }
    if (b->voice == APSIS_BLOOM_GLASS) {
        uint32_t v = (base * 72u + 50u) / 100u;
        uint32_t floor = bloom_samples_ms(6u);
        return v < floor ? floor : v;
    }
    return base;
}

static uint16_t bloom_cutoff_hz(uint8_t voice, uint16_t brightness_q15,
                                uint32_t freq_mhz)
{
    uint32_t base, span, hz, cutoff;
    uint16_t bright = brightness_q15 > APSIS_Q15_ONE
        ? APSIS_Q15_ONE : brightness_q15;
    if (voice == APSIS_BLOOM_CHOIR) {
        base = 1100u; span = 5600u;
    } else if (voice == APSIS_BLOOM_ORGAN) {
        base = 1500u; span = 6500u;
    } else if (voice == APSIS_BLOOM_GLASS) {
        base = 2200u; span = 8800u;
    } else {
        base = 1500u; span = 7800u;
    }
    base += (span * bright + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    hz = (freq_mhz + 500u) / 1000u;
    if (hz < 30u) hz = 30u;
    cutoff = base + (hz * 45u + 50u) / 100u;
    if (cutoff < 1000u) cutoff = 1000u;
    if (cutoff > 12000u) cutoff = 12000u;
    return (uint16_t)cutoff;
}

static uint16_t bloom_q_milli(uint8_t voice, uint16_t brightness_q15)
{
    if (voice == APSIS_BLOOM_ORGAN) return 420u;
    if (voice == APSIS_BLOOM_GLASS) return 240u;
    return (uint16_t)(300u +
        ((uint32_t)240u * brightness_q15 + APSIS_Q15_ONE / 2u) /
        APSIS_Q15_ONE);
}

static void bloom_add_osc(ApsisBloomDspPetal *v, uint8_t waveform,
                          uint8_t ratio, uint16_t gain_q15, int8_t detune,
                          uint32_t base_mhz)
{
    ApsisBloomDspOsc *o;
    uint32_t mhz;
    if (!v || v->osc_count >= APSIS_BLOOM_DSP_OSC_MAX) return;
    o = &v->osc[v->osc_count++];
    mhz = bloom_osc_freq(base_mhz, ratio, detune);
    o->phase_inc = bloom_phase_inc_from_millihz(mhz);
    o->gain_q15 = gain_q15;
    o->waveform = waveform;
}

static void bloom_voice_bank(ApsisBloomDspPetal *v, uint8_t voice,
                             uint8_t petal_index, uint32_t base_mhz)
{
    if (!v) return;
    if (voice == APSIS_BLOOM_CHOIR) {
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 1u, APSIS_Q15_MILLI(520), -6, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 1u, APSIS_Q15_MILLI(520), 6, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 2u, APSIS_Q15_MILLI(80), 0, base_mhz);
    } else if (voice == APSIS_BLOOM_ORGAN) {
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 1u, APSIS_Q15_MILLI(740), 0, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 2u, APSIS_Q15_MILLI(220), 0, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 3u, APSIS_Q15_MILLI(75), 0, base_mhz);
    } else if (voice == APSIS_BLOOM_GLASS) {
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 1u, APSIS_Q15_MILLI(820), 0, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 2u, APSIS_Q15_MILLI(180), 2, base_mhz);
        bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 4u, APSIS_Q15_MILLI(35), -2, base_mhz);
    } else {
        bloom_add_osc(v, petal_index == 0u ? APSIS_BLOOM_DSP_TRIANGLE
                                            : APSIS_BLOOM_DSP_SINE,
                      1u, APSIS_Q15_ONE, 0, base_mhz);
    }
}

static void bloom_sort_env_events(ApsisBloomDspPetal *v)
{
    uint8_t i;
    if (!v) return;
    for (i = 1u; i < 3u; ++i) {
        uint32_t t = v->env_event_sample[i];
        uint32_t g = v->env_event_gain_q23[i];
        uint8_t j = i;
        while (j && v->env_event_sample[j - 1u] > t) {
            v->env_event_sample[j] = v->env_event_sample[j - 1u];
            v->env_event_gain_q23[j] = v->env_event_gain_q23[j - 1u];
            --j;
        }
        v->env_event_sample[j] = t;
        v->env_event_gain_q23[j] = g;
    }
}

static void bloom_env_prepare(ApsisBloomDspPetal *v)
{
    uint32_t len, target;
    if (!v) return;
    while (v->env_event_index < 3u &&
           v->env_event_sample[v->env_event_index] <= v->age_samples) {
        v->env_gain_q23 = v->env_event_gain_q23[v->env_event_index];
        v->env_event_index++;
    }
    if (v->env_event_index >= 3u) {
        v->env_stage_left = 0u;
        return;
    }
    len = v->env_event_sample[v->env_event_index] - v->age_samples;
    target = v->env_event_gain_q23[v->env_event_index];
    v->env_stage_left = len;
    v->env_stage_target_q23 = target;
    v->env_linear_acc_q23 = 0u;
    v->env_linear_den = len ? len : 1u;
    if (target >= v->env_gain_q23) {
        uint32_t delta = target - v->env_gain_q23;
        v->env_stage_rising = 1u;
        v->env_linear_step_q23 = delta / v->env_linear_den;
        v->env_linear_rem_q23 = delta % v->env_linear_den;
        v->env_decay_mul_q31 = BLOOM_Q31_ONE;
    } else {
        uint32_t ratio;
        v->env_stage_rising = 0u;
        v->env_linear_step_q23 = 0u;
        v->env_linear_rem_q23 = 0u;
        ratio = v->env_gain_q23
            ? (uint32_t)(((uint64_t)target << 31) / v->env_gain_q23) : 0u;
        v->env_decay_mul_q31 = bloom_nth_root_q31(ratio, len);
    }
}

static void bloom_env_advance(ApsisBloomDspPetal *v)
{
    if (!v) return;
    if (v->env_stage_left) {
        if (v->env_stage_rising) {
            v->env_gain_q23 += v->env_linear_step_q23;
            v->env_linear_acc_q23 += v->env_linear_rem_q23;
            if (v->env_linear_acc_q23 >= v->env_linear_den) {
                v->env_gain_q23++;
                v->env_linear_acc_q23 -= v->env_linear_den;
            }
        } else {
            v->env_gain_q23 = (uint32_t)(((uint64_t)v->env_gain_q23 *
                                           v->env_decay_mul_q31 +
                                           (1ull << 30)) >> 31);
        }
        v->env_stage_left--;
    }
    v->age_samples++;
    if (!v->env_stage_left && v->env_event_index < 3u &&
        v->age_samples >= v->env_event_sample[v->env_event_index]) {
        v->env_gain_q23 = v->env_stage_target_q23;
        v->env_event_index++;
        bloom_env_prepare(v);
    }
}

static uint16_t bloom_active_count(const ApsisBloomDsp *dsp, uint16_t *osc)
{
    uint16_t active = 0u, oscs = 0u;
    uint16_t i;
    if (!dsp) {
        if (osc) *osc = 0u;
        return 0u;
    }
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i) {
        if (!dsp->petal[i].active) continue;
        active++;
        oscs = (uint16_t)(oscs + dsp->petal[i].osc_count);
    }
    if (osc) *osc = oscs;
    return active;
}

void apsis_bloom_dsp_reset(ApsisBloomDsp *dsp)
{
    if (!dsp) return;
    bloom_dsp_zero(dsp, (uint32_t)sizeof(*dsp));
}

uint8_t apsis_bloom_dsp_trigger_binding(ApsisBloomDsp *dsp,
                                        const ApsisState *state,
                                        const ApsisBloomAudioBinding *binding,
                                        uint64_t block_start)
{
    ApsisBloomDspPetal *v = 0;
    uint32_t base_mhz, attack, hold, duration, peak, sustain;
    uint16_t active, oscs, i;

    if (!dsp || !state || !binding || !binding->valid || !binding->pitch.valid)
        return 0u;

    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i) {
        if (!dsp->petal[i].active) {
            v = &dsp->petal[i];
            break;
        }
    }
    if (!v) {
        dsp->drop_count++;
        return 0u;
    }

    base_mhz = bloom_base_frequency(state, &binding->pitch);
    if (!base_mhz) {
        dsp->drop_count++;
        return 0u;
    }

    bloom_dsp_zero(v, (uint32_t)sizeof(*v));
    v->active = 1u;
    v->voice = binding->voice <= APSIS_BLOOM_GLASS
        ? binding->voice : APSIS_BLOOM_VEIL;
    v->petal_index = binding->petal_index;
    v->serial = ++dsp->next_serial;
    v->token = binding->token;
    if (binding->target_sample > block_start) {
        uint64_t delay = binding->target_sample - block_start;
        v->start_delay_samples = delay > 0xffffffffull ? 0xffffffffu : (uint32_t)delay;
    }

    duration = binding->acoustic.duration_samples;
    if (!duration) duration = bloom_samples_ms(650u);
    v->life_samples = duration + bloom_samples_ms(60u);
    attack = bloom_attack_samples(binding);
    hold = (duration * 48u + 50u) / 100u;
    if (hold > attack + bloom_samples_ms(420u))
        hold = attack + bloom_samples_ms(420u);

    peak = bloom_mul_q15(binding->acoustic.amp_q15,
                         bloom_voice_gain_scale(v->voice));
    if (peak < BLOOM_GAIN_MIN_PEAK_Q15) peak = BLOOM_GAIN_MIN_PEAK_Q15;
    sustain = bloom_mul_q15((uint16_t)peak, bloom_voice_sustain(v->voice));
    if (sustain < BLOOM_GAIN_MIN_SUSTAIN_Q15) sustain = BLOOM_GAIN_MIN_SUSTAIN_Q15;

    v->env_gain_q23 = BLOOM_GAIN_FLOOR_Q15 << 8;
    v->env_event_sample[0] = attack;
    v->env_event_gain_q23[0] = peak << 8;
    v->env_event_sample[1] = hold;
    v->env_event_gain_q23[1] = sustain << 8;
    v->env_event_sample[2] = duration;
    v->env_event_gain_q23[2] = BLOOM_GAIN_FLOOR_Q15 << 8;
    bloom_sort_env_events(v);
    bloom_env_prepare(v);

    bloom_pan_q15(binding->acoustic.pan_q15, &v->mix_left_q15,
                  &v->mix_right_q15);
    v->cutoff_hz = bloom_cutoff_hz(v->voice, binding->brightness_q15, base_mhz);
    v->q_milli = bloom_q_milli(v->voice, binding->brightness_q15);
    bloom_biquad_lowpass(&v->filter, v->cutoff_hz, v->q_milli);
    bloom_voice_bank(v, v->voice, v->petal_index, base_mhz);

    dsp->trigger_count++;
    active = bloom_active_count(dsp, &oscs);
    if (active > dsp->petal_high_water) dsp->petal_high_water = active;
    if (oscs > dsp->oscillator_high_water) dsp->oscillator_high_water = oscs;
    return 1u;
}

void apsis_bloom_dsp_render(ApsisBloomDsp *dsp, int32_t *out,
                            uint32_t frames)
{
    uint32_t frame;
    if (!dsp || !out) return;

    for (frame = 0u; frame < frames; ++frame) {
        int64_t add_l = 0, add_r = 0;
        uint16_t i;
        for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i) {
            ApsisBloomDspPetal *v = &dsp->petal[i];
            int32_t sum = 0, filtered, mono;
            uint8_t j;
            uint16_t gain_q15;

            if (!v->active) continue;
            if (v->start_delay_samples) {
                v->start_delay_samples--;
                continue;
            }
            if (v->age_samples >= v->life_samples) {
                v->active = 0u;
                continue;
            }

            for (j = 0u; j < v->osc_count; ++j) {
                ApsisBloomDspOsc *o = &v->osc[j];
                int32_t wave = o->waveform == APSIS_BLOOM_DSP_TRIANGLE
                    ? bloom_triangle_q15(o->phase) : bloom_sine_q15(o->phase);
                sum += (int32_t)(((int64_t)wave * o->gain_q15) >> 15);
                o->phase += o->phase_inc;
            }
            filtered = bloom_biquad_process(&v->filter, sum);
            gain_q15 = (uint16_t)(v->env_gain_q23 >> 8);
            mono = (int32_t)(((int64_t)filtered * gain_q15) >> 15);
            add_l += ((int64_t)mono * v->mix_left_q15) >> 15;
            add_r += ((int64_t)mono * v->mix_right_q15) >> 15;

            bloom_env_advance(v);
            if (v->age_samples >= v->life_samples) v->active = 0u;
        }
        out[frame * 2u] = bloom_sat_q15((int64_t)out[frame * 2u] + add_l);
        out[frame * 2u + 1u] = bloom_sat_q15((int64_t)out[frame * 2u + 1u] + add_r);
    }
}

uint16_t apsis_bloom_dsp_active(const ApsisBloomDsp *dsp)
{
    return bloom_active_count(dsp, 0);
}

uint16_t apsis_bloom_dsp_oscillator_active(const ApsisBloomDsp *dsp)
{
    uint16_t oscs = 0u;
    (void)bloom_active_count(dsp, &oscs);
    return oscs;
}
