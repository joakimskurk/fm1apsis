#include "apsis_fixed.h"
#include "apsis_sine.h"
#include "apsis_q31.h"
#include "apsis_bloom_dsp.h"
#include "apsis_string_dsp.h"

#define BLOOM_Q20_ONE 1048576u
#define BLOOM_Q30_ONE 1073741824ll
#define BLOOM_Q31_ONE 0x7fffffffu
#define BLOOM_GAIN_FLOOR_Q15 3u
#define BLOOM_GAIN_MIN_PEAK_Q15 7u
#define BLOOM_GAIN_MIN_SUSTAIN_Q15 4u
#define BLOOM_FILTER_STATE_LIMIT 131071

static void bloom_dsp_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint32_t bloom_samples_ms(uint32_t ms)
{
    return 44u * ms + (100u * ms + 500u) / 1000u;
}

static uint16_t bloom_dsp_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t bloom_quarter_sine_q15(uint16_t x_q15)
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

/* radians -> the FM index's phase units: 2^32 / (2 pi * 32767) per full-scale modulator sample */
#define BLOOM_FM_RAD_MILLI(m) ((uint32_t)(m) * 20861u / 1000u)

/* Bloom as classic 2-operator FM (the FM-1's own idiom; the user's choice 2026-11-02, docs/MARGIN_PLAN.md B): a sine
 * carrier at the petal's pitch and a sine modulator, no filter. Brightness sets the index (the additive Bloom
 * before it, three oscillators and a biquad a petal, set a cutoff). Veil 1:1, soft; Choir 1:1 with the modulator 6 cents up (a slow chorus-like drift of the
 * spectrum); Organ 1:1 at a fixed index (steady overtones); Glass 1:4 with an index that falls with the
 * envelope squared (a bell's bright strike). Level, envelope, pan and timing are the additive Bloom's. */
static void bloom_voice_fm(ApsisBloomDspPetal *v, uint8_t voice, uint8_t petal_index, uint32_t base_mhz,
                           uint16_t brightness_q15)
{
    uint32_t b = brightness_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : brightness_q15;
    uint32_t milli;                                   /* the index at the peak, milliradians */
    uint8_t ratio = 1u, follow = 1u;
    int8_t detune = 0;
    uint16_t gain = APSIS_Q15_ONE;
    if (voice == APSIS_BLOOM_CHOIR) {
        milli = 500u + 1000u * b / APSIS_Q15_ONE;
        detune = 6;
        gain = APSIS_Q15_MILLI(900);
    } else if (voice == APSIS_BLOOM_ORGAN) {
        milli = 600u + 800u * b / APSIS_Q15_ONE;
        follow = 0u;
        gain = APSIS_Q15_MILLI(800);
    } else if (voice == APSIS_BLOOM_GLASS) {
        milli = 1000u + 2000u * b / APSIS_Q15_ONE;
        ratio = 4u;
        follow = 2u;
        gain = APSIS_Q15_MILLI(840);
    } else {
        milli = 350u + 900u * b / APSIS_Q15_ONE + (petal_index == 0u ? 300u : 0u);
    }
    if (milli > 3100u) milli = 3100u;                 /* index * modulator stays inside 32 bits */
    v->osc_count = 0u;
    bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, 1u, gain, 0, base_mhz);
    bloom_add_osc(v, APSIS_BLOOM_DSP_SINE, ratio, APSIS_Q15_ONE, detune, base_mhz);
    v->fm_index_peak = (uint16_t)BLOOM_FM_RAD_MILLI(milli);
    v->fm_follow = follow;
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
        v->env_decay_mul_q31 = apsis_nth_root_q31(ratio, len);
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

    peak = bloom_dsp_mul_q15(binding->acoustic.amp_q15,
                         bloom_voice_gain_scale(v->voice));
    if (peak < BLOOM_GAIN_MIN_PEAK_Q15) peak = BLOOM_GAIN_MIN_PEAK_Q15;
    sustain = bloom_dsp_mul_q15((uint16_t)peak, bloom_voice_sustain(v->voice));
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
    v->fm_env_peak_q15 = (uint16_t)peak;
    bloom_voice_fm(v, v->voice, v->petal_index, base_mhz, binding->brightness_q15);

    dsp->trigger_count++;
    active = bloom_active_count(dsp, &oscs);
    if (active > dsp->petal_high_water) dsp->petal_high_water = active;
    if (oscs > dsp->oscillator_high_water) dsp->oscillator_high_water = oscs;
    return 1u;
}

