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

/* Cheap target-side activation probe. It only asks whether an existing shared
 * structural observation event due before end_sample could produce a persistent
 * String/Drift target: Gravity always; Mother only with pending state; Chain
 * only while Chain is actually playing. No event is inserted or consumed. */
uint8_t apsis_runtime_structural_audio_due(const ApsisRuntime *runtime,
                                           const ApsisMotherPending *mother_pending,
                                           const ApsisChainSequence *chain_sequence,
                                           uint64_t end_sample);

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
