#ifndef APSIS_SPIRAL_AUDIO_RUNTIME_H
#define APSIS_SPIRAL_AUDIO_RUNTIME_H

#include <stdint.h>
#include "apsis_runtime.h"
#include "apsis_spiral_audio.h"

#define APSIS_SPIRAL_AUDIO_BINDINGS_MAX 8u

typedef struct {
    uint8_t active;
    uint32_t token;
    uint64_t target_sample;
    ApsisSpiralAcousticPlan acoustic;
} ApsisSpiralAudioSnapshot;

typedef struct {
    uint8_t valid;
    uint8_t cutoff_voice;
    uint32_t token;
    uint64_t bind_sample;
    uint64_t target_sample;
    ApsisSpiralAcousticPlan acoustic;
    ApsisSpiralLateBinding pitch;
} ApsisSpiralAudioBinding;

typedef struct {
    ApsisSpiralAudioSnapshot snapshot[APSIS_RUNTIME_SPIRAL_SLOTS];
    ApsisSpiralAudioBinding binding[APSIS_SPIRAL_AUDIO_BINDINGS_MAX];
    uint8_t binding_count;
    uint32_t capture_count;
    uint32_t bind_count;
    uint32_t capture_miss_count;
    uint32_t bind_miss_count;
    uint32_t binding_overflow_count;
} ApsisSpiralAudioRuntime;

void apsis_spiral_audio_runtime_reset(ApsisSpiralAudioRuntime *audio);
void apsis_spiral_audio_runtime_begin_process(ApsisSpiralAudioRuntime *audio);

/* Call immediately after the frozen M3c/M4c scheduler has planned one pulse.
 * Newly-active native Spiral slots are mirrored by token. The matching shared
 * SPIRAL_TICK supplies the exact swung interval/subdivision metadata needed to
 * freeze Study-126 acoustic duration without scheduling anything new. */
void apsis_spiral_audio_runtime_capture(ApsisSpiralAudioRuntime *audio,
                                        const ApsisRuntime *runtime,
                                        const ApsisState *state);

/* Call immediately after M3c consumes APSIS_EVENT_SPIRAL_BIND. The already
 * resolved M3c pitch is combined with the occurrence-time acoustic snapshot.
 * No pitch, random choice or target sample is recomputed here. */
void apsis_spiral_audio_runtime_bind(ApsisSpiralAudioRuntime *audio,
                                     const ApsisRuntime *runtime,
                                     const ApsisState *state,
                                     const ApsisTransportEvent *bind_event);

#endif
