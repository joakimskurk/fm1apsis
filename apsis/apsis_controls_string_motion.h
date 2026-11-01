#ifndef APSIS_CONTROLS_STRING_MOTION_H
#define APSIS_CONTROLS_STRING_MOTION_H

#include "apsis_controls.h"
#include "apsis_string_motion.h"

typedef enum {
    APSIS_STRING_MOTION_ACTION_NONE = 0,
    APSIS_STRING_MOTION_ACTION_ARM_TOGGLE,
    APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE,
    APSIS_STRING_MOTION_ACTION_CLEAR
} ApsisStringMotionAction;

typedef struct {
    uint8_t action;
    uint8_t record_target;
    uint8_t target_changed;
    uint8_t suppress_string_bank;
} ApsisStringMotionControlResult;

/* Phase-10 contextual String Motion layer. This function is pure: it reads the
 * existing String mode plus one FM-1 frame and returns intents only.
 *
 * STRING gestures:
 * - REC edge: arm/cancel next-step-1 capture
 * - SEL+REC edge: play/stop existing Motion loop
 * - REC+OCT- chord: clear Motion (wins over ordinary arm/bank behavior)
 * - PRESETS detents: BODY/WIRE/BOTH target in Dual+FREE only
 *
 * Classic always resolves to RIGHT; Dual+LINK always BOTH. */
uint8_t apsis_controls_string_motion(const ApsisState *state,
                                     uint8_t current_target,
                                     const ApsisFm1InputFrame *input,
                                     ApsisStringMotionControlResult *out);

#endif
