#include "apsis_fixed.h"
#include "apsis_sine.h"
#include "apsis_q31.h"
#include "apsis_spiral_dsp.h"
#include "apsis_string_dsp.h"

#define SPIRAL_Q20_ONE 1048576u
#define SPIRAL_Q30_ONE 1073741824ll
#define SPIRAL_Q31_ONE 0x7fffffffu
#define SPIRAL_GAIN_FLOOR_Q15 3u
#define SPIRAL_GAIN_MIN_PEAK_Q15 7u
#define SPIRAL_FILTER_STATE_LIMIT 131071


static void spiral_dsp_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

APSIS_INLINE int32_t spiral_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

APSIS_INLINE int32_t spiral_filter_state(int64_t v)
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






APSIS_INLINE int16_t spiral_triangle_q15(uint32_t phase)
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
    a = apsis_sine_table[index];
    b = apsis_sine_table[index + 1u];
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
    return (int32_t)(((int64_t)apsis_sine_q15(phase) * SPIRAL_Q30_ONE) / 32767ll);
}

static int32_t spiral_cos_q30_for_hz(uint16_t hz)
{
    uint32_t phase = (uint32_t)hz * 97392u;
    return (int32_t)(((int64_t)apsis_sine_q15(phase + 0x40000000u) *
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
    return apsis_nth_root_q31(target, samples);
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

/* the leg's envelope, on locals: its gain for its next sample (age is the sample's age before it counts) */
APSIS_INLINE uint16_t spiral_envelope_at(uint32_t age, uint32_t attack, uint32_t tail, uint32_t peak_q23,
                                         uint32_t decay_mul_q31, uint32_t *gain_q23)
{
    const uint32_t floor_q23 = SPIRAL_GAIN_FLOOR_Q15 << 8;
    if (age < attack && attack) {
        /* the old 64-bit divisions in 32 bits: age < attack (a few hundred samples), so (age + 1) * 32767 < 2^32;
         * t * t < 2^30; the last product < 2^39 goes through apsis_div32767_u64 */
        uint32_t t = ((age + 1u) * APSIS_Q15_ONE + attack / 2u) / attack;
        uint32_t curve;
        if (t > APSIS_Q15_ONE) t = APSIS_Q15_ONE;
        curve = (t * t + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
        *gain_q23 = floor_q23 + apsis_div32767_u64((uint64_t)(peak_q23 - floor_q23) * curve + APSIS_Q15_ONE / 2u);
    } else if (age < tail) {
        *gain_q23 = (uint32_t)(((uint64_t)*gain_q23 * decay_mul_q31 + (1ull << 30)) >> 31);
        if (*gain_q23 < floor_q23) *gain_q23 = floor_q23;
    } else {
        *gain_q23 = floor_q23;
    }
    return (uint16_t)(*gain_q23 >> 8);
}

/* One leg over `frames` samples, added into mix (unsaturated; the caller saturates once per sample, as the old
 * sample-outer loop did after summing every leg: integer sums do not depend on the order of the legs). The leg's
 * state lives in locals for the block. oscs is a constant at each call. */
APSIS_INLINE void spiral_leg_render(ApsisSpiralDspLeg *v, int32_t *mix, uint32_t frames, uint8_t oscs)
{
    uint32_t k = 0u, age = v->age_samples, gain_q23 = v->gain_q23;
    const uint32_t life = v->life_samples, attack = v->attack_samples, tail = v->tail_samples;
    const uint32_t peak_q23 = (uint32_t)v->peak_gain_q15 << 8, decay_mul = v->decay_mul_q31;
    const int32_t mix_l = v->mix_left_q15, mix_r = v->mix_right_q15;
    const int32_t b0 = v->filter.b0_q30, b1 = v->filter.b1_q30, b2 = v->filter.b2_q30;
    const int32_t a1 = v->filter.a1_q30, a2 = v->filter.a2_q30;
    int32_t x1 = v->filter.x1, x2 = v->filter.x2, y1 = v->filter.y1, y2 = v->filter.y2;
    uint32_t ph0 = v->osc[0].phase, ph1 = v->osc[1].phase, ph2 = v->osc[2].phase;
    const uint32_t in0 = v->osc[0].phase_inc, in1 = v->osc[1].phase_inc, in2 = v->osc[2].phase_inc;
    const int32_t g0 = v->osc[0].gain_q15, g1 = v->osc[1].gain_q15, g2 = v->osc[2].gain_q15;
    const uint8_t t0 = v->osc[0].waveform == APSIS_SPIRAL_DSP_TRIANGLE;
    const uint8_t t1 = v->osc[1].waveform == APSIS_SPIRAL_DSP_TRIANGLE;
    const uint8_t t2 = v->osc[2].waveform == APSIS_SPIRAL_DSP_TRIANGLE;
    uint8_t active = 1u;

    if (v->start_delay_samples) {              /* a waiting sample only counts its delay down */
        k = v->start_delay_samples < frames ? v->start_delay_samples : frames;
        v->start_delay_samples -= k;
    }
    for (; k < frames; ++k) {
        int32_t raw, x, y, voiced;
        int64_t acc;
        if (age >= life) {
            active = 0u;
            break;
        }
        /* |wave * gain| < 2^31 (wave <= 32767, gain <= 65535): the old int64 sum, in 32 bits */
        raw = 0;
        if (oscs > 0u) {
            ph0 += in0;
            raw = ((t0 ? spiral_triangle_q15(ph0) : apsis_sine_q15(ph0)) * g0) >> 15;
        }
        if (oscs > 1u) {
            ph1 += in1;
            raw += ((t1 ? spiral_triangle_q15(ph1) : apsis_sine_q15(ph1)) * g1) >> 15;
        }
        if (oscs > 2u) {
            ph2 += in2;
            raw += ((t2 ? spiral_triangle_q15(ph2) : apsis_sine_q15(ph2)) * g2) >> 15;
        }
        x = spiral_filter_state(raw);
        acc = (int64_t)b0 * x + (int64_t)b1 * x1 + (int64_t)b2 * x2 - (int64_t)a1 * y1 - (int64_t)a2 * y2;
        y = spiral_filter_state(acc >> 30);
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        voiced = (int32_t)(((int64_t)y * spiral_envelope_at(age, attack, tail, peak_q23, decay_mul, &gain_q23)) >> 15);
        mix[k * 2u] += (int32_t)(((int64_t)voiced * mix_l) >> 15);
        mix[k * 2u + 1u] += (int32_t)(((int64_t)voiced * mix_r) >> 15);
        ++age;
        if (age >= life) {
            active = 0u;
            break;
        }
    }
    v->age_samples = age;
    v->gain_q23 = gain_q23;
    v->filter.x1 = x1;
    v->filter.x2 = x2;
    v->filter.y1 = y1;
    v->filter.y2 = y2;
    v->osc[0].phase = ph0;
    v->osc[1].phase = ph1;
    v->osc[2].phase = ph2;
    v->active = active;
}

#define SPIRAL_CHUNK 256u
static int32_t spiral_mix[SPIRAL_CHUNK * 2u];

void apsis_spiral_dsp_render(ApsisSpiralDsp *dsp, int32_t *out,
                             uint32_t frames)
{
    uint32_t base, n, c;
    uint16_t i;
    if (!dsp || !out) return;
    for (base = 0u; base < frames; base += c) {
        c = frames - base > SPIRAL_CHUNK ? SPIRAL_CHUNK : frames - base;
        for (n = 0u; n < c * 2u; ++n) spiral_mix[n] = 0;
        for (i = 0u; i < APSIS_SPIRAL_DSP_LEG_CAP; ++i) {
            ApsisSpiralDspLeg *v = &dsp->leg[i];
            if (!v->active) continue;
            if (v->osc_count >= 3u) spiral_leg_render(v, spiral_mix, c, 3u);
            else if (v->osc_count == 2u) spiral_leg_render(v, spiral_mix, c, 2u);
            else if (v->osc_count == 1u) spiral_leg_render(v, spiral_mix, c, 1u);
            else spiral_leg_render(v, spiral_mix, c, 0u);
        }
        for (n = 0u; n < c * 2u; ++n)
            out[base * 2u + n] = spiral_sat_q15((int64_t)out[base * 2u + n] + spiral_mix[n]);
    }
}