/* the FM index for the envelope's current gain: fixed, or the peak index scaled by gain/peak (or its square) */
APSIS_INLINE int32_t bloom_fm_index(const ApsisBloomDspPetal *v, uint32_t gain_q23)
{
    uint32_t g = gain_q23 >> 8, peak = v->fm_env_peak_q15, r;
    if (!v->fm_follow || !peak) return v->fm_index_peak;
    r = g >= peak ? APSIS_Q15_ONE : g * APSIS_Q15_ONE / peak;
    if (v->fm_follow == 2u) r = r * r >> 15;
    return (int32_t)((v->fm_index_peak * r) >> 15);
}

/* One FM petal over the block, added into out unsaturated (the caller saturates once; petals touch only their own
 * state): modulator, carrier at the modulated phase, the envelope's gain and the pan, with the state in locals
 * (written back at the end and around an envelope stage change, bloom_env_prepare, a few times a petal's life);
 * the index follows the envelope every 32 samples. */
static void bloom_petal_render(ApsisBloomDspPetal *v, int32_t *out, uint32_t frames)
{
    uint32_t k = 0u, age = v->age_samples;
    const uint32_t life = v->life_samples;
    const int32_t mix_l = v->mix_left_q15, mix_r = v->mix_right_q15, cg = v->osc[0].gain_q15;
    uint32_t pc = v->osc[0].phase, pm = v->osc[1].phase;
    const uint32_t ic = v->osc[0].phase_inc, im = v->osc[1].phase_inc;
    uint32_t gain = v->env_gain_q23, left = v->env_stage_left, step = v->env_linear_step_q23;
    uint32_t rem = v->env_linear_rem_q23, acc = v->env_linear_acc_q23, den = v->env_linear_den;
    uint32_t mul = v->env_decay_mul_q31;
    uint8_t rising = v->env_stage_rising;
    int32_t index = bloom_fm_index(v, gain);

    if (v->start_delay_samples) {
        k = v->start_delay_samples < frames ? v->start_delay_samples : frames;
        v->start_delay_samples -= k;
    }
    for (; k < frames && age < life; ++k) {
        int32_t wave, mono;
        if (!(age & 31u)) index = bloom_fm_index(v, gain);
        pm += im;
        pc += ic;
        /* |modulator * index| <= 32767 * 64669 < 2^31 */
        wave = (apsis_sine_q15(pc + (uint32_t)(apsis_sine_q15(pm) * index)) * cg) >> 15;
        mono = (wave * (int32_t)(gain >> 8)) >> 15;          /* 32767 * 65535 < 2^31 */
        out[k * 2u] += (mono * mix_l) >> 15;                  /* |mono| <= 65535, mix <= 32767 */
        out[k * 2u + 1u] += (mono * mix_r) >> 15;
        if (left) {
            if (rising) {
                gain += step;
                acc += rem;
                if (acc >= den) {
                    gain++;
                    acc -= den;
                }
            } else {
                gain = (uint32_t)(((uint64_t)gain * mul + (1ull << 30)) >> 31);
            }
            left--;
        }
        age++;
        if (!left && v->env_event_index < 3u && age >= v->env_event_sample[v->env_event_index]) {
            v->age_samples = age;
            v->env_gain_q23 = v->env_stage_target_q23;
            v->env_event_index++;
            bloom_env_prepare(v);
            gain = v->env_gain_q23;
            left = v->env_stage_left;
            step = v->env_linear_step_q23;
            rem = v->env_linear_rem_q23;
            acc = v->env_linear_acc_q23;
            den = v->env_linear_den;
            mul = v->env_decay_mul_q31;
            rising = v->env_stage_rising;
        }
    }
    v->age_samples = age;
    v->env_gain_q23 = gain;
    v->env_stage_left = left;
    v->env_linear_acc_q23 = acc;
    v->osc[0].phase = pc;
    v->osc[1].phase = pm;
    if (age >= life) v->active = 0u;
}

void apsis_bloom_dsp_render(ApsisBloomDsp *dsp, int32_t *out,
                            uint32_t frames)
{
    uint32_t i;
    if (!dsp || !out) return;
    for (i = 0u; i < APSIS_BLOOM_DSP_PETAL_CAP; ++i)
        if (dsp->petal[i].active) bloom_petal_render(&dsp->petal[i], out, frames);
    for (i = 0u; i < frames * 2u; ++i)
        out[i] = apsis_sat_q15(out[i]);
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
