#include "apsis_string_dsp.h"
#include "apsis_harmony.h"
#include "apsis_drift.h"

#define Q20_ONE 1048576u
#define FOUNDATION_LP_ALPHA_Q15 4553u
#define CLASSIC_LP_ALPHA_Q15 7412u
#define GAIN_ATTACK_ALPHA_Q15 11u
#define GAIN_RELEASE_ALPHA_Q15 17u

static const int16_t SINE_QUARTER[65] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};

static const uint32_t SEMITONE_Q20[12] = {
1048576u,1110928u,1176987u,1246974u,1321123u,1399681u,
1482910u,1571089u,1664511u,1763488u,1868350u,1979448u
};

/* 2^(cents/1200), Q20, one entry per cent from 0..100. */
static const uint32_t CENT_Q20[101] = {
1048576u,1049182u,1049788u,1050395u,1051002u,1051609u,1052216u,1052824u,1053433u,1054041u,
1054650u,1055260u,1055870u,1056480u,1057091u,1057702u,1058314u,1058926u,1059538u,1060151u,
1060764u,1061378u,1061992u,1062607u,1063222u,1063837u,1064453u,1065069u,1065686u,1066303u,
1066921u,1067539u,1068157u,1068776u,1069396u,1070016u,1070636u,1071257u,1071878u,1072500u,
1073122u,1073745u,1074368u,1074991u,1075615u,1076240u,1076865u,1077490u,1078116u,1078742u,
1079369u,1079996u,1080624u,1081252u,1081881u,1082510u,1083139u,1083769u,1084400u,1085031u,
1085662u,1086294u,1086926u,1087559u,1088193u,1088827u,1089461u,1090096u,1090731u,1091367u,
1092003u,1092640u,1093277u,1093915u,1094553u,1095192u,1095831u,1096471u,1097111u,1097752u,
1098393u,1099035u,1099677u,1100320u,1100963u,1101607u,1102251u,1102896u,1103541u,1104187u,
1104833u,1105480u,1106127u,1106775u,1107423u,1108072u,1108721u,1109371u,1110021u,1110672u,
1111323u
};

static void zero_bytes(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint16_t mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static int32_t div100_round(int32_t v)
{
    return v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100);
}

static uint8_t pitch_class(int32_t step)
{
    int32_t pc = step % 12;
    if (pc < 0) pc += 12;
    return (uint8_t)pc;
}

static uint32_t equal_frequency_millihz(int32_t step_milli)
{
    int32_t semitone, frac, octave, pc;
    uint32_t f, r, idx, rem;
    step_milli = clamp_i32(step_milli, -60000, 84000);
    semitone = step_milli / 1000;
    frac = step_milli % 1000;
    if (frac < 0) { frac += 1000; --semitone; }
    octave = semitone / 12;
    pc = semitone % 12;
    if (pc < 0) { pc += 12; --octave; }

    f = (uint32_t)(((uint64_t)130813u * SEMITONE_Q20[pc] + (Q20_ONE / 2u)) >> 20);
    if (octave >= 0) {
        if (octave > 7) octave = 7;
        f <<= (uint32_t)octave;
    } else {
        uint32_t sh = (uint32_t)(-octave);
        if (sh > 12u) sh = 12u;
        f = (f + (1u << (sh ? sh - 1u : 0u))) >> sh;
    }

    idx = (uint32_t)frac / 10u;
    rem = (uint32_t)frac - idx * 10u;
    r = CENT_Q20[idx];
    if (rem && idx < 100u)
        r += (uint32_t)(((uint64_t)(CENT_Q20[idx + 1u] - r) * rem + 5u) / 10u);
    return (uint32_t)(((uint64_t)f * r + (Q20_ONE / 2u)) >> 20);
}

