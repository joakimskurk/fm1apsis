#ifndef APSIS_CONTROLS_STRING_ATTRACTOR_H
#define APSIS_CONTROLS_STRING_ATTRACTOR_H

#include "apsis_controls.h"
#include "apsis_string_attractor.h"

typedef struct {
    uint8_t active;
} ApsisStringAttractorEditorSession;

typedef struct {
    uint8_t active;
    uint8_t entered;
    uint8_t exited;
} ApsisStringAttractorContextResult;

typedef struct {
    uint8_t handled;
    uint8_t state_changed;
    uint8_t rerolled;
    uint8_t suppress_string_dispatch;
    uint8_t suppress_motion;
    uint16_t toggled_gates;
    uint16_t seed;
} ApsisStringAttractorControlResult;

void apsis_controls_string_attractor_editor_reset(
    ApsisStringAttractorEditorSession *session);

/* UI-only context selection. SEL+ENV enters the editor, plain ENV returns to
 * normal STRING performance, and observing a non-STRING surface with no ENV
 * edge clears stale editor context. No musical state is stored here. */
uint8_t apsis_controls_string_attractor_context(
    ApsisStringAttractorEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisStringAttractorContextResult *out);

/* Pure Phase-10E state edit over the existing String Attractor fields.
 * While editor_active is true the caller must suppress ordinary P9 STRING
 * performance and P10 Motion interpretation for the same input frame.
 *
 * - first normalized N (4..16) note-key edges toggle gate bits;
 * - REC edge advances the existing 1..9999 seed permutation;
 * - no scheduler, DSP, cell store or RNG state is created here. */
uint8_t apsis_controls_string_attractor_edit(
    ApsisStringState *string,
    uint8_t editor_active,
    const ApsisFm1InputFrame *input,
    ApsisStringAttractorControlResult *out);

#endif