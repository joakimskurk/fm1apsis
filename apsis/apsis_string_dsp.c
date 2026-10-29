#include "apsis_string_dsp.h"
#include "apsis_harmony.h"
#include "apsis_drift.h"

#define Q20_ONE 1048576u
#define MONO_Q15 32767u
#define FOUNDATION_LP_ALPHA_Q15 4553u
#define CLASSIC_LP_ALPHA_Q15 7412u
#define BODY_LP_ALPHA_Q15 6493u
#define WIRE_LP_ALPHA_Q15 13148u
#define GAIN_ALPHA_Q15 11u
#define TRANSPORT_RELEASE_ALPHA_Q15 17u
#define TIMBRE_RELEASE_ALPHA_Q15 30u
#define BODY_LEFT_Q15 26955u
#define BODY_RIGHT_Q15 18630u
#define WIRE_LEFT_Q15 18630u
#define WIRE_RIGHT_Q15 26955u

static const int16_t SDSP_SINE_QUARTER[65] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767
};
static const uint32_t SDSP_SEMITONE_Q20[12] = {
1048576u,1110928u,1176987u,1246974u,1321123u,1399681u,
1482910u,1571089u,1664511u,1763488u,1868350u,1979448u
};
static const uint32_t SDSP_CENT_Q20[101] = {
1048576u,1049182u,1049788u,1050395u,1051002u,1051609u,1052216u,1052824u,1053433u,1054041u,
1054650u,1055260u,1055869u,1056479u,1057090u,1057701u,1058312u,1058923u,1059535u,1060147u,
1060760u,1061373u,1061986u,1062600u,1063214u,1063828u,1064443u,1065058u,1065673u,1066289u,
1066905u,1067521u,1068138u,1068755u,1069373u,1069991u,1070609u,1071227u,1071846u,1072466u,
1073085u,1073705u,1074326u,1074946u,1075568u,1076189u,1076811u,1077433u,1078055u,1078678u,
1079302u,1079925u,1080549u,1081174u,1081798u,1082423u,1083049u,1083674u,1084301u,1084927u,
1085554u,1086181u,1086809u,1087437u,1088065u,1088694u,1089323u,1089952u,1090582u,1091212u,
1091843u,1092473u,1093105u,1093736u,1094368u,1095000u,1095633u,1096266u,1096900u,1097533u,
1098167u,1098802u,1099437u,1100072u,1100708u,1101344u,1101980u,1102617u,1103254u,1103891u,
1104529u,1105167u,1105806u,1106445u,1107084u,1107724u,1108364u,1109004u,1109645u,1110286u,1110928u
};
static const uint16_t SDSP_BODY_COEFF_Q15[7] = {
    19919u, 6175u, 2988u, 1693u, 1036u, 598u, 359u
};

