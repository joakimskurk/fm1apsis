#ifndef APSIS_BLOOM_AUDIO_RUNTIME_H
#define APSIS_BLOOM_AUDIO_RUNTIME_H

#include <stdint.h>
#include "apsis_runtime.h"
#include "apsis_bloom_audio.h"

#define APSIS_BLOOM_AUDIO_BINDINGS_MAX 8u

typedef struct {
    uint8_t active;
    uint32_t token;
    uint64_t occurrence_sample;
    uint64_t target_sample;
    ApsisBloomAcousticPlan acoustic;
} ApsisBloomAudioSnapshot;

typedef struct {
    uint8_t valid;
    uint8_t petal_index;
    uint8_t voice;
    uint32_t token;
    uint64_t bind_sample;
    uint64_t target_sample;
    uint16_t attack_ms;
    uint16_t brightness_q15;
    ApsisBloomPetalAcoustic acoustic;
    ApsisBloomLateBinding pitch;
} ApsisBloomAudioBinding;

typedef struct {
    ApsisBloomAudioSnapshot snapshot[APSIS_RUNTIME_BLOOM_SLOTS];
    ApsisBloomAudioBinding binding[APSIS_BLOOM_AUDIO_BINDINGS_MAX];
    uint8_t binding_count;
    uint32_t capture_count;
    uint32_t bind_count;
    uint32_t capture_miss_count;
    uint32_t bind_miss_count;
    uint32_t binding_overflow_count;
} ApsisBloomAudioRuntime;

void apsis_bloom_audio_runtime_reset(ApsisBloomAudioRuntime *audio);
void apsis_bloom_audio_runtime_begin_process(ApsisBloomAudioRuntime *audio);

/* Call immediately after the frozen M3c BLOOM_OCCURRENCE handler. Newly-active
 * native Bloom slots are mirrored by slot+token and occurrence-time acoustic
 * geometry only. No harmony, random or target is recalculated here. */
void apsis_bloom_audio_runtime_capture(ApsisBloomAudioRuntime *audio,
                                       const ApsisRuntime *runtime,
                                       const ApsisState *state);

/* Call immediately after the frozen M3c BLOOM_BIND handler. The already-bound
 * native pitch is paired with the occurrence snapshot's matching petal and the
 * current Study-126 voice/attack/brightness. Snapshot lifetime continues until
 * the final frozen petal has bound. */
void apsis_bloom_audio_runtime_bind(ApsisBloomAudioRuntime *audio,
                                    const ApsisRuntime *runtime,
                                    const ApsisState *state,
                                    const ApsisTransportEvent *bind_event);

#endif
