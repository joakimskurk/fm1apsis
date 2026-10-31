#ifndef APSIS_CONTROLS_PERFORMANCE_H
#define APSIS_CONTROLS_PERFORMANCE_H

#include <stdint.h>
#include "apsis_state.h"

/* Study 126 direct String gesture debounce: 30 ms at the frozen 44.1-kHz APSIS
 * source/output rate. Timing is evaluated only at audio block boundaries so a
 * physical gesture can be late by at most one callback, never early. */
#define APSIS_CONTROLS_STRING_MANUAL_DEBOUNCE_SAMPLES 1323u

typedef struct {
    volatile uint8_t string_request; /* 0 none, 1 left/BODY, 2 right/WIRE */
    uint8_t string_last_valid;
    uint64_t string_last_sample;
} ApsisControlsPerformanceMailbox;

void apsis_controls_performance_reset(ApsisControlsPerformanceMailbox *mailbox);
void apsis_controls_performance_queue_string(ApsisControlsPerformanceMailbox *mailbox,
                                             uint8_t side);
uint8_t apsis_controls_performance_take_string(ApsisControlsPerformanceMailbox *mailbox,
                                               uint64_t now_sample,
                                               uint8_t *side);

#endif