static void sdsp_zero_bytes(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}
static int32_t sdsp_clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
static uint16_t sdsp_mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}
static int32_t sdsp_div100_round(int32_t v)
{
    return v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100);
}
static uint8_t sdsp_pitch_class(int32_t step)
{
    int32_t pc = step % 12;
    if (pc < 0) pc += 12;
    return (uint8_t)pc;
}
static uint32_t sdsp_equal_frequency_millihz(int32_t step_milli)
{
    int32_t semitone, frac, octave, pc;
    uint32_t f, r, idx, rem;
    step_milli = sdsp_clamp_i32(step_milli, -60000, 84000);
    semitone = step_milli / 1000;
    frac = step_milli % 1000;
    if (frac < 0) { frac += 1000; --semitone; }
    octave = semitone / 12;
    pc = semitone % 12;
    if (pc < 0) { pc += 12; --octave; }
    f = (uint32_t)(((uint64_t)130813u * SDSP_SEMITONE_Q20[pc] + Q20_ONE / 2u) >> 20);
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
    r = SDSP_CENT_Q20[idx];
    if (rem && idx < 100u)
        r += ((SDSP_CENT_Q20[idx + 1u] - r) * rem + 5u) / 10u;
    return (uint32_t)(((uint64_t)f * r + Q20_ONE / 2u) >> 20);
}
static uint32_t sdsp_frequency_millihz(const ApsisState *s, int32_t step_milli)
{
    int32_t whole, dev = 0;
    if (!s) return 0u;
    if ((step_milli % 1000) == 0) {
        whole = step_milli / 1000;
        whole = apsis_quantize_step_to_scale(whole, s->gravity.tonic, s->gravity.scale,
                                             s->gravity.scale_lock);
        step_milli = whole * 1000;
        dev = apsis_tuning_deviation_milli(sdsp_pitch_class(whole), s->gravity.tonic,
                                           s->temper.mode, s->temper.strength_q15);
        step_milli += sdsp_div100_round(dev);
    }
    return sdsp_equal_frequency_millihz(step_milli);
}
static uint32_t sdsp_phase_inc_from_millihz(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}
static uint16_t sdsp_alpha_for_ms(uint32_t ms)
{
    uint32_t samples, a;
    if (ms < 1u) ms = 1u;
    samples = (APSIS_STRING_DSP_RATE * ms + 500u) / 1000u;
    if (!samples) samples = 1u;
    if (samples <= 1u) return APSIS_Q15_ONE;
    a = (APSIS_Q15_ONE + samples / 2u) / samples;
    return (uint16_t)(a ? a : 1u);
}
static int16_t sdsp_sine_point(uint8_t index)
{
    uint8_t q = index >> 6;
    uint8_t o = index & 63u;
    if (q == 0u) return SDSP_SINE_QUARTER[o];
    if (q == 1u) return SDSP_SINE_QUARTER[64u - o];
    if (q == 2u) return (int16_t)-SDSP_SINE_QUARTER[o];
    return (int16_t)-SDSP_SINE_QUARTER[64u - o];
}
static int16_t sdsp_sine_q15(uint32_t phase)
{
    uint8_t i = (uint8_t)(phase >> 24);
    uint8_t frac = (uint8_t)(phase >> 16);
    int32_t a = sdsp_sine_point(i);
    int32_t b = sdsp_sine_point((uint8_t)(i + 1u));
    return (int16_t)(a + (((b - a) * frac) >> 8));
}
static int16_t sdsp_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}
static int16_t sdsp_body_q15(uint32_t phase)
{
    int64_t sum = 0;
    uint8_t h;
    for (h = 1u; h <= 7u; ++h) {
        uint32_t hp = phase * (uint32_t)h + 0x40000000u;
        sum += (int64_t)sdsp_sine_q15(hp) * SDSP_BODY_COEFF_Q15[h - 1u];
    }
    sum >>= 15;
    if (sum > 32767) sum = 32767;
    if (sum < -32767) sum = -32767;
    return (int16_t)sum;
}
static int16_t sdsp_waveform_q15(uint8_t waveform, uint32_t phase)
{
    if (waveform == APSIS_STRING_DSP_WAVE_TRIANGLE) return sdsp_triangle_q15(phase);
    if (waveform == APSIS_STRING_DSP_WAVE_BODY) return sdsp_body_q15(phase);
    return sdsp_sine_q15(phase);
}
static uint32_t sdsp_smooth_u32(uint32_t current, uint32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - (int64_t)current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (uint32_t)((int64_t)current + delta);
}
static int32_t sdsp_smooth_i32(int32_t current, int32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (int32_t)((int64_t)current + delta);
}
static int32_t sdsp_sat_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
}
static void sdsp_target_common(ApsisStringDspTargets *out, uint8_t i, uint32_t freq,
                               uint16_t gain, uint8_t waveform, uint16_t lp,
                               uint16_t mix_l, uint16_t mix_r, uint8_t audible)
{
    out->freq_millihz[i] = freq;
    out->gain_q15[i] = audible ? gain : 0u;
    out->waveform[i] = waveform;
    out->lowpass_alpha_q15[i] = lp;
    out->mix_left_q15[i] = mix_l;
    out->mix_right_q15[i] = mix_r;
    out->enabled[i] = 1u;
}

