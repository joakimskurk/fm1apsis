#ifndef APSIS_CLOCK_DSP_H
#define APSIS_CLOCK_DSP_H

#include <stdint.h>
#include "apsis_runtime.h"

#define APSIS_CLOCK_DSP_RATE 44100u
#define APSIS_CLOCK_DSP_VOICES 48u
#define APSIS_CLOCK_DSP_BATCH_CAP 8u

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
} ApsisClockBiquad;

typedef enum {
    APSIS_CLOCK_DSP_SINE = 0,
    APSIS_CLOCK_DSP_TRIANGLE,
    APSIS_CLOCK_DSP_NOISE,
    APSIS_CLOCK_DSP_SQUARE
} ApsisClockDspWaveform;

typedef struct {
    uint8_t active;
    uint8_t waveform;
    uint8_t filter_count;
    uint32_t delay_samples;
    uint32_t age_samples;
    uint32_t life_samples;
    uint32_t attack_samples;
    uint32_t decay_end_samples;
    uint32_t pitch_ramp_samples;
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t end_phase_inc;
    uint32_t pitch_mul_q31;
    uint32_t peak_gain_q23;
    uint32_t gain_q23;
    uint32_t decay_mul_q31;
    uint16_t mix_left_q15;
    uint16_t mix_right_q15;
    uint32_t noise_state;
    ApsisClockBiquad filter[2];
} ApsisClockDspVoice;

typedef struct {
    ApsisClockDspVoice voice[APSIS_CLOCK_DSP_VOICES];
    uint16_t high_water;
    uint32_t trigger_count;
    uint32_t drop_count;
} ApsisClockDsp;

typedef struct {
    uint8_t count;
    uint8_t overflowed;
    ApsisTransportEvent event[APSIS_CLOCK_DSP_BATCH_CAP];
} ApsisClockEventBatch;

void apsis_clock_dsp_reset(ApsisClockDsp *dsp);
void apsis_clock_event_batch_reset(ApsisClockEventBatch *batch);

/* Passive observation of already-planned Clock events due in this audio block.
 * On the first block after transport start/restart the shared queue has not yet
 * received pulse 0; bootstrap adds only deterministic pulse-0 Clock events and
 * does not mutate runtime or draw random numbers. */
void apsis_clock_collect_block(const ApsisRuntime *runtime,
                               const ApsisState *state,
                               uint32_t frames,
                               ApsisClockEventBatch *batch);

/* Instantiate source-derived finite voices with sample delay relative to the
 * supplied block start. Returns number of physical voices allocated. */
uint8_t apsis_clock_dsp_trigger_event(ApsisClockDsp *dsp,
                                      const ApsisState *state,
                                      const ApsisTransportEvent *event,
                                      uint64_t block_start_sample);

void apsis_clock_dsp_render(ApsisClockDsp *dsp, int32_t *out, uint32_t frames);

#endif
