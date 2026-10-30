#include "apsis_space_dsp.h"

#define SPACE_Q30_ONE 1073741824u
#define SPACE_EXP_RATIO_Q30 100913548u
#define SPACE_HAD_NORM_Q15 11585
#define SPACE_INPUT_Q15 12976
#define SPACE_OUTPUT_Q15 3277
#define SPACE_CROSS_SELF_Q15 28998
#define SPACE_CROSS_OTHER_Q15 3769
#define SPACE_MOD_DEPTH_Q8 2304

const uint16_t APSIS_SPACE_FDN_DELAY_SAMPLES[APSIS_SPACE_FDN_LINES] = {
    1871u, 1999u, 2137u, 2281u, 2441u, 2593u, 2767u, 2953u
};

static const uint32_t SPACE_LFO_INC[APSIS_SPACE_FDN_LINES] = {
    6915u, 8083u, 9447u, 10616u, 12369u, 13537u, 15290u, 17628u
};

static const int8_t SPACE_OUT_LEFT[APSIS_SPACE_FDN_LINES] = {
    1, 1, -1, 1, -1, 1, 1, -1
};
static const int8_t SPACE_OUT_RIGHT[APSIS_SPACE_FDN_LINES] = {
    1, -1, 1, 1, 1, -1, 1, -1
};

/* 1-exp(-2*pi*(hz*.65)/44100), Q15, 2200..6800 Hz in 100 Hz steps. */
static const uint16_t SPACE_DAMPING_ALPHA[47] = {
    6040u,6286u,6530u,6772u,7012u,7249u,7484u,7717u,
    7948u,8177u,8404u,8628u,8851u,9071u,9290u,9506u,
    9721u,9933u,10144u,10352u,10559u,10763u,10966u,11167u,
    11366u,11564u,11759u,11953u,12145u,12335u,12523u,12710u,
    12895u,13078u,13259u,13439u,13617u,13794u,13969u,14142u,
    14314u,14484u,14652u,14819u,14985u,15149u,15311u
};

static const int16_t SPACE_SINE_QUARTER[65] = {
    0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
    12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
    23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
    30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static int32_t space_clamp_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

static int32_t space_mul_q15(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    if (p >= 0) p += 16384;
    else p -= 16384;
    return (int32_t)(p >> 15);
}

static int16_t space_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return SPACE_SINE_QUARTER[o];
    if (q == 1u) return SPACE_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-SPACE_SINE_QUARTER[o];
    return (int16_t)-SPACE_SINE_QUARTER[64u - o];
}

static int16_t space_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = space_sine_point(i);
    int32_t b = space_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static uint16_t space_damping_alpha(uint16_t hz)
{
    uint32_t index, frac, a, b;
    if (hz <= 2200u) return SPACE_DAMPING_ALPHA[0];
    if (hz >= 6800u) return SPACE_DAMPING_ALPHA[46];
    index = (uint32_t)(hz - 2200u) / 100u;
    frac = (uint32_t)(hz - (uint16_t)(2200u + index * 100u));
    a = SPACE_DAMPING_ALPHA[index];
    b = SPACE_DAMPING_ALPHA[index + 1u];
    return (uint16_t)(a + ((b - a) * frac + 50u) / 100u);
}

/* Selected prototype feedback law:
 *   g = exp(-4.144653167 * delay_seconds / decay_seconds)
 * where the constant is -36 dB expressed as a natural-log amplitude decay.
 * delay/decay is formed with only a 32-bit division; the small exp argument
 * (<0.07 across the legal 4..12 s range) is evaluated by a cubic series. */
static uint16_t space_feedback_gain(uint16_t delay_samples, uint16_t decay_ms)
{
    uint32_t reciprocal_q30, ratio_q30, x_q30, x2_q30, x3_q30, g_q30;
    uint64_t wide;
    if (decay_ms < 4000u) decay_ms = 4000u;
    if (decay_ms > 12000u) decay_ms = 12000u;
    reciprocal_q30 = SPACE_Q30_ONE / decay_ms;
    ratio_q30 = (uint32_t)delay_samples * reciprocal_q30;
    wide = (uint64_t)ratio_q30 * SPACE_EXP_RATIO_Q30 + (1ull << 29);
    x_q30 = (uint32_t)(wide >> 30);
    x2_q30 = (uint32_t)(((uint64_t)x_q30 * x_q30 + (1ull << 29)) >> 30);
    x3_q30 = (uint32_t)(((uint64_t)x2_q30 * x_q30 + (1ull << 29)) >> 30);
    g_q30 = SPACE_Q30_ONE - x_q30 + x2_q30 / 2u - x3_q30 / 6u;
    g_q30 = (g_q30 + (1u << 14)) >> 15;
    return (uint16_t)(g_q30 > 32767u ? 32767u : g_q30);
}

static int32_t space_line_read(const ApsisSpaceFdn *fdn,
                               const ApsisSpaceFdnLine *line,
                               int32_t modulation_q8)
{
    int32_t delay_q8, read_q8, span_q8;
    uint32_t needed, i0, i1, frac;
    int32_t a, b;
    const int16_t *base;
    if (!fdn || !line || !fdn->storage_bound) return 0;
    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;
    if (delay_q8 < 256) delay_q8 = 256;
    needed = (uint32_t)(delay_q8 + 255) >> 8;
    if (line->fill_count < needed) return 0;

    span_q8 = (int32_t)line->capacity * 256;
    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;
    if (read_q8 < 0) read_q8 += span_q8;
    if (read_q8 >= span_q8) read_q8 -= span_q8;
    i0 = (uint32_t)read_q8 >> 8;
    frac = (uint32_t)read_q8 & 255u;
    i1 = i0 + 1u;
    if (i1 >= line->capacity) i1 = 0u;
    base = fdn->storage + line->offset;
    a = base[i0];
    if (!frac) return a;
    b = base[i1];
    return (a * (int32_t)(256u - frac) + b * (int32_t)frac) >> 8;
}