static uint32_t frequency_millihz(const ApsisState *s, int32_t step_milli)
{
    int32_t whole, dev = 0;
    if (!s) return 0u;
    if ((step_milli % 1000) == 0) {
        whole = step_milli / 1000;
        whole = apsis_quantize_step_to_scale(whole, s->gravity.tonic, s->gravity.scale,
                                             s->gravity.scale_lock);
        step_milli = whole * 1000;
        dev = apsis_tuning_deviation_milli(pitch_class(whole), s->gravity.tonic,
                                           s->temper.mode, s->temper.strength_q15);
        step_milli += div100_round(dev); /* milli-cent -> milli-semitone */
    }
    return equal_frequency_millihz(step_milli);
}

static uint32_t phase_inc_from_millihz(uint32_t mhz)
{
    /* 2^32 / (44100 * 1000) = 97.39154875... . Q16 constant keeps
     * the target path to multiply+shift only; no __udivdi3 is required. */
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint16_t alpha_for_ms(uint32_t ms)
{
    uint32_t samples;
    if (ms < 1u) ms = 1u;
    samples = (APSIS_STRING_DSP_RATE * ms + 500u) / 1000u;
    if (!samples) samples = 1u;
    if (samples <= 1u) return APSIS_Q15_ONE;
    {
        uint32_t a = (APSIS_Q15_ONE + samples / 2u) / samples;
        return (uint16_t)(a ? a : 1u);
    }
}

static int16_t sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return SINE_QUARTER[o];
    if (q == 1u) return SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-SINE_QUARTER[o];
    return (int16_t)-SINE_QUARTER[64u - o];
}

static int16_t sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = sine_point(i);
    int32_t b = sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}

static uint32_t smooth_u32(uint32_t current, uint32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - (int64_t)current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (uint32_t)((int64_t)current + delta);
}

static int32_t smooth_i32(int32_t current, int32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (int32_t)((int64_t)current + delta);
}

static int32_t sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}

void apsis_string_dsp_targets(const ApsisState *s,
                              ApsisStringPitchMemory *pitch_memory,
                              uint8_t transport_running,
                              ApsisStringDspTargets *out)
{
    uint16_t string_level, foundation_gain, classic_gain;
    uint16_t drift;
    uint32_t root, centre, fd, half;
    int32_t root_step, centre_step;
    uint8_t audible;
    if (!out) return;
    zero_bytes(out, (uint32_t)sizeof(*out));
    if (!s) return;

    drift = apsis_drift_audible_delta_millihz(&s->drift);
    root_step = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                               s->gravity.degree > 7u ? 7 : s->gravity.degree) * 1000 +
                (int32_t)s->string.foundation_octave * 12000;
    root = frequency_millihz(s, root_step);
    centre_step = apsis_string_target_step_milli(s, APSIS_SIDE_RIGHT, pitch_memory, 1u);
    centre = frequency_millihz(s, centre_step);
    fd = ((uint32_t)drift * 55u + 50u) / 100u;
    half = ((uint32_t)drift + 1u) / 2u;

    out->freq_millihz[APSIS_STRING_DSP_FOUNDATION_LO] = root > fd ? root - fd : 1u;
    out->freq_millihz[APSIS_STRING_DSP_FOUNDATION_HI] = root + fd;
    out->freq_millihz[APSIS_STRING_DSP_CLASSIC_LO] = centre > half ? centre - half : 1u;
    out->freq_millihz[APSIS_STRING_DSP_CLASSIC_HI] = centre + half;

    string_level = s->string.level_q15;
    foundation_gain = mul_q15(mul_q15(APSIS_Q15_MILLI(16), s->string.foundation_level_q15), string_level);
    classic_gain = mul_q15(APSIS_Q15_MILLI(20), string_level);
    audible = (uint8_t)(transport_running && s->string.sound && string_level);

    if (s->string.foundation) {
        out->enabled[0] = out->enabled[1] = 1u;
        if (audible) out->gain_q15[0] = out->gain_q15[1] = foundation_gain;
    }
    if (s->string.surface_mode == APSIS_STRING_CLASSIC) {
        out->enabled[2] = out->enabled[3] = 1u;
        if (audible) out->gain_q15[2] = out->gain_q15[3] = classic_gain;
    }
}

