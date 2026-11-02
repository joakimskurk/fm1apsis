#include "apsis_fixed.h"
#include "apsis_sine.h"
#include "apsis_drift_dsp.h"
#include "apsis_drift.h"
#include "apsis_harmony.h"
#include "apsis_string_dsp.h"

/* Source WebAudio setTargetAtTime() time constant: 25 ms. At 44.1 kHz,
 * 1-exp(-1/(0.025*44100)) = 0.0009066 -> Q15 30. */
#define DRIFT_SMOOTH_ALPHA_Q15 30u
#define DRIFT_BASE_GAIN_Q15 APSIS_Q15_MILLI(13)
#define DRIFT_PAN_MAX_Q15 APSIS_Q15_MILLI(340)


static void drift_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t drift_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

APSIS_INLINE int32_t drift_smooth_i32(int32_t current, int32_t target)
{
    int64_t diff = (int64_t)target - current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * DRIFT_SMOOTH_ALPHA_Q15) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (int32_t)((int64_t)current + delta);
}

APSIS_INLINE uint32_t drift_smooth_u32(uint32_t current, uint32_t target)
{
    int64_t diff = (int64_t)target - current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * DRIFT_SMOOTH_ALPHA_Q15) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (uint32_t)((int64_t)current + delta);
}




/* sin(x*pi/2), x in Q15 [0,1]. Used for WebAudio StereoPanner-style
 * equal-power gains. */
static uint16_t drift_quarter_sine_q15(uint16_t x_q15)
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

static uint32_t drift_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static void drift_equal_power_pair(uint16_t width_q15,
                                   uint16_t *near_q15,
                                   uint16_t *far_q15)
{
    uint16_t width = width_q15 > APSIS_Q15_ONE ? APSIS_Q15_ONE : width_q15;
    uint16_t pan = drift_mul_q15(width, DRIFT_PAN_MAX_Q15);
    uint16_t x_far, x_near;
    if (!pan) {
        uint16_t centre = drift_quarter_sine_q15((uint16_t)((APSIS_Q15_ONE + 1u) / 2u));
        if (near_q15) *near_q15 = centre;
        if (far_q15) *far_q15 = centre;
        return;
    }
    x_far = (uint16_t)((APSIS_Q15_ONE - pan) / 2u);
    x_near = (uint16_t)(APSIS_Q15_ONE - x_far);
    if (near_q15) *near_q15 = drift_quarter_sine_q15(x_near);
    if (far_q15) *far_q15 = drift_quarter_sine_q15(x_far);
}

void apsis_drift_dsp_targets(const ApsisState *s,
                             uint8_t transport_running,
                             ApsisDriftDspTargets *out)
{
    uint32_t carrier, half;
    uint16_t delta, gain, near_mix, far_mix;
    int32_t root_step;
    uint8_t audible;
    if (!out) return;
    drift_zero(out, (uint32_t)sizeof(*out));
    if (!s) return;

    root_step = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                               s->gravity.degree > 7u ? 7u : s->gravity.degree) * 1000;
    carrier = apsis_string_dsp_frequency_millihz(s, root_step);
    delta = apsis_drift_audible_delta_millihz(&s->drift);
    half = ((uint32_t)delta + 1u) / 2u;
    audible = (uint8_t)(transport_running && s->drift.sound && s->drift.level_q15);
    gain = audible ? drift_mul_q15(DRIFT_BASE_GAIN_Q15, s->drift.level_q15) : 0u;
    drift_equal_power_pair(s->drift.width_q15, &near_mix, &far_mix);

    out->freq_millihz[APSIS_DRIFT_DSP_LEFT] = carrier > half ? carrier - half : 1u;
    out->freq_millihz[APSIS_DRIFT_DSP_RIGHT] = carrier + half;
    out->gain_q15[APSIS_DRIFT_DSP_LEFT] = gain;
    out->gain_q15[APSIS_DRIFT_DSP_RIGHT] = gain;
    out->mix_left_q15[APSIS_DRIFT_DSP_LEFT] = near_mix;
    out->mix_right_q15[APSIS_DRIFT_DSP_LEFT] = far_mix;
    out->mix_left_q15[APSIS_DRIFT_DSP_RIGHT] = far_mix;
    out->mix_right_q15[APSIS_DRIFT_DSP_RIGHT] = near_mix;
}

