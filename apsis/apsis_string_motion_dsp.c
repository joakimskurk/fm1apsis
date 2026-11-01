#include "apsis_string_motion_dsp.h"
#include "apsis_drift.h"

#define P10_MOTION_Q20_ONE 1048576u

/* Exact M4a equal-temperament lookup values, copied into the P10-only layer so
 * frozen motion pitch never depends on current Gravity/Tuning and historical
 * M4a source blobs remain byte-for-byte untouched. */
static const uint32_t P10_MOTION_SEMITONE_Q20[12] = {
    1048576u,1110928u,1176987u,1246974u,1321123u,1399681u,
    1482910u,1571089u,1664511u,1763488u,1868350u,1979448u
};
static const uint32_t P10_MOTION_CENT_Q20[101] = {
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

static int32_t p10_motion_clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint32_t p10_motion_pitch_frequency(int32_t pitch_milli)
{
    int32_t semitone, frac, octave, pc;
    uint32_t f, ratio, index, rem;

    pitch_milli = p10_motion_clamp_i32(pitch_milli, -60000, 84000);
    semitone = pitch_milli / 1000;
    frac = pitch_milli % 1000;
    if (frac < 0) {
        frac += 1000;
        --semitone;
    }
    octave = semitone / 12;
    pc = semitone % 12;
    if (pc < 0) {
        pc += 12;
        --octave;
    }

    f = (uint32_t)(((uint64_t)130813u * P10_MOTION_SEMITONE_Q20[pc] +
                    P10_MOTION_Q20_ONE / 2u) >> 20);
    if (octave >= 0) {
        if (octave > 7) octave = 7;
        f <<= (uint32_t)octave;
    } else {
        uint32_t shift = (uint32_t)(-octave);
        if (shift > 12u) shift = 12u;
        f = (f + (1u << (shift ? shift - 1u : 0u))) >> shift;
    }

    index = (uint32_t)frac / 10u;
    rem = (uint32_t)frac - index * 10u;
    ratio = P10_MOTION_CENT_Q20[index];
    if (rem && index < 100u)
        ratio += ((P10_MOTION_CENT_Q20[index + 1u] - ratio) * rem + 5u) / 10u;
    return (uint32_t)(((uint64_t)f * ratio + P10_MOTION_Q20_ONE / 2u) >> 20);
}

static uint32_t p10_motion_phase_inc(uint32_t millihz)
{
    return (uint32_t)(((uint64_t)millihz * 6382653u + 32768u) >> 16);
}

static void p10_motion_set_pitch(ApsisStringDspVoice *voice, uint32_t millihz)
{
    uint32_t phase_inc;
    if (!voice || !millihz) return;
    phase_inc = p10_motion_phase_inc(millihz);
    voice->target_phase_inc = phase_inc;
    if (!voice->pitch_initialized) {
        voice->phase_inc = phase_inc;
        voice->pitch_initialized = 1u;
    }
}

void apsis_string_motion_dsp_override(ApsisStringDsp *dsp,
                                      const ApsisState *state,
                                      const ApsisStringMotionPlayback *playback)
{
    uint32_t left, right;
    if (!dsp || !state || !playback) return;

    right = p10_motion_pitch_frequency(playback->right_pitch_milli);
    if (state->string.surface_mode == APSIS_STRING_CLASSIC) {
        uint32_t drift = apsis_drift_audible_delta_millihz(&state->drift);
        uint32_t half = (drift + 1u) / 2u;
        p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_A],
                             right > half ? right - half : 1u);
        p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_B], right + half);
        return;
    }

    left = p10_motion_pitch_frequency(playback->left_pitch_milli);
    p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_A], left);
    p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_B], right);
}
