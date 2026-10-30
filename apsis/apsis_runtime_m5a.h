#ifndef APSIS_RUNTIME_M5A_H
#define APSIS_RUNTIME_M5A_H

#include "apsis_runtime_m4g.h"
#include "apsis_persistent_audio.h"

typedef struct {
    uint32_t gravity_captures;
    uint32_t mother_captures;
    uint32_t chain_captures;
    uint32_t capture_drops;
} ApsisStructuralAudioStats;

void apsis_structural_audio_stats_reset(ApsisStructuralAudioStats *stats);

/* Phase-5 extension of the exact M4g coordinator. Shared state still mutates
 * at the existing M3c observation callbacks. After a structural change has
 * actually committed, this wrapper snapshots the resulting persistent
 * String/Drift DSP targets for the event's original musical target sample.
 *
 * A NULL persistent_queue delegates directly to frozen M4g behaviour. */
uint8_t apsis_runtime_process_structural_audio(
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
                                     ApsisRuntimeStringRhythmObservation *rhythm_observation,
                                     ApsisPersistentAudioQueue *persistent_queue,
                                     ApsisStringPitchMemory *pitch_memory,
                                     ApsisStructuralAudioStats *stats);

#endif
