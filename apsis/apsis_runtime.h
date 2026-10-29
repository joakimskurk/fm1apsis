#ifndef APSIS_RUNTIME_H
#define APSIS_RUNTIME_H

#include <stdint.h>
#include "apsis_transport.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_mother.h"
#include "apsis_bloom.h"
#include "apsis_late_binding.h"

#define APSIS_RUNTIME_EVENT_TYPES ((uint8_t)APSIS_EVENT_BLOOM_BIND + 1u)
#define APSIS_RUNTIME_LOOKAHEAD_MS 1200u
#define APSIS_RUNTIME_GRAVITY_COMMIT_LEAD_MS 18u
#define APSIS_RUNTIME_MOTHER_COMMIT_LEAD_MS 26u
#define APSIS_RUNTIME_CHAIN_APPLY_LEAD_MS 24u
#define APSIS_RUNTIME_SPIRAL_SLOTS 64u
#define APSIS_RUNTIME_BLOOM_SLOTS 64u

typedef struct {
    uint8_t active;
    uint8_t degree;
    uint8_t ring;
    uint32_t token;
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
    uint8_t active;
    uint8_t route_index;
    uint8_t trigger_pos;
    uint16_t energy_q15;
    uint32_t token;
    uint64_t target_sample;
} ApsisRuntimeSpiralSlot;

typedef struct {
    uint8_t valid;
    uint8_t skipped;
    uint8_t trigger_pos;
    uint16_t energy_q15;
    uint32_t serial;
    uint64_t bind_sample;
    uint64_t target_sample;
    ApsisSpiralLateBinding pitch;
} ApsisRuntimeSpiralBinding;

typedef struct {
    uint8_t valid;
    uint64_t sample;
    uint32_t serial;
    ApsisBloomHarmony harmony;
    ApsisBloomPlan plan;
} ApsisRuntimeBloomOccurrence;

typedef struct {
    uint8_t active;
    uint8_t voice_count;
    uint8_t next_voice;
    uint32_t token;
    uint16_t unfurl_ms;
    uint64_t occurrence_sample;
    uint64_t target_sample;
    ApsisBloomHarmony harmony;
    ApsisBloomPlan plan;
} ApsisRuntimeBloomSlot;

typedef struct {
    uint8_t valid;
    uint8_t skipped;
    uint32_t serial;
    uint64_t bind_sample;
    uint64_t target_sample;
    ApsisBloomLateBinding pitch;
} ApsisRuntimeBloomBinding;

/* M4b does not add another scheduler. This is only a passive copy of one
 * shared transport event when the M3c coordinator consumes it. The observation
 * lives outside ApsisRuntime so the frozen M3c runtime layout stays unchanged. */
typedef struct {
    uint8_t valid;
    ApsisTransportEvent event;
} ApsisRuntimeStringRhythmObservation;

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
    uint32_t random_draw_count;
    uint32_t random_trace_hash;
    uint32_t late_trace_hash;
    uint32_t event_count[APSIS_RUNTIME_EVENT_TYPES];
    uint64_t last_event_sample;

    ApsisRuntimeGravityPending gravity_pending;

    uint8_t spiral_attractor_pos;
    uint32_t spiral_step;
    uint32_t spiral_occurrence_count;
    uint32_t spiral_bind_count;
    uint32_t spiral_bind_skipped;
    uint16_t spiral_slot_high_water;
    ApsisRuntimeSpiralOccurrence last_spiral;
    ApsisRuntimeSpiralBinding last_spiral_bind;
    ApsisRuntimeSpiralSlot spiral_slots[APSIS_RUNTIME_SPIRAL_SLOTS];

    uint32_t bloom_occurrence_count;
    uint32_t bloom_bind_count;
    uint32_t bloom_bind_skipped;
    uint16_t bloom_slot_high_water;
    uint8_t late_slot_overflowed;
    uint8_t scheduler_starved;
    ApsisRuntimeBloomOccurrence last_bloom;
    ApsisRuntimeBloomBinding last_bloom_bind;
    ApsisRuntimeBloomSlot bloom_slots[APSIS_RUNTIME_BLOOM_SLOTS];

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
 * M3c additionally schedules Study 126's state-observation commit callback at
 * target-18 ms on the same shared event heap. */
uint8_t apsis_runtime_request_gravity(ApsisRuntime *runtime,
                                      ApsisState *state,
                                      ApsisMotherPending *mother_pending,
                                      uint8_t degree, uint8_t ring,
                                      uint64_t now_sample,
                                      uint32_t min_lead_samples);

void apsis_runtime_chain_start(ApsisRuntime *runtime, ApsisState *state,
                               ApsisChainSequence *sequence);
void apsis_runtime_chain_stop(ApsisRuntime *runtime, ApsisChainSequence *sequence);

/* Frozen M3c timeline advance. */
void apsis_runtime_process(ApsisRuntime *runtime,
                           ApsisState *state,
                           ApsisKeepBank *keep_bank,
                           ApsisChainSequence *chain_sequence,
                           ApsisMotherPending *mother_pending,
                           ApsisBloomVoicingState *bloom_voicing,
                           uint32_t frames,
                           ApsisRuntimeMotherApplyFn mother_apply,
                           void *mother_user);

/* M4b audio handoff wrapper. It runs the exact M3c process above, but first
 * snapshots an already-queued STRING_RHYTHM event due inside this block. If M3c
 * actually consumes it, observation receives that event. No event is inserted,
 * rescheduled or removed by this wrapper. With legal Clock bounds there can be
 * at most one String Rhythm target in a 256-frame callback. */
uint8_t apsis_runtime_process_observed(ApsisRuntime *runtime,
                                      ApsisState *state,
                                      ApsisKeepBank *keep_bank,
                                      ApsisChainSequence *chain_sequence,
                                      ApsisMotherPending *mother_pending,
                                      ApsisBloomVoicingState *bloom_voicing,
                                      uint32_t frames,
                                      ApsisRuntimeMotherApplyFn mother_apply,
                                      void *mother_user,
                                      ApsisRuntimeStringRhythmObservation *observation);

#endif