void apsis_drift_dsp_reset(ApsisDriftDsp *dsp)
{
    if (!dsp) return;
    drift_zero(dsp, (uint32_t)sizeof(*dsp));
}

void apsis_drift_dsp_update(ApsisDriftDsp *dsp,
                            const ApsisState *s,
                            uint8_t transport_running)
{
    ApsisDriftDspTargets target;
    uint8_t i;
    if (!dsp) return;
    apsis_drift_dsp_targets(s, transport_running, &target);
    dsp->transport_running = transport_running ? 1u : 0u;
    for (i = 0u; i < APSIS_DRIFT_DSP_VOICES; ++i) {
        ApsisDriftDspVoice *v = &dsp->voice[i];
        uint32_t inc = drift_phase_inc_from_millihz(target.freq_millihz[i]);
        if (!v->pitch_initialized) {
            v->phase_inc = inc;
            v->mix_left_q15 = target.mix_left_q15[i];
            v->mix_right_q15 = target.mix_right_q15[i];
            v->pitch_initialized = 1u;
        }
        v->target_phase_inc = inc;
        v->target_gain_q23 = (int32_t)target.gain_q15[i] << 8;
        v->target_mix_left_q15 = target.mix_left_q15[i];
        v->target_mix_right_q15 = target.mix_right_q15[i];
    }
}

/* Each voice over the whole block in locals, added into out unsaturated, then one saturation: bit for bit the
 * old sample-outer loop, without a load and a store of every field each sample. */
void apsis_drift_dsp_render(ApsisDriftDsp *dsp, int32_t *out, uint32_t frames)
{
    uint32_t f;
    uint8_t i;
    if (!dsp || !out || !frames) return;
    for (i = 0u; i < APSIS_DRIFT_DSP_VOICES; ++i) {
        ApsisDriftDspVoice *v = &dsp->voice[i];
        uint32_t phase = v->phase, inc = v->phase_inc;
        const uint32_t target_inc = v->target_phase_inc;
        int32_t gain = v->gain_q23, mix_l = v->mix_left_q15, mix_r = v->mix_right_q15;
        const int32_t target_gain = v->target_gain_q23, target_l = v->target_mix_left_q15,
                      target_r = v->target_mix_right_q15;
        if (!v->pitch_initialized) continue;
        if (inc == target_inc && gain == target_gain && mix_l == target_l && mix_r == target_r) {
            /* settled: the smoothers would return their current values */
            const int32_t g = (gain + 128) >> 8;
            for (f = 0u; f < frames; ++f) {
                int32_t sample = (int32_t)(((int64_t)apsis_sine_q15(phase) * g) >> 15);
                out[f * 2u] += (int32_t)(((int64_t)sample * mix_l) >> 15);
                out[f * 2u + 1u] += (int32_t)(((int64_t)sample * mix_r) >> 15);
                phase += inc;
            }
        } else for (f = 0u; f < frames; ++f) {
            int32_t wave, sample;
            inc = drift_smooth_u32(inc, target_inc);
            gain = drift_smooth_i32(gain, target_gain);
            mix_l = drift_smooth_i32(mix_l, target_l);
            mix_r = drift_smooth_i32(mix_r, target_r);
            wave = apsis_sine_q15(phase);
            sample = (int32_t)(((int64_t)wave * ((gain + 128) >> 8)) >> 15);
            out[f * 2u] += (int32_t)(((int64_t)sample * mix_l) >> 15);
            out[f * 2u + 1u] += (int32_t)(((int64_t)sample * mix_r) >> 15);
            phase += inc;
        }
        v->phase = phase;
        v->phase_inc = inc;
        v->gain_q23 = gain;
        v->mix_left_q15 = mix_l;
        v->mix_right_q15 = mix_r;
    }
    for (f = 0u; f < frames * 2u; ++f)
        out[f] = apsis_sat_q15(out[f]);
}
