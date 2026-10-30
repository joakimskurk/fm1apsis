#ifndef APSIS_BLOOM_DSP_H
#define APSIS_BLOOM_DSP_H

#include <stdint.h>
#include "apsis_bloom_audio_runtime.h"

#define APSIS_BLOOM_DSP_RATE 44100u
#define APSIS_BLOOM_DSP_OSC_MAX 3u
/* Frozen from the legal 11-second worst-case probe: 125 simultaneous petals
 * / 375 oscillators with zero drops. 136 leaves 11 petal slots of margin,
 * enough for more than two complete five-petal occurrences beyond the measured
 * high-water, without carrying the provisional 160-slot RAM cost to target. */
#define APSIS_BLOOM_DSP_PETAL_CAP 136u

typedef enum {
    APSIS_BLOOM_DSP_SINE = 0,
    APSIS_BLOOM_DSP_TRIANGLE
} ApsisBloomDspWaveform;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint16_t gain_q15;
    uint8_t waveform;
} ApsisBloomDspOsc;

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
} ApsisBloomDspBiquad;

typedef struct {
    uint8_t active;
    uint8_t voice;
    uint8_t petal_index;
    uint8_t osc_count;
    uint32_t serial;
    uint32_t token;
    uint32_t start_delay_samples;
    uint32_t age_samples;
    uint32_t life_samples;
    uint32_t env_gain_q23;
    uint32_t env_event_sample[3];
    uint32_t env_event_gain_q23[3];
    uint8_t env_event_index;
    uint32_t env_stage_left;
    uint32_t env_stage_target_q23;
    uint32_t env_decay_mul_q31;
    uint32_t env_linear_step_q23;
    uint32_t env_linear_rem_q23;
    uint32_t env_linear_acc_q23;
    uint32_t env_linear_den;
    uint8_t env_stage_rising;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint16_t cutoff_hz;
    uint16_t q_milli;
    ApsisBloomDspOsc osc[APSIS_BLOOM_DSP_OSC_MAX];
    ApsisBloomDspBiquad filter;
} ApsisBloomDspPetal;

typedef struct {
    ApsisBloomDspPetal petal[APSIS_BLOOM_DSP_PETAL_CAP];
    uint32_t next_serial;
    uint32_t trigger_count;
    uint32_t drop_count;
    uint16_t petal_high_water;
    uint16_t oscillator_high_water;
} ApsisBloomDsp;

void apsis_bloom_dsp_reset(ApsisBloomDsp *dsp);

/* Instantiate one already-bound M3c/M4g petal. block_start is the callback's
 * first sample; any remaining delay keeps the existing target sample exact. */
uint8_t apsis_bloom_dsp_trigger_binding(ApsisBloomDsp *dsp,
                                        const ApsisState *state,
                                        const ApsisBloomAudioBinding *binding,
                                        uint64_t block_start);

void apsis_bloom_dsp_render(ApsisBloomDsp *dsp, int32_t *out,
                            uint32_t frames);

uint16_t apsis_bloom_dsp_active(const ApsisBloomDsp *dsp);
uint16_t apsis_bloom_dsp_oscillator_active(const ApsisBloomDsp *dsp);

#endif
