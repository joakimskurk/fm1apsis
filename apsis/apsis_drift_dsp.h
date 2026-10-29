#ifndef APSIS_DRIFT_DSP_H
#define APSIS_DRIFT_DSP_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_DRIFT_DSP_RATE 44100u
#define APSIS_DRIFT_DSP_VOICES 2u

typedef enum {
    APSIS_DRIFT_DSP_LEFT = 0,
    APSIS_DRIFT_DSP_RIGHT
} ApsisDriftDspVoiceId;

typedef struct {
    uint32_t freq_millihz[APSIS_DRIFT_DSP_VOICES];
    uint16_t gain_q15[APSIS_DRIFT_DSP_VOICES];
    uint16_t mix_left_q15[APSIS_DRIFT_DSP_VOICES];
    uint16_t mix_right_q15[APSIS_DRIFT_DSP_VOICES];
} ApsisDriftDspTargets;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t target_phase_inc;
    int32_t gain_q23;
    int32_t target_gain_q23;
    int32_t mix_left_q15;
    int32_t target_mix_left_q15;
    int32_t mix_right_q15;
    int32_t target_mix_right_q15;
    uint8_t pitch_initialized;
} ApsisDriftDspVoice;

typedef struct {
    ApsisDriftDspVoice voice[APSIS_DRIFT_DSP_VOICES];
    uint8_t transport_running;
} ApsisDriftDsp;

/* Source-resolved persistent two-sine beating field around the live,
 * scale/tuning-aware Gravity root. */
void apsis_drift_dsp_targets(const ApsisState *state,
                             uint8_t transport_running,
                             ApsisDriftDspTargets *out);

void apsis_drift_dsp_reset(ApsisDriftDsp *dsp);
void apsis_drift_dsp_update(ApsisDriftDsp *dsp,
                            const ApsisState *state,
                            uint8_t transport_running);
void apsis_drift_dsp_render(ApsisDriftDsp *dsp, int32_t *out, uint32_t frames);

#endif
