#ifndef APSIS_RUNTIME_M4F_H
#define APSIS_RUNTIME_M4F_H

#include "apsis_runtime.h"
#include "apsis_spiral_audio_runtime.h"

void apsis_runtime_start_spiral_audio(ApsisRuntime *runtime,
                                      ApsisState *state,
                                      ApsisStringAttractorRuntime *attractor,
                                      ApsisSpiralAudioRuntime *spiral_audio);

void apsis_runtime_stop_spiral_audio(ApsisRuntime *runtime,
                                     ApsisStringAttractorRuntime *attractor,
                                     ApsisSpiralAudioRuntime *spiral_audio);

/* M4f extension of the proven M4c coordinator. It preserves the same scheduler
 * and consume order, but snapshots newly-created M3c Spiral slots externally
 * and emits bound audio records after the existing SPIRAL_BIND handler. */
uint8_t apsis_runtime_process_spiral_audio(
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
                                      ApsisRuntimeStringRhythmObservation *rhythm_observation);

#endif
