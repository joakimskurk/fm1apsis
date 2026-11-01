#ifndef APSIS_CONTROLS_DISPATCH_H
#define APSIS_CONTROLS_DISPATCH_H

#include <stdint.h>
#include "apsis_controls_apply.h"
#include "apsis_controls_keep_chain.h"
#include "apsis_controls_mother.h"
#include "apsis_controls_drift.h"

/* UI-only physical session. None of this is musical/persisted state. */
typedef struct {
    uint8_t clock_layer;
    uint8_t string_bank_base;
    uint8_t spiral_bank;
    uint8_t spiral_cursor;
    ApsisSpaceControlState space;
    ApsisBloomGrowGesture bloom_grow;
} ApsisControlsSession;

typedef struct {
    uint8_t changed;
    uint8_t suppress_navigation;
    uint8_t project_dirty;

    uint8_t gravity_request;
    uint8_t gravity_degree;
    uint8_t gravity_ring;

    uint8_t chain_transport;
    uint8_t chain_apply_current;

    uint8_t string_pluck;
    uint8_t string_side;
    uint16_t string_split_q15;

    uint8_t bloom_core;
    uint8_t bloom_petal_mask;
} ApsisControlsDispatchResult;

void apsis_controls_session_reset(ApsisControlsSession *session);

/* Dispatch one already-consumed FM-1 input frame on the current APSIS surface.
 * The caller owns global navigation and runtime/audio actions. last_abs_step is
 * only used to arm native MOTHER pending timing. */
void apsis_controls_dispatch(ApsisControlsSession *session,
                             ApsisState *state,
                             ApsisKeepBank *keep_bank,
                             ApsisChainSequence *chain,
                             ApsisMotherPending *mother_pending,
                             uint8_t surface,
                             const ApsisFm1InputFrame *input,
                             int32_t last_abs_step,
                             ApsisControlsDispatchResult *result);

#endif
