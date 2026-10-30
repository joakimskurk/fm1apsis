#ifndef APSIS_RUNTIME_M4G_H
#define APSIS_RUNTIME_M4G_H

#include "apsis_runtime_m4f.h"
#include "apsis_bloom_audio_runtime.h"

void apsis_runtime_start_bloom_audio(ApsisRuntime *runtime,
                                     ApsisState *state,
                                     ApsisStringAttractorRuntime *attractor,
                                     ApsisSpiralAudioRuntime *spiral_audio,
                                     ApsisBloomAudioRuntime *bloom_audio);

void apsis_runtime_stop_bloom_audio(ApsisRuntime *runtime,
                                    ApsisStringAttractorRuntime *attractor,
                                    ApsisSpiralAudioRuntime *spiral_audio,
                                    ApsisBloomAudioRuntime *bloom_audio);

/* Read-only target activation probe. It never mutates the shared heap and only
 * reports an already-planned M3c BLOOM_OCCURRENCE whose callback belongs to the
 * current audio block. This lets the FM-1 target keep the exact frozen M4f path
 * until Bloom genuinely needs the M4g consumer. */
uint8_t apsis_runtime_bloom_occurrence_due(const ApsisRuntime *runtime,
                                           uint64_t end_sample);

/* M4g extends the proven M4f coordinator only with passive Bloom acoustic
 * capture/bind hooks around the existing M3c occurrence and petal-bind handlers.
 * NULL bloom_audio is exactly the M4f path. */
uint8_t apsis_runtime_process_bloom_audio(
                                     ApsisRuntime *runtime,
                                     ApsisState *state,
                                     ApsisKeepBank *keep_bank,
                                     ApsisChainSequence *chain_sequence,
                                     ApsisMotherPending *mother_pending,
                                     ApsisBloomVoicingState *bloom_voicing,
                                     uint32_t frames,
                                     ApsisRuntimeMotherApplyFn mother_apply,
                                     void *mother_user,
                                     ApsisStringAttractorRuntime *attractor,
                                     ApsisSpiralAudioRuntime *spiral_audio,
                                     ApsisBloomAudioRuntime *bloom_audio,
                                     ApsisRuntimeStringRhythmObservation *rhythm_observation);

#endif
