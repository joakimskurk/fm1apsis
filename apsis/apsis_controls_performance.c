#include "apsis_controls_performance.h"

void apsis_controls_performance_reset(ApsisControlsPerformanceMailbox *mailbox)
{
    if (!mailbox) return;
    mailbox->string_request = 0u;
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
