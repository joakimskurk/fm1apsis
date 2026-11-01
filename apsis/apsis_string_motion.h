#ifndef APSIS_STRING_MOTION_H
#define APSIS_STRING_MOTION_H

#include <stdint.h>
#include "apsis_state.h"

/* Study-126 allows 1800 raw capture points and simplifies the completed loop
 * to 1200. Native stores LEFT/RIGHT as independent tracks so a Dual+FREE
 * overdub can replace one side without cloning the untouched side. */
#define APSIS_STRING_MOTION_RAW_MAX 1800u
#define APSIS_STRING_MOTION_LOOP_MAX 1200u
#define APSIS_STRING_MOTION_CAPTURE_MIN_SAMPLES 1379u /* ceil(44100 / 32) */

typedef enum {
    APSIS_STRING_MOTION_LEFT = 0,
    APSIS_STRING_MOTION_RIGHT,
    APSIS_STRING_MOTION_BOTH
} ApsisStringMotionTarget;

typedef struct {
    uint16_t phase_q15[APSIS_STRING_MOTION_RAW_MAX];
    uint16_t split_q15[APSIS_STRING_MOTION_RAW_MAX];
    /* 24-bit little-endian centi-Hz: 0.01-Hz resolution, >167-kHz range.
     * Quantization is below Study-126's 0.08-Hz simplification threshold. */
    uint8_t frequency_centi_hz[APSIS_STRING_MOTION_RAW_MAX * 3u];
    uint16_t count;
} ApsisStringMotionTrack;

typedef struct {
    uint8_t tonic;
    uint8_t scale;
    uint8_t degree;
    uint8_t voicing;
    uint8_t voices;
    uint8_t inversion;
    uint8_t scale_lock;
    uint8_t surface_mode;
    uint8_t left_anchor;
    uint8_t right_anchor;
    uint8_t left_follow_gravity;
    uint8_t right_follow_gravity;
    int8_t left_gravity_degree;
    int8_t right_gravity_degree;
    uint8_t quantize;
    uint8_t harmony;
    uint16_t span_q15;
    uint8_t temper_mode;
    uint16_t temper_strength_q15;
} ApsisStringMotionPitchContext;

typedef struct {
    uint8_t valid;
    uint16_t split_q15;
    uint32_t pitch_a_millihz;
    uint32_t pitch_b_millihz;
    uint16_t pitch_fraction_q15;
} ApsisStringMotionSideSample;

typedef struct {
    ApsisStringMotionSideSample left;
    ApsisStringMotionSideSample right;
    uint8_t pitch_continuous;
} ApsisStringMotionPlayback;

typedef struct {
    ApsisStringMotionTrack left;
    ApsisStringMotionTrack right;
    ApsisStringMotionPitchContext pitch_context;
    uint8_t armed;
    uint8_t recording;
    uint8_t record_target;
    uint8_t preferred_target;
    uint8_t manual_override;
    uint8_t just_finished;
    uint8_t record_cycles;
    uint8_t play_cycle;
    uint16_t record_pulse_index;
    uint16_t play_pulse_index;
    uint64_t record_pulse_sample;
    uint64_t play_pulse_sample;
    uint32_t record_pulse_interval;
    uint32_t play_pulse_interval;
    uint64_t last_capture_sample;
    uint16_t owned_left_q15;
    uint16_t owned_right_q15;
} ApsisStringMotionRuntime;

void apsis_string_motion_reset(ApsisStringMotionRuntime *motion);
uint32_t apsis_string_motion_storage_bytes(void);

uint8_t apsis_string_motion_resolve_target(const ApsisState *state,
                                           uint8_t requested_target);
uint8_t apsis_string_motion_arm(ApsisStringMotionRuntime *motion,
                                ApsisState *state,
                                uint8_t requested_target);
void apsis_string_motion_clear(ApsisStringMotionRuntime *motion,
                               ApsisState *state);
uint8_t apsis_string_motion_toggle_play(ApsisStringMotionRuntime *motion,
                                        ApsisState *state,
                                        uint64_t sample_now);

/* Feed only the already-shared Clock pulse stream. step is zero-based. No
 * private timer/clock is owned by String Motion. */
void apsis_string_motion_pulse(ApsisStringMotionRuntime *motion,
                               ApsisState *state,
                               uint8_t step,
                               uint64_t pulse_sample,
                               uint32_t pulse_interval_samples);

/* Capture physical movement against the frozen record-start pitch context.
 * force is used by Clock pulse boundaries; ordinary gestures are throttled to
 * Study-126's 1/32-second capture cadence. */
uint8_t apsis_string_motion_capture(ApsisStringMotionRuntime *motion,
                                    const ApsisState *state,
                                    uint64_t sample_now,
                                    uint8_t force);

/* Physical String movement is always live. During recording it updates the
 * position ownership lock; during playback it temporarily overrides the loop
 * until manual_release(), then playback resumes at the current Clock phase. */
void apsis_string_motion_manual_split(ApsisStringMotionRuntime *motion,
                                      ApsisState *state,
                                      uint8_t side,
                                      uint16_t split_q15,
                                      uint8_t mirror,
                                      uint64_t sample_now);
void apsis_string_motion_manual_release(ApsisStringMotionRuntime *motion);
void apsis_string_motion_restore_owned(const ApsisStringMotionRuntime *motion,
                                       ApsisState *state);

/* Read-only playback sample. Frequency is returned as bracketing absolute
 * millihertz plus a Q15 interpolation fraction. For quantized recordings,
 * pitch_continuous=0 means hold pitch_a until the next recorded point; for
 * continuous recordings the audio adapter performs geometric interpolation. */
uint8_t apsis_string_motion_playback(const ApsisStringMotionRuntime *motion,
                                     const ApsisState *state,
                                     uint64_t sample_now,
                                     ApsisStringMotionPlayback *out);

#endif
