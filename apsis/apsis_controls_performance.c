#include "apsis_controls_performance.h"

void apsis_controls_performance_reset(ApsisControlsPerformanceMailbox *mailbox)
{
    if (!mailbox) return;
    mailbox->string_request = 0u;
    mailbox->bloom_request = 0u;
    mailbox->string_last_valid = 0u;
    mailbox->string_last_sample = 0u;
}

void apsis_controls_performance_queue_string(ApsisControlsPerformanceMailbox *mailbox,
                                             uint8_t side)
{
    if (!mailbox) return;
    /* Single-core main->audio handoff: payload and valid flag are one volatile
     * byte, so ISR consumption can never observe a half-written side value.
     * A newer pre-consumption gesture intentionally replaces an older one. */
    mailbox->string_request = side == APSIS_SIDE_LEFT ? 1u : 2u;
}

uint8_t apsis_controls_performance_take_string(ApsisControlsPerformanceMailbox *mailbox,
                                               uint64_t now_sample,
                                               uint8_t *side)
{
    uint8_t request;
    uint64_t elapsed;
    if (!mailbox || !side) return 0u;
    request = mailbox->string_request;
    if (!request) return 0u;
    mailbox->string_request = 0u;

    if (mailbox->string_last_valid) {
        /* A runtime reset/rebase is not a rapid retrigger; permit it instead of
         * underflowing the debounce interval. */
        if (now_sample >= mailbox->string_last_sample) {
            elapsed = now_sample - mailbox->string_last_sample;
            if (elapsed < APSIS_CONTROLS_STRING_MANUAL_DEBOUNCE_SAMPLES)
                return 0u;
        }
    }

    mailbox->string_last_valid = 1u;
    mailbox->string_last_sample = now_sample;
    *side = request == 1u ? APSIS_SIDE_LEFT : APSIS_SIDE_RIGHT;
    return 1u;
}

void apsis_controls_performance_queue_bloom(ApsisControlsPerformanceMailbox *mailbox,
                                            uint8_t core,
                                            uint8_t petal_mask,
                                            uint8_t grow_mask)
{
    uint16_t request;
    if (!mailbox) return;
    petal_mask &= (uint8_t)APSIS_CONTROLS_BLOOM_PETAL_MASK;
    grow_mask &= petal_mask;
    request = (uint16_t)petal_mask |
              (uint16_t)((uint16_t)grow_mask << APSIS_CONTROLS_BLOOM_GROW_SHIFT);
    if (core) request |= APSIS_CONTROLS_BLOOM_CORE_BIT;
    mailbox->bloom_request = (uint16_t)(mailbox->bloom_request | request);
}

uint8_t apsis_controls_performance_take_bloom(ApsisControlsPerformanceMailbox *mailbox,
                                              ApsisControlsBloomRequest *request)
{
    uint16_t packed;
    if (!mailbox || !request) return 0u;
    packed = mailbox->bloom_request;
    if (!packed) return 0u;
    mailbox->bloom_request = 0u;
    request->core = (packed & APSIS_CONTROLS_BLOOM_CORE_BIT) ? 1u : 0u;
    request->petal_mask = (uint8_t)(packed & APSIS_CONTROLS_BLOOM_PETAL_MASK);
    request->grow_mask = (uint8_t)((packed & APSIS_CONTROLS_BLOOM_GROW_MASK) >>
                                   APSIS_CONTROLS_BLOOM_GROW_SHIFT);
    request->grow_mask &= request->petal_mask;
    return 1u;
}
