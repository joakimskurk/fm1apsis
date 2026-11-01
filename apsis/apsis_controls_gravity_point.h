#ifndef APSIS_CONTROLS_GRAVITY_POINT_H
#define APSIS_CONTROLS_GRAVITY_POINT_H

#include "apsis_controls.h"

typedef enum {
    APSIS_GRAVITY_PAGE_HARMONY = 0,
    APSIS_GRAVITY_PAGE_TIMING,
    APSIS_GRAVITY_PAGE_POINT,
    APSIS_GRAVITY_PAGE_COUNT
} ApsisGravityEditorPage;

typedef struct {
    uint8_t page;
} ApsisGravityPointEditorSession;

typedef struct {
    uint8_t page;
    uint8_t changed;
    uint8_t entered_point;
    uint8_t exited_point;
} ApsisGravityPointContextResult;

typedef struct {
    uint8_t handled;
    uint8_t state_changed;
    uint8_t selection_changed;
    uint8_t suppress_degree_request;
    uint8_t ring;
    uint8_t degree;
    uint8_t point_index;
    uint8_t enabled;
    int8_t offset;
    int8_t octave;
    uint16_t level_q15;
    uint16_t brightness_q15;
    uint16_t attack_ms;
    uint16_t release_ms;
} ApsisGravityPointControlResult;

void apsis_controls_gravity_point_editor_reset(
    ApsisGravityPointEditorSession *session);

uint8_t apsis_controls_gravity_point_context(
    ApsisGravityPointEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisGravityPointContextResult *out);

/* Edits only SELECTED POINT page. Plain degree keys are deliberately left for
 * the existing P9 Gravity request path. SEL+degree selects without audition and
 * asks target integration to filter direct degree-note edges from P9 dispatch. */
uint8_t apsis_controls_gravity_point_edit(
    ApsisGravityPointEditorSession *session,
    ApsisGravityState *gravity,
    const ApsisFm1InputFrame *input,
    ApsisGravityPointControlResult *out);

#endif
