#include "apsis_spiral_dsp.h"
#include "apsis_string_dsp.h"

#define SPIRAL_Q20_ONE 1048576u
#define SPIRAL_Q30_ONE 1073741824ll
#define SPIRAL_Q31_ONE 0x7fffffffu
#define SPIRAL_GAIN_FLOOR_Q15 3u
#define SPIRAL_GAIN_MIN_PEAK_Q15 7u
#define SPIRAL_FILTER_STATE_LIMIT 131071

static const int16_t SPIRAL_SINE_QUARTER[65] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static void spiral_dsp_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static int32_t spiral_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static int32_t spiral_filter_state(int64_t v)
{
    if (v > SPIRAL_FILTER_STATE_LIMIT) return SPIRAL_FILTER_STATE_LIMIT;
    if (v < -SPIRAL_FILTER_STATE_LIMIT) return -SPIRAL_FILTER_STATE_LIMIT;
    return (int32_t)v;
}

static uint32_t spiral_samples_ms(uint32_t ms)
{
    return (APSIS_SPIRAL_DSP_RATE * ms + 500u) / 1000u;
}

static uint16_t spiral_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t spiral_mul_q31(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + (1ull << 30)) >> 31);
}

static uint32_t spiral_pow_q31(uint32_t base, uint32_t exp)
{
    uint32_t out = SPIRAL_Q31_ONE;
    while (exp) {
        if (exp & 1u) out = spiral_mul_q31(out, base);
        exp >>= 1u;
        if (exp) base = spiral_mul_q31(base, base);
    }
    return out;
}

static uint32_t spiral_nth_root_q31(uint32_t target, uint32_t n)
{
    uint32_t lo, hi, k;
    if (!n || target >= SPIRAL_Q31_ONE) return SPIRAL_Q31_ONE;
    if (!target) return 0u;
    lo = target;
    hi = SPIRAL_Q31_ONE;
    for (k = 0u; k < 24u; ++k) {
        uint32_t mid = lo + ((hi - lo) >> 1u);
        uint32_t p = spiral_pow_q31(mid, n);
        if (p > target) hi = mid;
        else lo = mid;
    }
    return lo + ((hi - lo) >> 1u);
}

static int16_t spiral_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return SPIRAL_SINE_QUARTER[o];
    if (q == 1u) return SPIRAL_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-SPIRAL_SINE_QUARTER[o];
    return (int16_t)-SPIRAL_SINE_QUARTER[64u - o];
}

static int16_t spiral_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = spiral_sine_point(i);
    int32_t b = spiral_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static int16_t spiral_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}

static uint16_t spiral_quarter_sine_q15(uint16_t x_q15)
{
    uint32_t scaled = (uint32_t)x_q15 * 64u;
    uint32_t index = scaled / APSIS_Q15_ONE;
    uint32_t rem;
    int32_t a, b;
    if (index >= 64u) return 32767u;
    rem = scaled - index * APSIS_Q15_ONE;
    a = SPIRAL_SINE_QUARTER[index];
    b = SPIRAL_SINE_QUARTER[index + 1u];
    return (uint16_t)(a + ((int32_t)(b - a) * (int32_t)rem +
                           (int32_t)APSIS_Q15_ONE / 2) /
                          (int32_t)APSIS_Q15_ONE);
}

static void spiral_pan_q15(int16_t pan_q15, uint16_t *left, uint16_t *right)
{
    int32_t pan = pan_q15;
    uint16_t x;
    if (pan < -(int32_t)APSIS_Q15_ONE) pan = -(int32_t)APSIS_Q15_ONE;
    if (pan > (int32_t)APSIS_Q15_ONE) pan = APSIS_Q15_ONE;
    x = (uint16_t)(((uint32_t)(pan + (int32_t)APSIS_Q15_ONE) *
                    APSIS_Q15_ONE + APSIS_Q15_ONE) /
                   (2u * APSIS_Q15_ONE));
    if (right) *right = spiral_quarter_sine_q15(x);
    if (left) *left = spiral_quarter_sine_q15((uint16_t)(APSIS_Q15_ONE - x));
}

static uint32_t spiral_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static int32_t spiral_sin_q30_for_hz(uint16_t hz)
{
    /* round(2^32 / 44100) = 97392. hz is <= 10500, so the product fits u32
     * and avoids another target DI-mode division. */
    uint32_t phase = (uint32_t)hz * 97392u;
    return (int32_t)(((int64_t)spiral_sine_q15(phase) * SPIRAL_Q30_ONE) / 32767ll);
}

