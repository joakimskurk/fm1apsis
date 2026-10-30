#ifndef APSIS_SPIRAL_DSP_H
#define APSIS_SPIRAL_DSP_H

#include <stdint.h>
#include "apsis_spiral_audio_runtime.h"

#define APSIS_SPIRAL_DSP_RATE 44100u
#define APSIS_SPIRAL_DSP_OSC_MAX 3u
/* Provisional analysis ceiling. M4f freezes the final value only after the
 * required 180 BPM / 1/16 / 4x / HALO worst-case stress has measured it. */
#define APSIS_SPIRAL_DSP_LEG_CAP 64u

typedef enum {
    APSIS_SPIRAL_DSP_SINE = 0,
    APSIS_SPIRAL_DSP_TRIANGLE
} ApsisSpiralDspWaveform;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint16_t gain_q15;
    uint8_t waveform;
} ApsisSpiralDspOsc;

typedef struct {
    int32_t b0_q30;
    int32_t b1_q30;
    int32_t b2_q30;
    int32_t a1_q30;
    int32_t a2_q30;
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;
} ApsisSpiralDspBiquad;

typedef struct {
    uint8_t active;
    uint8_t topology_voice;
    uint8_t cutoff_voice;
    uint8_t shadow;
    uint8_t osc_count;
    uint32_t serial;
    uint32_t token;
    uint32_t start_delay_samples;
    uint32_t age_samples;
    uint32_t attack_samples;
    uint32_t tail_samples;
    uint32_t life_samples;
    uint32_t gain_q23;
    uint32_t decay_mul_q31;
    uint16_t peak_gain_q15;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint16_t cutoff_hz;
    uint16_t q_milli;
    ApsisSpiralDspOsc osc[APSIS_SPIRAL_DSP_OSC_MAX];
    ApsisSpiralDspBiquad filter;
} ApsisSpiralDspLeg;

typedef struct {
    ApsisSpiralDspLeg leg[APSIS_SPIRAL_DSP_LEG_CAP];
    uint32_t next_serial;
    uint32_t hit_trigger_count;
    uint32_t leg_trigger_count;
    uint32_t steal_count;
    uint32_t drop_count;
    uint16_t logical_high_water;
    uint16_t leg_high_water;
    uint16_t oscillator_high_water;
} ApsisSpiralDsp;

void apsis_spiral_dsp_reset(ApsisSpiralDsp *dsp);

/* Instantiate the already-frozen binding into the finite leg pool. block_start
 * is the callback's first sample, so the existing target sample remains exact
 * even though the M3c bind was consumed later in the same callback. */
uint8_t apsis_spiral_dsp_trigger_binding(ApsisSpiralDsp *dsp,
                                         const ApsisState *state,
                                         const ApsisSpiralAudioBinding *binding,
                                         uint64_t block_start);

void apsis_spiral_dsp_render(ApsisSpiralDsp *dsp, int32_t *out,
                             uint32_t frames);

uint16_t apsis_spiral_dsp_logical_active(const ApsisSpiralDsp *dsp);
uint16_t apsis_spiral_dsp_leg_active(const ApsisSpiralDsp *dsp);
uint16_t apsis_spiral_dsp_oscillator_active(const ApsisSpiralDsp *dsp);

#endif
