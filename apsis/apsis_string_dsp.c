#include "apsis_fixed.h"
#include "apsis_sine.h"
#include "apsis_string_dsp.h"
#include "apsis_harmony.h"
#include "apsis_drift.h"

#define Q20_ONE 1048576u
#define MONO_Q15 32767u
#define FOUNDATION_LP_ALPHA_Q15 4553u
#define CLASSIC_LP_ALPHA_Q15 7412u
#define GAIN_ALPHA_Q15 11u
#define TRANSPORT_RELEASE_ALPHA_Q15 17u
#define TIMBRE_RELEASE_ALPHA_Q15 30u

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
APSIS_INLINE int16_t sdsp_triangle_q15(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    if (p < 16384u) return (int16_t)((uint32_t)p * 2u);
    if (p < 49152u) return (int16_t)(32767 - (int32_t)(p - 16384u) * 2);
    return (int16_t)(-32767 + (int32_t)(p - 49152u) * 2);
}
APSIS_INLINE int16_t sdsp_waveform_q15(uint8_t waveform, uint32_t phase)
{
    if (waveform == APSIS_STRING_DSP_WAVE_TRIANGLE) return sdsp_triangle_q15(phase);
    return apsis_sine_q15(phase);
}
APSIS_INLINE uint32_t sdsp_smooth_u32(uint32_t current, uint32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - (int64_t)current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (uint32_t)((int64_t)current + delta);
}
APSIS_INLINE int32_t sdsp_smooth_i32(int32_t current, int32_t target, uint16_t alpha)
{
    int64_t diff = (int64_t)target - current;
    int64_t delta;
    if (!diff) return current;
    delta = (diff * alpha) >> 15;
    if (!delta) delta = diff > 0 ? 1 : -1;
    return (int32_t)((int64_t)current + delta);
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
    {
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
    }
}

APSIS_INLINE void sdsp_apply_timbre(ApsisStringDspVoice *v)
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
    uint32_t glide_ms;
    uint8_t i;
    if (!dsp || !s) return;
    apsis_string_dsp_targets(s, pitch_memory, transport_running, &targets);
    glide_ms = ((uint32_t)s->string.glide_q15 * 1000u + APSIS_Q15_ONE / 2u) /
               APSIS_Q15_ONE;
    if (glide_ms < 25u) glide_ms = 25u;
    if (glide_ms > 800u) glide_ms = 800u;
    dsp->transport_running = transport_running ? 1u : 0u;
    dsp->voice[0].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
    dsp->voice[1].pitch_alpha_q15 = sdsp_alpha_for_ms(12u);
    dsp->voice[2].pitch_alpha_q15 = sdsp_alpha_for_ms(glide_ms);
    dsp->voice[3].pitch_alpha_q15 = sdsp_alpha_for_ms(glide_ms);
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

/* Each voice over the whole block in locals, added into out unsaturated, then one saturation: the voices touch
 * only their own state, so this is the old sample-outer loop bit for bit, without a load and a store of every
 * field each sample (out[] could alias the voices). */
void apsis_string_dsp_render(ApsisStringDsp *dsp, int32_t *out, uint32_t frames)
{
    uint32_t n, i;
    if (!dsp || !out) return;
    for (i = 0u; i < APSIS_STRING_DSP_VOICES; ++i) {
        ApsisStringDspVoice *v = &dsp->voice[i];
        uint32_t phase = v->phase, inc = v->phase_inc;
        const uint32_t target_inc = v->target_phase_inc;
        int32_t gain = v->gain_q15, target_gain = v->target_gain_q15, lp = v->lowpass_q15;
        const uint16_t pitch_alpha = v->pitch_alpha_q15;
        uint16_t alpha = v->lowpass_alpha_q15, mix_l = v->mix_left_q15, mix_r = v->mix_right_q15;
        uint8_t waveform = v->waveform, transition = v->timbre_transition;
        if (!transition && inc == target_inc && gain == target_gain) {
            /* settled (most blocks): both smoothers would return their current value, so leave them out. Every
             * product fits 32 bits: |raw - lp| <= 65534 times alpha <= 32767, |lp| and gain <= 32767, mix <= 65535 */
            if (waveform == APSIS_STRING_DSP_WAVE_TRIANGLE) {
                for (n = 0u; n < frames; ++n) {
                    int32_t voiced;
                    phase += inc;
                    lp += ((sdsp_triangle_q15(phase) - lp) * (int32_t)alpha) >> 15;
                    voiced = (lp * gain) >> 15;
                    out[n * 2u] += (voiced * (int32_t)mix_l) >> 15;
                    out[n * 2u + 1u] += (voiced * (int32_t)mix_r) >> 15;
                }
            } else {
                for (n = 0u; n < frames; ++n) {
                    int32_t voiced;
                    phase += inc;
                    lp += ((apsis_sine_q15(phase) - lp) * (int32_t)alpha) >> 15;
                    voiced = (lp * gain) >> 15;
                    out[n * 2u] += (voiced * (int32_t)mix_l) >> 15;
                    out[n * 2u + 1u] += (voiced * (int32_t)mix_r) >> 15;
                }
            }
        } else for (n = 0u; n < frames; ++n) {
            int32_t raw, voiced;
            uint16_t gain_alpha;
            if (transition && gain <= 1) {          /* sdsp_apply_timbre */
                waveform = v->desired_waveform;
                alpha = v->desired_lowpass_alpha_q15;
                mix_l = v->desired_mix_left_q15;
                mix_r = v->desired_mix_right_q15;
                lp = 0;
                target_gain = v->desired_gain_q15;
                transition = 0u;
            }
            if (transition) gain_alpha = TIMBRE_RELEASE_ALPHA_Q15;
            else if (!dsp->transport_running && target_gain == 0) gain_alpha = TRANSPORT_RELEASE_ALPHA_Q15;
            else gain_alpha = GAIN_ALPHA_Q15;
            inc = sdsp_smooth_u32(inc, target_inc, pitch_alpha);
            gain = sdsp_smooth_i32(gain, target_gain, gain_alpha);
            phase += inc;
            raw = sdsp_waveform_q15(waveform, phase);
            lp += (int32_t)(((int64_t)(raw - lp) * alpha) >> 15);
            voiced = (int32_t)(((int64_t)lp * gain) >> 15);
            out[n * 2u] += (int32_t)(((int64_t)voiced * mix_l) >> 15);
            out[n * 2u + 1u] += (int32_t)(((int64_t)voiced * mix_r) >> 15);
        }
        v->phase = phase;
        v->phase_inc = inc;
        v->gain_q15 = gain;
        v->target_gain_q15 = target_gain;
        v->lowpass_q15 = lp;
        v->lowpass_alpha_q15 = alpha;
        v->mix_left_q15 = mix_l;
        v->mix_right_q15 = mix_r;
        v->waveform = waveform;
        v->timbre_transition = transition;
    }
    for (n = 0u; n < frames * 2u; ++n)
        out[n] = apsis_sat_q15(out[n]);
}