void apsis_string_dsp_targets(const ApsisState *s,
                              ApsisStringPitchMemory *pitch_memory,
                              uint8_t transport_running,
                              ApsisStringDspTargets *out)
{
    uint16_t string_level, drift, foundation_gain;
    uint32_t root, fd;
    int32_t root_step;
    uint8_t audible;
    if (!out) return;
    sdsp_zero_bytes(out, (uint32_t)sizeof(*out));
    if (!s) return;
    string_level = s->string.level_q15;
    audible = (uint8_t)(transport_running && s->string.sound && string_level);
    drift = apsis_drift_audible_delta_millihz(&s->drift);
    root_step = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                               s->gravity.degree > 7u ? 7 : s->gravity.degree) * 1000 +
                (int32_t)s->string.foundation_octave * 12000;
    root = sdsp_frequency_millihz(s, root_step);
    fd = ((uint32_t)drift * 55u + 50u) / 100u;
    foundation_gain = sdsp_mul_q15(sdsp_mul_q15(APSIS_Q15_MILLI(16),
                                                s->string.foundation_level_q15), string_level);
    if (s->string.foundation) {
        sdsp_target_common(out, 0u, root > fd ? root - fd : 1u, foundation_gain,
                           APSIS_STRING_DSP_WAVE_SINE, FOUNDATION_LP_ALPHA_Q15,
                           MONO_Q15, MONO_Q15, audible);
        sdsp_target_common(out, 1u, root + fd, foundation_gain,
                           APSIS_STRING_DSP_WAVE_SINE, FOUNDATION_LP_ALPHA_Q15,
                           MONO_Q15, MONO_Q15, audible);
    }
    if (s->string.surface_mode == APSIS_STRING_CLASSIC) {
        int32_t centre_step = apsis_string_target_step_milli(s, APSIS_SIDE_RIGHT,
                                                              pitch_memory, 1u);
        uint32_t centre = sdsp_frequency_millihz(s, centre_step);
        uint32_t half = ((uint32_t)drift + 1u) / 2u;
        uint16_t gain = sdsp_mul_q15(APSIS_Q15_MILLI(20), string_level);
        sdsp_target_common(out, 2u, centre > half ? centre - half : 1u, gain,
                           APSIS_STRING_DSP_WAVE_SINE, CLASSIC_LP_ALPHA_Q15,
                           MONO_Q15, MONO_Q15, audible);
        sdsp_target_common(out, 3u, centre + half, gain,
                           APSIS_STRING_DSP_WAVE_SINE, CLASSIC_LP_ALPHA_Q15,
                           MONO_Q15, MONO_Q15, audible);
    } else {
        int32_t left_step = apsis_string_target_step_milli(s, APSIS_SIDE_LEFT,
                                                            pitch_memory, 1u);
        int32_t right_step = apsis_string_target_step_milli(s, APSIS_SIDE_RIGHT,
                                                             pitch_memory, 1u);
        uint32_t left = sdsp_frequency_millihz(s, left_step);
        uint32_t right = sdsp_frequency_millihz(s, right_step);
        uint16_t body_gain = sdsp_mul_q15(sdsp_mul_q15(APSIS_Q15_MILLI(20),
                                                       s->string.left_level_q15), string_level);
        uint16_t wire_gain = sdsp_mul_q15(508u,
                                          sdsp_mul_q15(s->string.right_level_q15, string_level));
        sdsp_target_common(out, 2u, left, body_gain, APSIS_STRING_DSP_WAVE_BODY,
                           BODY_LP_ALPHA_Q15, BODY_LEFT_Q15, BODY_RIGHT_Q15, audible);
        sdsp_target_common(out, 3u, right, wire_gain, APSIS_STRING_DSP_WAVE_TRIANGLE,
                           WIRE_LP_ALPHA_Q15, WIRE_LEFT_Q15, WIRE_RIGHT_Q15, audible);
    }
}

static void sdsp_apply_timbre(ApsisStringDspVoice *v)
{
    v->waveform = v->desired_waveform;
    v->lowpass_alpha_q15 = v->desired_lowpass_alpha_q15;
    v->mix_left_q15 = v->desired_mix_left_q15;
    v->mix_right_q15 = v->desired_mix_right_q15;
    v->lowpass_q15 = 0;
    v->target_gain_q15 = v->desired_gain_q15;
    v->timbre_transition = 0u;
}

void apsis_string_dsp_reset(ApsisStringDsp *dsp)
{
    uint8_t i;
    if (!dsp) return;
    sdsp_zero_bytes(dsp, (uint32_t)sizeof(*dsp));
    for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
        dsp->voice[i].waveform = APSIS_STRING_DSP_WAVE_SINE;
        dsp->voice[i].desired_waveform = APSIS_STRING_DSP_WAVE_SINE;
        dsp->voice[i].mix_left_q15 = MONO_Q15;
        dsp->voice[i].mix_right_q15 = MONO_Q15;
        dsp->voice[i].desired_mix_left_q15 = MONO_Q15;
        dsp->voice[i].desired_mix_right_q15 = MONO_Q15;
    }
    dsp->voice[0].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
    dsp->voice[1].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
}

