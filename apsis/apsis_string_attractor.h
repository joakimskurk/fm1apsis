#ifndef APSIS_STRING_ATTRACTOR_H
#define APSIS_STRING_ATTRACTOR_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_STRING_ATTRACTOR_CELLS_MAX 16u
#define APSIS_STRING_ATTRACTOR_DECISIONS_MAX 2u
/* Provisional until the M4c1 shared-horizon stress gate measures the real
 * simultaneous planned-but-not-bound high-water. The final contract must lock
 * a measured value plus explicit safety margin, not this initial ceiling. */
#define APSIS_STRING_ATTRACTOR_PENDING_CAP 80u
#define APSIS_STRING_ATTRACTOR_BINDS_PER_BLOCK_MAX 2u

#define APSIS_STRING_ATTRACTOR_ROUTE_LEFT    (1u << 0)
#define APSIS_STRING_ATTRACTOR_ROUTE_RIGHT   (1u << 1)
#define APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC (1u << 2)

typedef uint32_t (*ApsisStringAttractorRandomFn)(void *user, uint32_t tag);

typedef struct {
    uint8_t active;
    uint8_t route;
    uint8_t pos;
    uint8_t both_scale;
    uint16_t value_q15;
    uint16_t base_level_q15;
    uint32_t token;
    uint64_t bind_sample;
    uint64_t target_sample;
} ApsisStringAttractorPending;

typedef struct {
    uint8_t valid;
    uint8_t route;
    uint8_t pos;
    uint8_t both_scale;
    uint16_t value_q15;
    uint16_t base_level_q15;
    uint32_t token;
    uint64_t bind_sample;
    uint64_t target_sample;
} ApsisStringAttractorBinding;

typedef struct {
    uint16_t cell_q15[APSIS_STRING_ATTRACTOR_CELLS_MAX];
    uint16_t seed;
    uint8_t length;
    uint8_t pos;
    uint32_t advance_count;
    uint32_t mutation_count;
    uint32_t excitation_count;

    ApsisStringAttractorPending pending[APSIS_STRING_ATTRACTOR_PENDING_CAP];
    uint32_t next_token;
    uint32_t bind_count;
    uint32_t bind_stale_count;
    uint16_t pending_high_water;
    uint8_t pending_overflowed;

    /* Per-process output. At legal Clock bounds at most one S&H target can
     * reach bind time in a 256-frame block, but BOTH can yield two side binds. */
    uint8_t binding_count;
    ApsisStringAttractorBinding binding[APSIS_STRING_ATTRACTOR_BINDS_PER_BLOCK_MAX];
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

/* Full transport-start reset: Study 126 calls resetStringAttractor() before
 * startScheduler(), so each new run deterministically regenerates cells from
 * the current seed and starts with no pending bind timers. */
void apsis_string_attractor_reset(ApsisStringAttractorRuntime *runtime,
                                  const ApsisState *state);

/* Seed/length changes regenerate only the S&H cells/cursor. Study 126 does not
 * cancel already-scheduled attractor voice timers in resetStringAttractor(). */
void apsis_string_attractor_sync(ApsisStringAttractorRuntime *runtime,
                                 const ApsisState *state);

/* Transport stop cancels the outstanding Attractor voice timers. Cell values
 * may remain in memory; the next transport start regenerates them from seed. */
void apsis_string_attractor_stop(ApsisStringAttractorRuntime *runtime);

/* Clear only the per-callback binding output before one runtime process call. */
void apsis_string_attractor_begin_process(ApsisStringAttractorRuntime *runtime);

/* Fixed pending slot helpers used by the shared M3c heap integration. */
uint8_t apsis_string_attractor_pending_alloc(ApsisStringAttractorRuntime *runtime,
                                             uint8_t route,
                                             uint8_t pos,
                                             uint8_t both_scale,
                                             uint16_t value_q15,
                                             uint16_t base_level_q15,
                                             uint64_t bind_sample,
                                             uint64_t target_sample,
                                             uint32_t *token_out);
uint8_t apsis_string_attractor_pending_bind(ApsisStringAttractorRuntime *runtime,
                                            uint8_t slot_index,
                                            uint32_t token,
                                            uint64_t event_sample);

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
