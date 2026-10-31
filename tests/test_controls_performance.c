#include <assert.h>
#include <stdio.h>
#include "apsis_controls_performance.h"

int main(void)
{
    ApsisControlsPerformanceMailbox m;
    uint8_t side = 99u;

    apsis_controls_performance_reset(&m);
    assert(m.string_request == 0u);
    assert(apsis_controls_performance_take_string(&m, 0u, &side) == 0u);

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

    /* Latest main-loop gesture wins before the ISR consumes the byte mailbox. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_LEFT);
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_RIGHT);
    assert(apsis_controls_performance_take_string(&m, 4000u, &side) == 1u);
    assert(side == APSIS_SIDE_RIGHT);

    /* A rebased sample cursor is accepted rather than underflowing elapsed. */
    apsis_controls_performance_queue_string(&m, APSIS_SIDE_LEFT);
    assert(apsis_controls_performance_take_string(&m, 10u, &side) == 1u);
    assert(side == APSIS_SIDE_LEFT);

    puts("PASS: P9H4a String main-to-audio mailbox and 30 ms debounce");
    return 0;
}
