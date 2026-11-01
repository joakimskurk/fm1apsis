#ifndef APSIS_CONTROLS_APPLY_H
#define APSIS_CONTROLS_APPLY_H

#include <stdint.h>
#include "apsis_controls.h"
#include "apsis_controls_drift.h"
#include "apsis_controls_bloom.h"
#include "apsis_controls_space.h"
#include "apsis_mother.h"

/* Stable native keys for the subset of P9 physical edits that Study 126 routes
 * through applyParam(). Pointer/grid performance edits intentionally do not
 * live here: CLOCK grid, String split/mode, Drift amount/Tuning stations,
 * Spiral route and Bloom triggers are source-live gestures. */
typedef enum {
    APSIS_P9_PARAM_STRING_RESONANCE = 0x9100,
    APSIS_P9_PARAM_STRING_BRIGHTNESS,
    APSIS_P9_PARAM_STRING_GLIDE,
    APSIS_P9_PARAM_STRING_SPAN,

    APSIS_P9_PARAM_DRIFT_DEPTH = 0x9110,
    APSIS_P9_PARAM_DRIFT_WIDTH,
    APSIS_P9_PARAM_TUNING_STRENGTH,

    APSIS_P9_PARAM_BLOOM_UNFURL = 0x9120,
    APSIS_P9_PARAM_BLOOM_SPREAD,
    APSIS_P9_PARAM_BLOOM_LENGTH,
    APSIS_P9_PARAM_BLOOM_BRIGHTNESS,

    APSIS_P9_PARAM_SPACE_RETURN = 0x9140,
    APSIS_P9_PARAM_SPACE_DECAY_MS,
    APSIS_P9_PARAM_SPACE_PREDELAY_MS,
    APSIS_P9_PARAM_SPACE_DAMPING_HZ,
    APSIS_P9_PARAM_SPACE_ECHO_TIME_MS,
    APSIS_P9_PARAM_SPACE_ECHO_FEEDBACK,

    APSIS_P9_PARAM_SEND_STRING = 0x9160,
    APSIS_P9_PARAM_SEND_DRIFT,
    APSIS_P9_PARAM_SEND_CLOCK,
    APSIS_P9_PARAM_SEND_SPIRAL,
    APSIS_P9_PARAM_SEND_BLOOM
} ApsisP9PendingParam;

/* Apply immediately while MOTHER ->1 is inactive, otherwise coalesce into the
 * existing native pending queue. Returns 1 only for a known/accepted key. */
uint8_t apsis_controls_apply_structural_param(ApsisState *state,
                                              ApsisMotherPending *pending,
                                              uint16_t key,
                                              int32_t value,
                                              int32_t last_abs_step);

/* Runtime callback for an already-frozen pending item. Compatible directly
 * with ApsisRuntimeMotherApplyFn; user is reserved for later target context. */
uint8_t apsis_controls_apply_pending_item(ApsisState *state,
                                          const ApsisMotherPendingItem *item,
                                          void *user);

uint8_t apsis_controls_apply_string_edit(ApsisState *state,
                                         ApsisMotherPending *pending,
                                         const ApsisControlParamEdit *edit,
                                         int32_t last_abs_step);
uint8_t apsis_controls_apply_drift_edit(ApsisState *state,
                                        ApsisMotherPending *pending,
                                        const ApsisDriftParamEdit *edit,
                                        int32_t last_abs_step);
uint8_t apsis_controls_apply_bloom_edit(ApsisState *state,
                                        ApsisMotherPending *pending,
                                        const ApsisBloomParamEdit *edit,
                                        int32_t last_abs_step);
uint8_t apsis_controls_apply_space_edit(ApsisState *state,
                                        ApsisMotherPending *pending,
                                        const ApsisSpaceControlEdit *edit,
                                        int32_t last_abs_step);

#endif
