#ifndef APSIS_STRING_MOTION_H
#define APSIS_STRING_MOTION_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_STRING_MOTION_POINTS_MAX 1200u
#define APSIS_STRING_MOTION_CAPTURE_HZ 32u
#define APSIS_STRING_MOTION_RATE 44100u

typedef enum {
    APSIS_STRING_MOTION_TARGET_LEFT = 0,
    APSIS_STRING_MOTION_TARGET_RIGHT,
    APSIS_STRING_MOTION_TARGET_BOTH
} ApsisStringMotionTarget;

/* Native bounded representation of Study-126 [phase, split, frozen pitch].
 * pitch_milli is absolute semitone position relative to C3=0, including the
 * recording-time tuning deviation. Linear interpolation in pitch_milli is the
 * log-pitch equivalent of the browser's geometric Hz interpolation. */
typedef struct {
    uint16_t phase_q16;
    uint16_t split_q15;
    int32_t pitch_milli;
} ApsisStringMotionPoint;

typedef struct {
    ApsisStringMotionPoint point[APSIS_STRING_MOTION_POINTS_MAX];
    uint16_t count;
} ApsisStringMotionLane;

typedef struct {
    ApsisStringMotionLane left;
    ApsisStringMotionLane right;
} ApsisStringMotionStore;

/* Minimum frozen musical context required to reproduce
 * Study-126 stringFrequencyForSplitInContext() after later Gravity/Tuning edits. */
typedef struct {
    uint16_t span_q15;
    uint16_t temper_strength_q15;
    uint8_t surface_mode;
    uint8_t left_anchor;
    uint8_t right_anchor;
    uint8_t left_follow_gravity;
    uint8_t right_follow_gravity;
    int8_t left_gravity_degree;
    int8_t right_gravity_degree;
    uint8_t quantize;
    uint8_t harmony;
    uint8_t tonic;
    uint8_t scale;
    uint8_t degree;
    uint8_t voicing;
    uint8_t voices;
    uint8_t scale_lock;
    uint8_t temper_mode;
} ApsisStringMotionPitchContext;

typedef struct {
    ApsisStringMotionPitchContext pitch;
    uint64_t record_pulse_sample;
    uint64_t play_pulse_sample;
    uint64_t last_capture_sample;
    uint32_t pulse_interval_samples;
    uint16_t record_pulse_index;
    uint16_t play_pulse_index;
    uint16_t owned_left_q15;
    uint16_t owned_right_q15;
    uint8_t armed;
    uint8_t recording;
    uint8_t record_target;
    uint8_t record_cycles;
    uint8_t play_cycle;
    uint8_t manual_override;
    uint8_t just_finished;
} ApsisStringMotionRuntime;

typedef struct {
    uint16_t left_split_q15;
    uint16_t right_split_q15;
    int32_t left_pitch_milli;
    int32_t right_pitch_milli;
    uint16_t phase_q16;
} ApsisStringMotionPlayback;

void apsis_string_motion_reset(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime);

uint8_t apsis_string_motion_length(uint8_t cycles);
uint8_t apsis_string_motion_resolve_target(const ApsisState *state,
                                           uint8_t requested_target);

/* Arm/cancel is immediate, but recording begins only from string_motion_pulse()
 * at Clock step 0 (source UI: ARM ... TO NEXT 1). */
uint8_t apsis_string_motion_arm(ApsisStringMotionRuntime *runtime,
                                ApsisState *state,
                                uint8_t requested_target);
void apsis_string_motion_clear(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime,
                               ApsisState *state);
uint8_t apsis_string_motion_toggle_play(const ApsisStringMotionStore *store,
                                        ApsisStringMotionRuntime *runtime,
                                        ApsisState *state);

void apsis_string_motion_freeze_pitch(const ApsisState *state,
                                      ApsisStringMotionPitchContext *out);
int32_t apsis_string_motion_pitch_for_split(const ApsisStringMotionPitchContext *context,
                                            uint8_t side,
                                            uint16_t split_q15);

/* Clock pulse authority. step is zero-based visible Clock position; recording
 * starts/ends only at step==0 and therefore remains whole-cycle quantized. */
void apsis_string_motion_pulse(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime,
                               ApsisState *state,
                               uint8_t step,
                               uint64_t when_sample,
                               uint32_t interval_samples);

/* Continuous capture may be called more often; non-forced calls are bounded to
 * 32 Hz like Study 126. Forced Clock captures bypass the throttle. */
uint8_t apsis_string_motion_capture(ApsisStringMotionStore *store,
                                    ApsisStringMotionRuntime *runtime,
                                    const ApsisState *state,
                                    uint64_t now_sample,
                                    uint8_t force);

/* During REC, Gravity/scene changes may change shared harmony but cannot move
 * the String positions owned by the recorder. */
void apsis_string_motion_restore_owned(const ApsisStringMotionRuntime *runtime,
                                       ApsisState *state);

/* Manual String grab temporarily suppresses playback. Release resumes from the
 * current Clock-derived phase; it never restarts the loop. */
void apsis_string_motion_manual_override(ApsisStringMotionRuntime *runtime,
                                         uint8_t active);

uint8_t apsis_string_motion_playback(const ApsisStringMotionStore *store,
                                     const ApsisStringMotionRuntime *runtime,
                                     const ApsisState *state,
                                     uint64_t now_sample,
                                     ApsisStringMotionPlayback *out);

#endif