static int32_t spiral_cos_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)hz * 97392u;
    return (int32_t)(((int64_t)spiral_sine_q15(phase + 0x40000000u) *
                      SPIRAL_Q30_ONE) / 32767ll);
}

static int32_t spiral_norm_q30(int64_t value_q30, int64_t a0_q30)
{
    return (int32_t)((value_q30 * SPIRAL_Q30_ONE) / a0_q30);
}

static void spiral_biquad_lowpass(ApsisSpiralDspBiquad *b,
                                  uint16_t hz, uint16_t q_milli)
{
    int64_t s, c, alpha, a0, one_minus_c;
    if (!b) return;
    s = spiral_sin_q30_for_hz(hz);
    c = spiral_cos_q30_for_hz(hz);
    /* alpha = sin(w0)/(2Q). The source has only Q .28 and .62. */
    alpha = q_milli >= 600u ? (s * 25ll) / 31ll : (s * 25ll) / 14ll;
    a0 = SPIRAL_Q30_ONE + alpha;
    one_minus_c = SPIRAL_Q30_ONE - c;
    b->b0_q30 = spiral_norm_q30(one_minus_c / 2ll, a0);
    b->b1_q30 = spiral_norm_q30(one_minus_c, a0);
    b->b2_q30 = b->b0_q30;
    b->a1_q30 = spiral_norm_q30(-2ll * c, a0);
    b->a2_q30 = spiral_norm_q30(SPIRAL_Q30_ONE - alpha, a0);
    b->x1 = b->x2 = b->y1 = b->y2 = 0;
}

static int32_t spiral_biquad_process(ApsisSpiralDspBiquad *b, int32_t x)
{
    int64_t acc;
    int32_t y;
    if (!b) return x;
    acc = (int64_t)b->b0_q30 * x + (int64_t)b->b1_q30 * b->x1 +
          (int64_t)b->b2_q30 * b->x2 - (int64_t)b->a1_q30 * b->y1 -
          (int64_t)b->a2_q30 * b->y2;
    y = spiral_filter_state(acc >> 30);
    b->x2 = b->x1;
    b->x1 = x;
    b->y2 = b->y1;
    b->y1 = y;
    return y;
}

static uint32_t spiral_detune_q20(int8_t cents)
{
    switch (cents) {
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
        default: return SPIRAL_Q20_ONE;
    }
}

static uint32_t spiral_osc_freq(uint32_t base_mhz, uint8_t ratio, int8_t detune)
{
    uint64_t v = (uint64_t)base_mhz * (ratio ? ratio : 1u);
    v = (v * spiral_detune_q20(detune) + SPIRAL_Q20_ONE / 2u) >> 20;
    return v > 0xffffffffull ? 0xffffffffu : (uint32_t)v;
}

static int32_t spiral_div100_round(int32_t v)
{
    return v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100);
}

static uint32_t spiral_base_frequency(const ApsisState *state,
                                      const ApsisSpiralLateBinding *pitch)
{
    ApsisState equal;
    int32_t step_milli;
    if (!state || !pitch || !pitch->valid) return 0u;
    equal = *state;
    equal.gravity.scale_lock = 0u;
    equal.temper.mode = APSIS_TUNING_EQUAL;
    equal.temper.strength_q15 = 0u;
    step_milli = (int32_t)pitch->step * 1000 +
                 spiral_div100_round(pitch->tuning_deviation_milli);
    return apsis_string_dsp_frequency_millihz(&equal, step_milli);
}

static uint16_t spiral_cutoff_hz(uint8_t family, uint32_t freq_mhz)
{
    uint32_t hz = (freq_mhz + 500u) / 1000u;
    uint32_t cutoff;
    if (family == APSIS_SPIRAL_GLASS) {
        cutoff = (hz * 42u + 5u) / 10u;
        if (cutoff < 3800u) cutoff = 3800u;
        if (cutoff > 10500u) cutoff = 10500u;
    } else if (family == APSIS_SPIRAL_REED) {
        cutoff = hz * 3u;
        if (cutoff < 2100u) cutoff = 2100u;
        if (cutoff > 6200u) cutoff = 6200u;
    } else if (family == APSIS_SPIRAL_HALO) {
        cutoff = (hz * 35u + 5u) / 10u;
        if (cutoff < 2600u) cutoff = 2600u;
        if (cutoff > 7600u) cutoff = 7600u;
    } else {
        cutoff = (hz * 31u + 5u) / 10u;
        if (cutoff < 2500u) cutoff = 2500u;
        if (cutoff > 8500u) cutoff = 8500u;
    }
    return (uint16_t)cutoff;
}

static uint16_t spiral_topology_q(uint8_t voice)
{
    return voice == APSIS_SPIRAL_REED ? 620u : 280u;
}

