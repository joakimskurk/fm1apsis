#ifndef APSIS_CONTROLS_PERFORMANCE_H
#define APSIS_CONTROLS_PERFORMANCE_H

#include <stdint.h>
#include "apsis_state.h"

/* Study 126 direct String gesture debounce: 30 ms at the frozen 44.1-kHz APSIS
 * source/output rate. Timing is evaluated only at audio block boundaries so a
 * physical gesture can be late by at most one callback, never early. */
#define APSIS_CONTROLS_STRING_MANUAL_DEBOUNCE_SAMPLES 1323u

#define APSIS_CONTROLS_BLOOM_PETAL_MASK 0x001fu
#define APSIS_CONTROLS_BLOOM_GROW_SHIFT 5u
#define APSIS_CONTROLS_BLOOM_GROW_MASK 0x03e0u
#define APSIS_CONTROLS_BLOOM_CORE_BIT 0x0400u

typedef struct {
    uint8_t core;
    uint8_t petal_mask;
    uint8_t grow_mask;
} ApsisControlsBloomRequest;

typedef struct {
    volatile uint8_t string_request; /* 0 none, 1 left/BODY, 2 right/WIRE */
    volatile uint16_t bloom_request; /* packed petals/grow/core; target producer IRQ-guards RMW */
    uint8_t string_last_valid;
    uint64_t string_last_sample;
} ApsisControlsPerformanceMailbox;

void apsis_controls_performance_reset(ApsisControlsPerformanceMailbox *mailbox);
void apsis_controls_performance_queue_string(ApsisControlsPerformanceMailbox *mailbox,
                                             uint8_t side);
uint8_t apsis_controls_performance_take_string(ApsisControlsPerformanceMailbox *mailbox,
                                               uint64_t now_sample,
                                               uint8_t *side);

/* Multiple grow-detent frames may arrive before the next audio callback, so
 * Bloom requests accumulate by bit. The generated FM-1 target wraps producer
 * calls in fm1_irq_off()/fm1_irq_on(); the audio ISR is the sole consumer. */
void apsis_controls_performance_queue_bloom(ApsisControlsPerformanceMailbox *mailbox,
                                            uint8_t core,
                                            uint8_t petal_mask,
                                            uint8_t grow_mask);
uint8_t apsis_controls_performance_take_bloom(ApsisControlsPerformanceMailbox *mailbox,
                                              ApsisControlsBloomRequest *request);

#endif
