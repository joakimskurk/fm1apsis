#include <assert.h>
#include <stdio.h>
#include "apsis_controls_performance.h"

int main(void)
{
    ApsisControlsPerformanceMailbox m;
    ApsisControlsBloomRequest bloom;
    uint8_t side = 99u;

    apsis_controls_performance_reset(&m);
    assert(m.string_request == 0u);
    assert(m.bloom_request == 0u);
    assert(apsis_controls_performance_take_string(&m, 0u, &side) == 0u);
    assert(apsis_controls_performance_take_bloom(&m, &bloom) == 0u);

    apsis_controls_performance_queue_string(&m, APSIS_SIDE_LEFT);
    assert(apsis_controls_performance_take_string(&m, 1000u, &side) == 1u);
    assert(side == APSIS_SIDE_LEFT);
    assert(m.string_request == 0u);

    /* A rapid request is consumed but acoustically debounced. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_RIGHT);
    assert(apsis_controls_performance_take_string(&m, 2000u, &side) == 0u);
    assert(m.string_request == 0u);

    /* 30 ms exactly after the last accepted trigger is legal. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_RIGHT);
    assert(apsis_controls_performance_take_string(
        &m, 1000u + APSIS_CONTROLS_STRING_MANUAL_DEBOUNCE_SAMPLES, &side) == 1u);
    assert(side == APSIS_SIDE_RIGHT);

    /* Latest main-loop String gesture wins before the ISR consumes the byte. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_LEFT);
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_RIGHT);
    assert(apsis_controls_performance_take_string(&m, 4000u, &side) == 1u);
    assert(side == APSIS_SIDE_RIGHT);

    /* A rebased sample cursor is accepted rather than underflowing elapsed. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_LEFT);
    assert(apsis_controls_performance_take_string(&m, 10u, &side) == 1u);
    assert(side == APSIS_SIDE_LEFT);

    /* Bloom accumulates independently until the audio callback consumes it.
     * Grow bits are always a subset of the petal mask; CORE is orthogonal. */
    apsis_controls_performance_queue_bloom(&m, 0u, 0x01u, 0u);
    apsis_controls_performance_queue_bloom(&m, 0u, 0x06u, 0x06u);
    apsis_controls_performance_queue_bloom(&m, 1u, 0u, 0u);
    assert(apsis_controls_performance_take_bloom(&m, &bloom) == 1u);
    assert(bloom.core == 1u);
    assert(bloom.petal_mask == 0x07u);
    assert(bloom.grow_mask == 0x06u);
    assert(m.bloom_request == 0u);
    assert(apsis_controls_performance_take_bloom(&m, &bloom) == 0u);

    /* A stray grow bit cannot escape without its corresponding petal. */
    apsis_controls_performance_queue_bloom(&m, 0u, 0x08u, 0x18u);
    assert(apsis_controls_performance_take_bloom(&m, &bloom) == 1u);
    assert(bloom.petal_mask == 0x08u);
    assert(bloom.grow_mask == 0x08u);

    puts("PASS: P9H4 performance mailbox String debounce + Bloom accumulation");
    return 0;
}
