#ifndef APSIS_KEEP_H
#define APSIS_KEEP_H

#include <stdint.h>
#include "apsis_state.h"

#define APSIS_KEEP_LANE_KEY_SCALE (1u << 0)
#define APSIS_KEEP_LANE_CHORD     (1u << 1)
#define APSIS_KEEP_LANE_TENSION   (1u << 2)
#define APSIS_KEEP_LANE_DRIFT     (1u << 3)
#define APSIS_KEEP_LANE_CLOCK     (1u << 4)
#define APSIS_KEEP_LANE_TRACE     (1u << 5)
#define APSIS_KEEP_LANE_TUNING    (1u << 6)
#define APSIS_KEEP_LANE_ALL       ((1u << 7) - 1u)

typedef struct {
    uint8_t valid;

    /* tension / String musical memory only */
    uint16_t split_q15;
    uint16_t left_split_q15;
    uint16_t right_split_q15;
    uint8_t surface_mode;
    uint8_t linked;
    uint8_t left_anchor;
    uint8_t right_anchor;
    uint8_t left_follow_gravity;
    uint8_t right_follow_gravity;
    uint8_t left_gravity_degree;
    uint8_t right_gravity_degree;

    /* drift */
    uint16_t drift_amount_q15;

    /* clock geometry, not tempo/timbre */
    uint8_t clock_n;
    uint8_t clock_bounds[3];
    uint32_t clock_main_mask;
    uint32_t clock_inner_mask;
    uint32_t clock_snap_mask;

    /* trace */
    uint8_t spiral_route[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t spiral_route_count;

    /* tuning */
    uint8_t tuning_mode;

    /* Gravity harmonic memory */
    uint8_t gravity_tonic;
    uint8_t gravity_scale;
    uint8_t gravity_degree;
    uint8_t gravity_voicing;
    uint8_t gravity_ring;
    uint8_t gravity_voices;
    uint8_t gravity_inversion;
} ApsisKeepSnapshot;

typedef struct {
    ApsisKeepSnapshot slots[APSIS_KEEP_SLOTS];
    int8_t selected_slot;
} ApsisKeepBank;

typedef struct {
    uint8_t from_chain;
    uint8_t skip_continuous;
    int8_t transpose;
    uint8_t lane_mask;
} ApsisKeepApplyOptions;

void apsis_keep_bank_reset(ApsisKeepBank *bank);
ApsisKeepSnapshot apsis_keep_capture(const ApsisState *state);
int8_t apsis_keep_capture_to_bank(ApsisKeepBank *bank, const ApsisState *state);
uint8_t apsis_keep_apply(ApsisState *state, const ApsisKeepSnapshot *snap,
                         const ApsisKeepApplyOptions *options);
uint8_t apsis_keep_lane_mask_from_chain(const ApsisChainState *chain);

#endif
