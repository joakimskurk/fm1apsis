#ifndef APSIS_STRING_ATTRACTOR_H
#define APSIS_STRING_ATTRACTOR_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_STRING_ATTRACTOR_CELLS_MAX 16u
#define APSIS_STRING_ATTRACTOR_DECISIONS_MAX 2u

#define APSIS_STRING_ATTRACTOR_ROUTE_LEFT    (1u << 0)
#define APSIS_STRING_ATTRACTOR_ROUTE_RIGHT   (1u << 1)
#define APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC (1u << 2)

typedef uint32_t (*ApsisStringAttractorRandomFn)(void *user, uint32_t tag);

typedef struct {
    uint16_t cell_q15[APSIS_STRING_ATTRACTOR_CELLS_MAX];
    uint16_t seed;
    uint8_t length;
    uint8_t pos;
    uint32_t advance_count;
    uint32_t mutation_count;
    uint32_t excitation_count;
} ApsisStringAttractorRuntime;

typedef struct {
    uint64_t target_sample;
    uint16_t value_q15;
    uint16_t base_level_q15;
    uint8_t pos;
    uint8_t route_mask;
    uint8_t gate;
    uint8_t excite;
} ApsisStringAttractorDecision;

/* Rebuild the local S&H cells from Study-126's seeded LCG and reset cursor.
 * This reset generator is local and must not consume the shared M3c RNG. */
void apsis_string_attractor_reset(ApsisStringAttractorRuntime *runtime,
                                  const ApsisState *state);

/* Ensure seed/length changes reproduce resetStringAttractor(). */
void apsis_string_attractor_sync(ApsisStringAttractorRuntime *runtime,
                                 const ApsisState *state);

/* Source rate mapping. Returns number of S&H advances for this Clock pulse:
 * 0, 1, or 2 (the latter is pulse + half-pulse for rate 0.5). */
uint8_t apsis_string_attractor_advances_for_pulse(const ApsisState *state,
                                                  uint8_t clock_step);

/* Pure planning-time source semantics. target_sample[0] is the pulse target;
 * target_sample[1] is the half-pulse target when the selected rate is 0.5.
 * The random callback is called once for lock and only conditionally a second
 * time for mutation, preserving Study-126 draw order. Returns advances made. */
uint8_t apsis_string_attractor_plan_pulse(ApsisStringAttractorRuntime *runtime,
                                          const ApsisState *state,
                                          uint8_t clock_step,
                                          const uint64_t target_sample[2],
                                          ApsisStringAttractorRandomFn random_fn,
                                          void *random_user,
                                          ApsisStringAttractorDecision out[2]);

#endif
