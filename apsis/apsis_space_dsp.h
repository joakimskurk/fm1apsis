#ifndef APSIS_SPACE_DSP_H
#define APSIS_SPACE_DSP_H

#include <stdint.h>

#define APSIS_SPACE_DSP_RATE 44100u
#define APSIS_SPACE_FDN_LINES 8u
#define APSIS_SPACE_FDN_MOD_RESERVE 16u
#define APSIS_SPACE_FDN_STORAGE_SAMPLES 19170u
#define APSIS_SPACE_FDN_STORAGE_BYTES (APSIS_SPACE_FDN_STORAGE_SAMPLES * 2u)

/* Frozen selected Phase-6 topology. */
extern const uint16_t APSIS_SPACE_FDN_DELAY_SAMPLES[APSIS_SPACE_FDN_LINES];

typedef struct {
    uint32_t offset;
    uint16_t base_delay;
    uint16_t capacity;
    uint16_t write_pos;
    uint16_t fill_count;
    uint32_t lfo_phase;
    uint32_t lfo_phase_inc;
    int32_t damping_state_q15;
} ApsisSpaceFdnLine;

typedef struct {
    int16_t *storage;
    uint32_t storage_samples;
    ApsisSpaceFdnLine line[APSIS_SPACE_FDN_LINES];
    int32_t post_left_q15;
    int32_t post_right_q15;
    uint8_t storage_bound;
} ApsisSpaceFdn;

/* Bind caller-owned storage. The engine never allocates and never clears the
 * large delay memory. Reset only invalidates history through fill counters, so
 * arbitrary NOLOAD contents are never read before they have been overwritten. */
uint8_t apsis_space_fdn_bind(ApsisSpaceFdn *fdn,
                             int16_t *storage,
                             uint32_t storage_samples);
void apsis_space_fdn_reset(ApsisSpaceFdn *fdn);

/* Render one interleaved stereo block from a mono-Q15 room excitation buffer.
 * `out_stereo` is additive. Decay is clamped to 4..12 s and damping to the
 * Study-126 live tail range 2200..6800 Hz. */
void apsis_space_fdn_render(ApsisSpaceFdn *fdn,
                            const int32_t *input_mono_q15,
                            int32_t *out_stereo,
                            uint32_t frames,
                            uint16_t decay_ms,
                            uint16_t damping_hz);

#endif
