#include "apsis_runtime_m5a.h"

uint8_t apsis_runtime_structural_audio_due(const ApsisRuntime *runtime,
                                           const ApsisMotherPending *mother_pending,
                                           const ApsisChainSequence *chain_sequence,
                                           uint64_t end_sample)
{
    uint16_t i;
    uint8_t mother_active, chain_active;
    if (!runtime) return 0u;
    mother_active = (uint8_t)(mother_pending &&
        apsis_mother_pending_count(mother_pending) > 0u);
    chain_active = (uint8_t)(chain_sequence && chain_sequence->playing);

    for (i = 0u; i < runtime->queue.count; ++i) {
        const ApsisTransportEvent *e = &runtime->queue.events[i];
        if (e->sample >= end_sample) continue;
        if (e->type == APSIS_EVENT_GRAVITY_COMMIT) return 1u;
        if (mother_active && e->type == APSIS_EVENT_MOTHER_COMMIT_OBSERVE) return 1u;
        if (chain_active && e->type == APSIS_EVENT_CHAIN_APPLY_OBSERVE) return 1u;
    }
    return 0u;
}