static uint32_t spiral_attack_samples(uint8_t voice)
{
    if (voice == APSIS_SPIRAL_GLASS) return spiral_samples_ms(6u);
    if (voice == APSIS_SPIRAL_HALO) return spiral_samples_ms(22u);
    return spiral_samples_ms(10u);
}

static uint32_t spiral_tail_samples(uint8_t voice, uint32_t leg_duration)
{
    if (voice == APSIS_SPIRAL_HALO)
        return (uint32_t)(((uint64_t)leg_duration * 112u + 50u) / 100u);
    return leg_duration;
}

static uint32_t spiral_decay_mul(uint32_t peak_q23, uint32_t samples)
{
    uint32_t floor_q23 = SPIRAL_GAIN_FLOOR_Q15 << 8;
    uint32_t target;
    if (!samples || peak_q23 <= floor_q23) return SPIRAL_Q31_ONE;
    target = (uint32_t)(((uint64_t)floor_q23 << 31) / peak_q23);
    return spiral_nth_root_q31(target, samples);
}

static void spiral_add_osc(ApsisSpiralDspLeg *v, uint8_t waveform,
                           uint8_t ratio, uint16_t gain_q15, int8_t detune,
                           uint32_t base_mhz)
{
    ApsisSpiralDspOsc *o;
    uint32_t mhz;
    if (!v || v->osc_count >= APSIS_SPIRAL_DSP_OSC_MAX) return;
    o = &v->osc[v->osc_count++];
    mhz = spiral_osc_freq(base_mhz, ratio, detune);
    o->phase_inc = spiral_phase_inc_from_millihz(mhz);
    o->gain_q15 = gain_q15;
    o->waveform = waveform;
}

static void spiral_voice_bank(ApsisSpiralDspLeg *v, uint8_t voice,
                              uint8_t shadow, uint32_t base_mhz)
{
    if (!v) return;
    if (voice == APSIS_SPIRAL_GLASS) {
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 1u, APSIS_Q15_MILLI(820), 0, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 2u, APSIS_Q15_MILLI(200), 2, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 3u, APSIS_Q15_MILLI(55), -3, base_mhz);
    } else if (voice == APSIS_SPIRAL_REED) {
        spiral_add_osc(v, APSIS_SPIRAL_DSP_TRIANGLE, 1u, APSIS_Q15_MILLI(820), 0, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 2u, APSIS_Q15_MILLI(150), 0, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 3u, APSIS_Q15_MILLI(45), 1, base_mhz);
    } else if (voice == APSIS_SPIRAL_HALO) {
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 1u, APSIS_Q15_MILLI(550), -5, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 1u, APSIS_Q15_MILLI(550), 5, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 2u, APSIS_Q15_MILLI(100), 0, base_mhz);
    } else {
        spiral_add_osc(v, shadow ? APSIS_SPIRAL_DSP_SINE : APSIS_SPIRAL_DSP_TRIANGLE,
                       1u, APSIS_Q15_ONE, 0, base_mhz);
        spiral_add_osc(v, APSIS_SPIRAL_DSP_SINE, 2u,
                       shadow ? APSIS_Q15_MILLI(55) : APSIS_Q15_MILLI(85),
                       0, base_mhz);
    }
}

uint16_t apsis_spiral_dsp_leg_active(const ApsisSpiralDsp *dsp)
{
    uint16_t n = 0u, i;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i)
        if (dsp->leg[i].active) ++n;
    return n;
}

uint16_t apsis_spiral_dsp_oscillator_active(const ApsisSpiralDsp *dsp)
{
    uint16_t n = 0u, i;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i)
        if (dsp->leg[i].active) n = (uint16_t)(n + dsp->leg[i].osc_count);
    return n;
}

uint16_t apsis_spiral_dsp_logical_active(const ApsisSpiralDsp *dsp)
{
    uint16_t n = 0u, i, j;
    if (!dsp) return 0u;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i) {
        uint8_t seen = 0u;
        if (!dsp->leg[i].active) continue;
        for (j = 0u; j < i; ++j) {
            if (dsp->leg[j].active && dsp->leg[j].token == dsp->leg[i].token) {
                seen = 1u;
                break;
            }
        }
        if (!seen) ++n;
    }
    return n;
}

static void spiral_update_high_water(ApsisSpiralDsp *dsp)
{
    uint16_t logical, legs, oscs;
    if (!dsp) return;
    logical = apsis_spiral_dsp_logical_active(dsp);
    legs = apsis_spiral_dsp_leg_active(dsp);
    oscs = apsis_spiral_dsp_oscillator_active(dsp);
    if (logical > dsp->logical_high_water) dsp->logical_high_water = logical;
    if (legs > dsp->leg_high_water) dsp->leg_high_water = legs;
    if (oscs > dsp->oscillator_high_water) dsp->oscillator_high_water = oscs;
}