void apsis_string_dsp_update(ApsisStringDsp *dsp,
                             const ApsisState *s,
                             ApsisStringPitchMemory *pitch_memory,
                             uint8_t transport_running)
{
    ApsisStringDspTargets targets;
    uint32_t glide_ms, wire_ms;
    uint8_t i;
    if (!dsp || !s) return;
    apsis_string_dsp_targets(s, pitch_memory, transport_running, &targets);
    glide_ms = ((uint32_t)s->string.glide_q15 * 1000u + APSIS_Q15_ONE / 2u) /
               APSIS_Q15_ONE;
    if (glide_ms < 25u) glide_ms = 25u;
    if (glide_ms > 800u) glide_ms = 800u;
    wire_ms = glide_ms > 45u ? 45u : glide_ms;
    dsp->transport_running = transport_running ? 1u : 0u;
    dsp->voice[0].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
    dsp->voice[1].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
    dsp->voice[2].pitch_alpha_q15 = sdsp_alpha_for_ms(glide_ms);
    dsp->voice[3].pitch_alpha_q15 = sdsp_alpha_for_ms(
        s->string.surface_mode == APSIS_STRING_CLASSIC ? glide_ms : wire_ms);
    for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
        ApsisStringDspVoice *v = &dsp->voice[i];
        uint8_t metadata_changed;
        v->target_phase_inc = sdsp_phase_inc_from_millihz(targets.freq_millihz[i]);
        v->desired_gain_q15 = targets.gain_q15[i];
        v->desired_waveform = targets.waveform[i];
        v->desired_lowpass_alpha_q15 = targets.lowpass_alpha_q15[i];
        v->desired_mix_left_q15 = targets.mix_left_q15[i];
        v->desired_mix_right_q15 = targets.mix_right_q15[i];
        if (!v->pitch_initialized && v->target_phase_inc) {
            v->phase_inc = v->target_phase_inc;
            v->pitch_initialized = 1u;
        }
        metadata_changed = (uint8_t)(
            v->waveform != v->desired_waveform ||
            v->lowpass_alpha_q15 != v->desired_lowpass_alpha_q15 ||
            v->mix_left_q15 != v->desired_mix_left_q15 ||
            v->mix_right_q15 != v->desired_mix_right_q15);
        if (metadata_changed) {
            if (v->gain_q15 <= 1) sdsp_apply_timbre(v);
            else { v->target_gain_q15 = 0; v->timbre_transition = 1u; }
        } else if (!v->timbre_transition) {
            v->target_gain_q15 = v->desired_gain_q15;
        }
    }
}

void apsis_string_dsp_render(ApsisStringDsp *dsp, int32_t *out, uint32_t frames)
{
    uint32_t n, i;
    if (!dsp || !out) return;
    for (n = 0u; n < frames; ++n) {
        int64_t mix_l = 0, mix_r = 0;
        for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
            ApsisStringDspVoice *v = &dsp->voice[i];
            int32_t raw, voiced;
            uint16_t gain_alpha;
            if (v->timbre_transition && v->gain_q15 <= 1) sdsp_apply_timbre(v);
            if (v->timbre_transition) gain_alpha = TIMBRE_RELEASE_ALPHA_Q15;
            else if (!dsp->transport_running && v->target_gain_q15 == 0)
                gain_alpha = TRANSPORT_RELEASE_ALPHA_Q15;
            else gain_alpha = GAIN_ALPHA_Q15;
            v->phase_inc = sdsp_smooth_u32(v->phase_inc, v->target_phase_inc,
                                           v->pitch_alpha_q15);
            v->gain_q15 = sdsp_smooth_i32(v->gain_q15, v->target_gain_q15, gain_alpha);
            v->phase += v->phase_inc;
            raw = sdsp_waveform_q15(v->waveform, v->phase);
            v->lowpass_q15 += (int32_t)(((int64_t)(raw - v->lowpass_q15) *
                                          v->lowpass_alpha_q15) >> 15);
            voiced = (int32_t)(((int64_t)v->lowpass_q15 * v->gain_q15) >> 15);
            mix_l += ((int64_t)voiced * v->mix_left_q15) >> 15;
            mix_r += ((int64_t)voiced * v->mix_right_q15) >> 15;
        }
        out[n * 2u] = sdsp_sat_q15((int64_t)out[n * 2u] + mix_l);
        out[n * 2u + 1u] = sdsp_sat_q15((int64_t)out[n * 2u + 1u] + mix_r);
    }
}
