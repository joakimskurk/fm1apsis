#ifndef APSIS_RUNTIME_H
#define APSIS_RUNTIME_H

#include <stdint.h>
#include "apsis_transport.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_mother.h"
#include "apsis_bloom.h"

#define APSIS_RUNTIME_EVENT_TYPES ((uint8_t)APSIS_EVENT_CHAIN_PULSE + 1u)

typedef struct {
    uint8_t active;
    uint8_t degree;
    uint8_t ring;
    uint64_t target_sample;
} ApsisRuntimeGravityPending;

typedef struct {
    uint8_t valid;
    uint64_t sample;
    uint32_t serial;
    uint8_t route_index;
    uint8_t route_point;
    uint8_t trigger_pos;
    uint16_t energy_q15;
} ApsisRuntimeSpiralOccurrence;

typedef struct {
    uint8_t valid;
    uint64_t sample;
    uint32_t serial;
    ApsisBloomHarmony harmony;
    ApsisBloomPlan plan;
} ApsisRuntimeBloomOccurrence;

typedef uint8_t (*ApsisRuntimeMotherApplyFn)(ApsisState *state,
                                              const ApsisMotherPendingItem *item,
                                              void *user);

typedef struct {
    ApsisTransport transport;
    ApsisEventQueue queue;
    uint64_t cursor_sample;
    uint16_t queue_high_water;
    uint8_t queue_overflowed;
    uint8_t mother_apply_blocked;

    uint32_t random_state;
    uint32_t event_count[APSIS_RUNTIME_EVENT_TYPES];
    uint64_t last_event_sample;

    ApsisRuntimeGravityPending gravity_pending;

    uint8_t spiral_attractor_pos;
    uint8_t spiral_step;
    uint32_t spiral_occurrence_count;
    ApsisRuntimeSpiralOccurrence last_spiral;

    uint32_t bloom_occurrence_count;
    ApsisRuntimeBloomOccurrence last_bloom;

    /* Browser startChain() snapshots meter.n into chainMeter. Cell lengths,
     * master phrase and morph-cycle length keep using that frozen n until Stop. */
    uint8_t chain_meter_n;
    ApsisChainMorphTargets chain_morph;
    uint8_t chain_morph_active;
    uint64_t chain_morph_start_sample;
    uint64_t chain_morph_end_sample;
    uint16_t chain_morph_from_left_q15;
    uint16_t chain_morph_from_right_q15;
    uint16_t chain_morph_from_drift_q15;
} ApsisRuntime;

void apsis_runtime_reset(ApsisRuntime *runtime, uint32_t sample_rate,
                         uint64_t first_sample, uint32_t random_seed);
void apsis_runtime_start(ApsisRuntime *runtime);
void apsis_runtime_stop(ApsisRuntime *runtime);

/* Request a Gravity candidate using current source semantics. If MOTHER ->1
 * owns structural edits, the candidate is queued there. Immediate mode applies
 * directly. Quantized mode targets the next source-derived Gravity boundary.
 * min_lead_samples implements the UI safety lead without browser timer jitter. */
uint8_t apsis_runtime_request_gravity(ApsisRuntime *runtime,
                                      ApsisState *state,
                                      ApsisMotherPending *mother_pending,
                                      uint8_t degree, uint8_t ring,
                                      uint64_t now_sample,
                                      uint32_t min_lead_samples);

void apsis_runtime_chain_start(ApsisRuntime *runtime, ApsisState *state,
                               ApsisChainSequence *sequence);
void apsis_runtime_chain_stop(ApsisRuntime *runtime, ApsisChainSequence *sequence);

/* Advance the silent APSIS timeline by sample frames. This schedules only as
 * far as the current block requires, consumes due structural/symbolic events,
 * and never writes audio. Generic MOTHER structural keys are applied through
 * mother_apply; native UI code can provide the path/key dispatcher later. */
void apsis_runtime_process(ApsisRuntime *runtime,
                           ApsisState *state,
                           ApsisKeepBank *keep_bank,
                           ApsisChainSequence *chain_sequence,
                           ApsisMotherPending *mother_pending,
                           ApsisBloomVoicingState *bloom_voicing,
                           uint32_t frames,
                           ApsisRuntimeMotherApplyFn mother_apply,
                           void *mother_user);

#endif
