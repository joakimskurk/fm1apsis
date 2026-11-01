#ifndef APSIS_CONTROLS_DRIFT_H
#define APSIS_CONTROLS_DRIFT_H

#include <stdint.h>
#include "apsis_controls.h"

#define APSIS_DRIFT_AMOUNT_STEP_Q15 APSIS_Q15_MILLI(10)
#define APSIS_DRIFT_DEPTH_MIN_Q15   APSIS_Q15_MILLI(150)
#define APSIS_DRIFT_PARAM_STEP_Q15  APSIS_Q15_MILLI(10)

typedef enum {
    APSIS_DRIFT_PARAM_NONE = 0,
    APSIS_DRIFT_PARAM_DEPTH,
    APSIS_DRIFT_PARAM_WIDTH,
    APSIS_DRIFT_PARAM_TUNING_STRENGTH
} ApsisDriftControlParam;

typedef struct {
    uint8_t param;
    int32_t value;
} ApsisDriftParamEdit;

/* Study 126 surface gestures are live: SELECT changes the physical Drift
 * separation/amount and the first three note keys select the three Tuning
 * stations in their source left-to-right order: PYTH, JUST, EQUAL. */
uint8_t apsis_controls_drift_apply_live(ApsisState *state,
                                        const ApsisFm1InputFrame *input);

/* Parameter-panel roles use applyParam() in the source and therefore emit
 * intents for P9H's existing MOTHER-aware structural apply bridge. */
uint8_t apsis_controls_drift_param_edits(const ApsisState *state,
                                         const ApsisFm1InputFrame *input,
                                         ApsisDriftParamEdit out[3]);

#endif
