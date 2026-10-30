#ifndef APSIS_PERSISTENT_AUDIO_H
#define APSIS_PERSISTENT_AUDIO_H

#include <stdint.h>
#include "apsis_string_dsp.h"
#include "apsis_drift_dsp.h"

/* Provisional until M5a runtime stress measures the real structural-audio
 * pending high-water. Do not treat this as a frozen product capacity yet. */
#define APSIS_PERSISTENT_AUDIO_QUEUE_CAP 8u

typedef struct {
    ApsisStringDspTargets string_targets;
    ApsisDriftDspTargets drift_targets;
    uint16_t string_glide_q15;
    uint8_t string_surface_mode;
    uint8_t transport_running;
} ApsisPersistentAudioTargets;

typedef struct {
    uint64_t sample;
    uint32_t serial;
    ApsisPersistentAudioTargets targets;
} ApsisPersistentAudioEvent;

typedef struct {
    ApsisPersistentAudioEvent event[APSIS_PERSISTENT_AUDIO_QUEUE_CAP];
    uint32_t next_serial;
    uint8_t count;
    uint8_t high_water;
    uint8_t overflowed;
} ApsisPersistentAudioQueue;

/* Capture only the persistent acoustic result of the current shared state.
 * This deliberately does not copy ApsisState. */
void apsis_persistent_audio_capture(const ApsisState *state,
                                    ApsisStringPitchMemory *pitch_memory,
                                    uint8_t transport_running,
                                    ApsisPersistentAudioTargets *out);

/* Apply one precomputed snapshot to the existing M4 String/Drift DSP state.
 * No oscillator/filter identity is redesigned here. */
void apsis_persistent_audio_apply(ApsisStringDsp *string_dsp,
                                  ApsisDriftDsp *drift_dsp,
                                  const ApsisPersistentAudioTargets *targets);

void apsis_persistent_audio_queue_reset(ApsisPersistentAudioQueue *queue);
uint8_t apsis_persistent_audio_queue_push(ApsisPersistentAudioQueue *queue,
                                          uint64_t sample,
                                          const ApsisPersistentAudioTargets *targets);
const ApsisPersistentAudioEvent *apsis_persistent_audio_queue_peek(
                                          const ApsisPersistentAudioQueue *queue);
uint8_t apsis_persistent_audio_queue_pop(ApsisPersistentAudioQueue *queue,
                                         ApsisPersistentAudioEvent *out);

/* Return the next automation boundary in [block_start, block_end). Events that
 * are already late clamp to block_start; an event exactly at block_end belongs
 * to the next callback, matching M3c's event.sample < end convention. */
uint8_t apsis_persistent_audio_next_boundary(const ApsisPersistentAudioQueue *queue,
                                             uint64_t block_start,
                                             uint64_t block_end,
                                             uint64_t *sample_out);

/* Apply every event whose target is <= sample in stable sample/serial order. */
uint8_t apsis_persistent_audio_apply_through(ApsisPersistentAudioQueue *queue,
                                             uint64_t sample,
                                             ApsisStringDsp *string_dsp,
                                             ApsisDriftDsp *drift_dsp);

/* Render persistent String + Drift across one callback, splitting only at
 * queued structural-audio targets. The event sample itself belongs to the new
 * target: frames before the boundary use the old target, then all events at
 * that sample are applied before rendering the frame at the boundary. */
void apsis_persistent_audio_render_block(ApsisPersistentAudioQueue *queue,
                                         ApsisStringDsp *string_dsp,
                                         ApsisDriftDsp *drift_dsp,
                                         int32_t *out,
                                         uint32_t frames,
                                         uint64_t block_start);

#endif