static void space_hadamard8(int32_t x[APSIS_SPACE_FDN_LINES])
{
    int32_t a0=x[0]+x[1], a1=x[0]-x[1], a2=x[2]+x[3], a3=x[2]-x[3];
    int32_t a4=x[4]+x[5], a5=x[4]-x[5], a6=x[6]+x[7], a7=x[6]-x[7];
    int32_t b0=a0+a2, b1=a1+a3, b2=a0-a2, b3=a1-a3;
    int32_t b4=a4+a6, b5=a5+a7, b6=a4-a6, b7=a5-a7;
    x[0]=b0+b4; x[1]=b1+b5; x[2]=b2+b6; x[3]=b3+b7;
    x[4]=b0-b4; x[5]=b1-b5; x[6]=b2-b6; x[7]=b3-b7;
}

uint8_t apsis_space_fdn_bind(ApsisSpaceFdn *fdn,
                             int16_t *storage,
                             uint32_t storage_samples)
{
    if (!fdn) return 0u;
    fdn->storage = storage;
    fdn->storage_samples = storage_samples;
    fdn->storage_bound = (storage && storage_samples >= APSIS_SPACE_FDN_STORAGE_SAMPLES) ? 1u : 0u;
    apsis_space_fdn_reset(fdn);
    return fdn->storage_bound;
}

void apsis_space_fdn_reset(ApsisSpaceFdn *fdn)
{
    uint32_t offset = 0u;
    uint8_t i;
    if (!fdn) return;
    fdn->post_left_q15 = 0;
    fdn->post_right_q15 = 0;
    for (i = 0u; i < APSIS_SPACE_FDN_LINES; ++i) {
        ApsisSpaceFdnLine *line = &fdn->line[i];
        line->offset = offset;
        line->base_delay = APSIS_SPACE_FDN_DELAY_SAMPLES[i];
        line->capacity = (uint16_t)(line->base_delay + APSIS_SPACE_FDN_MOD_RESERVE);
        line->write_pos = 0u;
        line->fill_count = 0u;
        line->lfo_phase = (uint32_t)i * 0x20000000u;
        line->lfo_phase_inc = SPACE_LFO_INC[i];
        line->damping_state_q15 = 0;
        offset += line->capacity;
    }
    if (offset != APSIS_SPACE_FDN_STORAGE_SAMPLES) fdn->storage_bound = 0u;
}

void apsis_space_fdn_render(ApsisSpaceFdn *fdn,
                            const int32_t *input_mono_q15,
                            int32_t *out_stereo,
                            uint32_t frames,
                            uint16_t decay_ms,
                            uint16_t damping_hz)
{
    uint16_t feedback_q15[APSIS_SPACE_FDN_LINES];
    uint16_t alpha;
    uint32_t frame;
    uint8_t k;
    if (!fdn || !fdn->storage_bound || !out_stereo) return;
    alpha = space_damping_alpha(damping_hz);
    for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k)
        feedback_q15[k] = space_feedback_gain(fdn->line[k].base_delay, decay_ms);

    for (frame = 0u; frame < frames; ++frame) {
        int32_t mixed[APSIS_SPACE_FDN_LINES];
        int32_t out_l = 0, out_r = 0;
        int32_t input = input_mono_q15 ? input_mono_q15[frame] : 0;

        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int32_t mod_q8, delayed;
            line->lfo_phase += line->lfo_phase_inc;
            mod_q8 = ((int32_t)space_sine_q15(line->lfo_phase) * SPACE_MOD_DEPTH_Q8) >> 15;
            delayed = space_line_read(fdn, line, mod_q8);
            line->damping_state_q15 += space_mul_q15(
                delayed - line->damping_state_q15, alpha);
            mixed[k] = line->damping_state_q15;
            out_l += (int32_t)SPACE_OUT_LEFT[k] * mixed[k];
            out_r += (int32_t)SPACE_OUT_RIGHT[k] * mixed[k];
        }

        fdn->post_left_q15 += space_mul_q15(
            space_mul_q15(out_l, SPACE_OUTPUT_Q15) - fdn->post_left_q15, alpha);
        fdn->post_right_q15 += space_mul_q15(
            space_mul_q15(out_r, SPACE_OUTPUT_Q15) - fdn->post_right_q15, alpha);

        out_stereo[frame * 2u] +=
            space_mul_q15(fdn->post_left_q15, SPACE_CROSS_SELF_Q15) +
            space_mul_q15(fdn->post_right_q15, SPACE_CROSS_OTHER_Q15);
        out_stereo[frame * 2u + 1u] +=
            space_mul_q15(fdn->post_right_q15, SPACE_CROSS_SELF_Q15) +
            space_mul_q15(fdn->post_left_q15, SPACE_CROSS_OTHER_Q15);

        space_hadamard8(mixed);
        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {
            ApsisSpaceFdnLine *line = &fdn->line[k];
            int16_t *base = fdn->storage + line->offset;
            int32_t feedback = space_mul_q15(mixed[k], SPACE_HAD_NORM_Q15);
            int32_t excitation = space_mul_q15(input, (k & 1u) ? -SPACE_INPUT_Q15 : SPACE_INPUT_Q15);
            int32_t write_value;
            feedback = space_mul_q15(feedback, feedback_q15[k]);
            write_value = space_clamp_q15((int64_t)excitation + feedback);
            base[line->write_pos] = (int16_t)write_value;
            line->write_pos++;
            if (line->write_pos >= line->capacity) line->write_pos = 0u;
            if (line->fill_count < line->capacity) line->fill_count++;
        }
    }
}
