#include "apsis_string_motion_dsp.h"
#include "apsis_drift.h"

static uint32_t p10_motion_pitch_frequency(const ApsisState *state,
                                           int32_t pitch_milli)
{
    ApsisState local;
    if (!state) return 0u;
    local = *state;
    local.gravity.scale_lock = 0u;
    local.temper.mode = APSIS_TUNING_EQUAL;
    local.temper.strength_q15 = 0u;
    return apsis_string_dsp_frequency_millihz(&local, pitch_milli);
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

    right = p10_motion_pitch_frequency(state, playback->right_pitch_milli);
    if (state->string.surface_mode == APSIS_STRING_CLASSIC) {
        uint32_t drift = apsis_drift_audible_delta_millihz(&state->drift);
        uint32_t half = (drift + 1u) / 2u;
        p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_A],
                             right > half ? right - half : 1u);
        p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_B], right + half);
        return;
    }

    left = p10_motion_pitch_frequency(state, playback->left_pitch_milli);
    p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_A], left);
    p10_motion_set_pitch(&dsp->voice[APSIS_STRING_DSP_PRIMARY_B], right);
}