static ApsisSpiralDspLeg *spiral_alloc_leg(ApsisSpiralDsp *dsp)
{
    uint16_t i, oldest = 0u;
    uint32_t oldest_serial = 0xffffffffu;
    if (!dsp) return 0;
    for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i) {
        if (!dsp->leg[i].active) {
            spiral_dsp_zero(&dsp->leg[i], (uint32_t)sizeof(dsp->leg[i]));
            dsp->leg[i].active = 1u;
            return &dsp->leg[i];
        }
        if (dsp->leg[i].serial < oldest_serial) {
            oldest_serial = dsp->leg[i].serial;
            oldest = i;
        }
    }
    dsp->steal_count++;
    spiral_dsp_zero(&dsp->leg[oldest], (uint32_t)sizeof(dsp->leg[oldest]));
    dsp->leg[oldest].active = 1u;
    return &dsp->leg[oldest];
}

void apsis_spiral_dsp_reset(ApsisSpiralDsp *dsp)
{
    if (!dsp) return;
    spiral_dsp_zero(dsp, (uint32_t)sizeof(*dsp));
}

static uint32_t spiral_delay_from_target(uint64_t target, uint64_t block_start)
{
    uint64_t d;
    if (target <= block_start) return 0u;
    d = target - block_start;
    return d > 0xffffffffull ? 0xffffffffu : (uint32_t)d;
}

static uint8_t spiral_init_leg(ApsisSpiralDsp *dsp,
                               const ApsisSpiralAudioBinding *binding,
                               const ApsisSpiralLegPlan *plan,
                               uint32_t base_mhz, uint64_t block_start)
{
    ApsisSpiralDspLeg *v;
    uint64_t target;
    uint32_t half_delta, leg_mhz, decay_samples, life;
    uint16_t peak;
    if (!dsp || !binding || !plan || !base_mhz) return 0u;
    v = spiral_alloc_leg(dsp);
    if (!v) {
        dsp->drop_count++;
        return 0u;
    }

    target = binding->target_sample + plan->start_offset_samples;
    if (target < binding->target_sample) target = ~(uint64_t)0;
    half_delta = ((uint32_t)binding->acoustic.drift_delta_millihz + 1u) / 2u;
    if (plan->drift_sign < 0)
        leg_mhz = base_mhz > half_delta ? base_mhz - half_delta : 1u;
    else if (plan->drift_sign > 0)
        leg_mhz = base_mhz + half_delta;
    else
        leg_mhz = base_mhz;

    v->serial = ++dsp->next_serial;
    v->token = binding->token;
    v->topology_voice = binding->acoustic.voice;
    v->cutoff_voice = binding->cutoff_voice;
    v->shadow = plan->shadow;
    v->start_delay_samples = spiral_delay_from_target(target, block_start);
    v->attack_samples = spiral_attack_samples(v->topology_voice);
    v->tail_samples = spiral_tail_samples(v->topology_voice, plan->duration_samples);
    if (!v->tail_samples) v->tail_samples = 1u;
    if (v->attack_samples > v->tail_samples) v->attack_samples = v->tail_samples;
    life = v->tail_samples + spiral_samples_ms(50u);
    if (life < v->tail_samples) life = 0xffffffffu;
    v->life_samples = life;

    peak = plan->gain_q15;
    if (v->topology_voice == APSIS_SPIRAL_HALO)
        peak = spiral_mul_q15(peak, APSIS_Q15_MILLI(820));
    if (peak < SPIRAL_GAIN_MIN_PEAK_Q15) peak = SPIRAL_GAIN_MIN_PEAK_Q15;
    v->peak_gain_q15 = peak;
    v->gain_q23 = SPIRAL_GAIN_FLOOR_Q15 << 8;
    decay_samples = v->tail_samples > v->attack_samples
        ? v->tail_samples - v->attack_samples : 1u;
    v->decay_mul_q31 = spiral_decay_mul((uint32_t)peak << 8, decay_samples);
    spiral_pan_q15(plan->pan_q15, &v->mix_left_q15, &v->mix_right_q15);
    v->cutoff_hz = spiral_cutoff_hz(v->cutoff_voice, leg_mhz);
    v->q_milli = spiral_topology_q(v->topology_voice);
    spiral_biquad_lowpass(&v->filter, v->cutoff_hz, v->q_milli);
    spiral_voice_bank(v, v->topology_voice, v->shadow, leg_mhz);
    if (!v->osc_count) {
        v->active = 0u;
        dsp->drop_count++;
        return 0u;
    }
    dsp->leg_trigger_count++;
    spiral_update_high_water(dsp);
    return 1u;
}

