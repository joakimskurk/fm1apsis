#ifndef APSIS_STRING_DSP_H
#define APSIS_STRING_DSP_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_string.h"

#define APSIS_STRING_DSP_RATE 44100u
#define APSIS_STRING_DSP_VOICES 4u

typedef enum {
    APSIS_STRING_DSP_FOUNDATION_LO = 0,
    APSIS_STRING_DSP_FOUNDATION_HI,
    APSIS_STRING_DSP_CLASSIC_LO,
    APSIS_STRING_DSP_CLASSIC_HI
} ApsisStringDspVoiceId;

typedef struct {
    uint32_t freq_millihz[APSIS_STRING_DSP_VOICES];
    uint16_t gain_q15[APSIS_STRING_DSP_VOICES];
    uint8_t enabled[APSIS_STRING_DSP_VOICES];
} ApsisStringDspTargets;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t target_phase_inc;
    int32_t gain_q15;
    int32_t target_gain_q15;
    int32_t lowpass_q15;
    uint16_t pitch_alpha_q15;
    uint16_t lowpass_alpha_q15;
    uint8_t pitch_initialized;
} ApsisStringDspVoice;

typedef struct {
    ApsisStringDspVoice voice[APSIS_STRING_DSP_VOICES];
    uint8_t transport_running;
} ApsisStringDsp;

/* Source-derived acoustic targets. M4a intentionally emits only Foundation
 * plus Study-98 Classic sustain. Dual sustain remains a documented follow-up
 * and therefore has no invented BODY/WIRE replacement in this slice. */
void apsis_string_dsp_targets(const ApsisState *state,
                              ApsisStringPitchMemory *pitch_memory,
                              uint8_t transport_running,
                              ApsisStringDspTargets *out);

void apsis_string_dsp_reset(ApsisStringDsp *dsp);
void apsis_string_dsp_update(ApsisStringDsp *dsp,
                             const ApsisState *state,
                             ApsisStringPitchMemory *pitch_memory,
                             uint8_t transport_running);

/* Adds dry String to an interleaved stereo Q15 buffer. */
void apsis_string_dsp_render(ApsisStringDsp *dsp, int32_t *out, uint32_t frames);

#endif