void apsis_string_dsp_reset(ApsisStringDsp *dsp)
{
    if (!dsp) return;
    zero_bytes(dsp, (uint32_t)sizeof(*dsp));
    dsp->voice[0].lowpass_alpha_q15 = FOUNDATION_LP_ALPHA_Q15;
    dsp->voice[1].lowpass_alpha_q15 = FOUNDATION_LP_ALPHA_Q15;
    dsp->voice[2].lowpass_alpha_q15 = CLASSIC_LP_ALPHA_Q15;
    dsp->voice[3].lowpass_alpha_q15 = CLASSIC_LP_ALPHA_Q15;
    dsp->voice[0].pitch_alpha_q15 = alpha_for_ms(12u);
    dsp->voice[1].pitch_alpha_q15 = alpha_for_ms(12u);
    dsp->voice[2].pitch_alpha_q15 = alpha_for_ms(55u);
    dsp->voice[3].pitch_alpha_q15 = alpha_for_ms(55u);
}

void apsis_string_dsp_update(ApsisStringDsp *dsp,
                             const ApsisState *s,
                             ApsisStringPitchMemory *pitch_memory,
                             uint8_t transport_running)
{
    ApsisStringDspTargets targets;
    uint32_t glide_ms;
    uint8_t i;
    if (!dsp || !s) return;
    apsis_string_dsp_targets(s, pitch_memory, transport_running, &targets);
    glide_ms = ((uint32_t)s->string.glide_q15 * 1000u + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    if (glide_ms < 25u) glide_ms = 25u;
    if (glide_ms > 800u) glide_ms = 800u;
    dsp->voice[2].pitch_alpha_q15 = alpha_for_ms(glide_ms);
    dsp->voice[3].pitch_alpha_q15 = alpha_for_ms(glide_ms);
    dsp->transport_running = transport_running ? 1u : 0u;

    for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
        ApsisStringDspVoice *v = &dsp->voice[i];
        v->target_phase_inc = phase_inc_from_millihz(targets.freq_millihz[i]);
        v->target_gain_q15 = targets.gain_q15[i];
        if (!v->pitch_initialized && v->target_phase_inc) {
            v->phase_inc = v->target_phase_inc;
            v->pitch_initialized = 1u;
        }
    }
}

void apsis_string_dsp_render(ApsisStringDsp *dsp, int32_t *out, uint32_t frames)
{
    uint32_t n, i;
    if (!dsp || !out) return;
    for (n = 0u; n < frames; ++n) {
        int64_t mix = 0;
        for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
            ApsisStringDspVoice *v = &dsp->voice[i];
            int32_t raw, filtered, voiced;
            uint16_t ga = (!dsp->transport_running && v->target_gain_q15 == 0) ?
                          GAIN_RELEASE_ALPHA_Q15 : GAIN_ATTACK_ALPHA_Q15;
            v->phase_inc = smooth_u32(v->phase_inc, v->target_phase_inc, v->pitch_alpha_q15);
            v->gain_q15 = smooth_i32(v->gain_q15, v->target_gain_q15, ga);
            v->phase += v->phase_inc;
            raw = sine_q15(v->phase);
            v->lowpass_q15 += (int32_t)(((int64_t)(raw - v->lowpass_q15) * v->lowpass_alpha_q15) >> 15);
            filtered = v->lowpass_q15;
            voiced = (int32_t)(((int64_t)filtered * v->gain_q15) >> 15);
            mix += voiced;
        }
        out[n * 2u] = sat_q15((int64_t)out[n * 2u] + mix);
        out[n * 2u + 1u] = sat_q15((int64_t)out[n * 2u + 1u] + mix);
    }
}