uint8_t apsis_spiral_dsp_trigger_binding(ApsisSpiralDsp *dsp,
                                         const ApsisState *state,
                                         const ApsisSpiralAudioBinding *binding,
                                         uint64_t block_start)
{
    uint32_t base_mhz;
    uint8_t i, accepted = 0u;
    if (!dsp || !state || !binding || !binding->valid ||
        !binding->acoustic.valid || !binding->pitch.valid)
        return 0u;
    base_mhz = spiral_base_frequency(state, &binding->pitch);
    if (!base_mhz) return 0u;
    dsp->hit_trigger_count++;
    for (i = 0u; i < binding->acoustic.leg_count &&
                 i < APSIS_SPIRAL_AUDIO_LEGS_MAX; ++i)
        accepted = (uint8_t)(accepted + spiral_init_leg(
            dsp, binding, &binding->acoustic.leg[i], base_mhz, block_start));
    return accepted;
}

static uint16_t spiral_envelope(ApsisSpiralDspLeg *v)
{
    uint32_t floor_q23 = SPIRAL_GAIN_FLOOR_Q15 << 8;
    uint32_t peak_q23;
    if (!v) return 0u;
    peak_q23 = (uint32_t)v->peak_gain_q15 << 8;
    if (v->age_samples < v->attack_samples && v->attack_samples) {
        uint32_t t = (uint32_t)(((uint64_t)(v->age_samples + 1u) *
                                 APSIS_Q15_ONE + v->attack_samples / 2u) /
                                v->attack_samples);
        uint32_t curve, g;
        if (t > APSIS_Q15_ONE) t = APSIS_Q15_ONE;
        curve = (uint32_t)(((uint64_t)t * t + APSIS_Q15_ONE / 2u) /
                           APSIS_Q15_ONE);
        g = floor_q23 + (uint32_t)(((uint64_t)(peak_q23 - floor_q23) *
                                    curve + APSIS_Q15_ONE / 2u) /
                                   APSIS_Q15_ONE);
        v->gain_q23 = g;
    } else if (v->age_samples < v->tail_samples) {
        v->gain_q23 = (uint32_t)(((uint64_t)v->gain_q23 *
                                  v->decay_mul_q31 + (1ull << 30)) >> 31);
        if (v->gain_q23 < floor_q23) v->gain_q23 = floor_q23;
    } else {
        v->gain_q23 = floor_q23;
    }
    return (uint16_t)(v->gain_q23 >> 8);
}

void apsis_spiral_dsp_render(ApsisSpiralDsp *dsp, int32_t *out,
                             uint32_t frames)
{
    uint32_t n;
    uint16_t i;
    if (!dsp || !out) return;
    for (n = 0u; n < frames; ++n) {
        int64_t mix_l = 0, mix_r = 0;
        for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i) {
            ApsisSpiralDspLeg *v = &dsp->leg[i];
            int64_t raw = 0;
            int32_t filtered, voiced;
            uint16_t env;
            uint8_t j;
            if (!v->active) continue;
            if (v->start_delay_samples) {
                --v->start_delay_samples;
                continue;
            }
            if (v->age_samples >= v->life_samples) {
                v->active = 0u;
                continue;
            }
            for (j = 0u; j < v->osc_count; ++j) {
                ApsisSpiralDspOsc *o = &v->osc[j];
                int32_t wave;
                o->phase += o->phase_inc;
                wave = o->waveform == APSIS_SPIRAL_DSP_TRIANGLE
                    ? spiral_triangle_q15(o->phase) : spiral_sine_q15(o->phase);
                raw += ((int64_t)wave * o->gain_q15) >> 15;
            }
            filtered = spiral_biquad_process(&v->filter, spiral_filter_state(raw));
            env = spiral_envelope(v);
            voiced = (int32_t)(((int64_t)filtered * env) >> 15);
            mix_l += ((int64_t)voiced * v->mix_left_q15) >> 15;
            mix_r += ((int64_t)voiced * v->mix_right_q15) >> 15;
            ++v->age_samples;
            if (v->age_samples >= v->life_samples) v->active = 0u;
        }
        out[n * 2u] = spiral_sat_q15((int64_t)out[n * 2u] + mix_l);
        out[n * 2u + 1u] = spiral_sat_q15((int64_t)out[n * 2u + 1u] + mix_r);
    }
}
