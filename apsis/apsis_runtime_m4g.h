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
