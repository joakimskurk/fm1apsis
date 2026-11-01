#ifndef APSIS_STRING_MOTION_CLOCK_H
#define APSIS_STRING_MOTION_CLOCK_H

#include "apsis_runtime.h"

#define APSIS_STRING_MOTION_PULSE_BATCH_MAX 4u

typedef struct {
    uint64_t sample;
    uint32_t interval_samples;
    uint32_t absolute_pulse;
    uint8_t step;
} ApsisStringMotionPulse;

typedef struct {
    ApsisStringMotionPulse pulse[APSIS_STRING_MOTION_PULSE_BATCH_MAX];
    uint8_t count;
    uint8_t overflowed;
} ApsisStringMotionPulseBatch;

/* Passive observation of the shared M3c PLAYHEAD events due in this callback.
 * No event is inserted/removed and no transport field is advanced here. */
void apsis_string_motion_collect_pulses(const ApsisRuntime *runtime,
                                        const ApsisState *state,
                                        uint32_t frames,
                                        ApsisStringMotionPulseBatch *out);

#endif
