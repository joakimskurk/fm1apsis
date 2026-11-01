#ifndef APSIS_CONTROLS_SPIRAL_ATTRACTOR_H
#define APSIS_CONTROLS_SPIRAL_ATTRACTOR_H

#include "apsis_controls.h"
#include "apsis_spiral.h"

typedef struct {
    uint8_t active;
    uint8_t selected_cell;
} ApsisSpiralAttractorEditorSession;

typedef struct {
    uint8_t active;
    uint8_t entered;
    uint8_t exited;
} ApsisSpiralAttractorContextResult;

typedef struct {
    uint8_t handled;
    uint8_t state_changed;
    uint8_t rerolled;
    uint8_t suppress_route_dispatch;
    uint8_t selected_cell;
    uint8_t rate;
    uint8_t length;
    uint16_t lock_q15;
    uint16_t density_q15;
    uint16_t energy_q15;
    uint32_t toggled_gates;
} ApsisSpiralAttractorControlResult;

void apsis_controls_spiral_attractor_editor_reset(
    ApsisSpiralAttractorEditorSession *session);

/* ALGORITHM toggles ROUTE/ATTRACTOR only while the current surface is SPIRAL.
 * Leaving SPIRAL clears this UI-only editor context. */
uint8_t apsis_controls_spiral_attractor_context(
    ApsisSpiralAttractorEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisSpiralAttractorContextResult *out);

/* Pure control/state editor above the frozen Spiral attractor authority.
 * Active editor frames must suppress ordinary P9 Spiral route dispatch.
 * REC delegates reroll to apsis_spiral_reset_attractor(..., 1). */
uint8_t apsis_controls_spiral_attractor_edit(
    ApsisSpiralAttractorEditorSession *session,
    ApsisSpiralState *spiral,
    const ApsisFm1InputFrame *input,
    ApsisSpiralAttractorControlResult *out);

#endif
