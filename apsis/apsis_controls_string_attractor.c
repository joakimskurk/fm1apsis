#include "apsis_controls_string_attractor.h"

#define P10E_REROLL_STEP 7919u
#define P10E_SEED_MAX 9999u
#define P10E_SEED_DEFAULT 317u

static uint32_t p10e_button_bit(uint8_t button)
{
    return 1u << button;
}

static uint8_t p10e_length(uint8_t n)
{
    if (n < 4u) return 4u;
    if (n > APSIS_STRING_ATTRACTOR_CELLS_MAX)
        return APSIS_STRING_ATTRACTOR_CELLS_MAX;
    return n;
}

static uint16_t p10e_seed(uint16_t seed)
{
    if (!seed) return P10E_SEED_DEFAULT;
    if (seed > P10E_SEED_MAX) return P10E_SEED_MAX;
    return seed;
}

static uint16_t p10e_reroll_seed(uint16_t seed)
{
    uint32_t normalized = p10e_seed(seed);
    return (uint16_t)(((normalized - 1u + P10E_REROLL_STEP) % P10E_SEED_MAX) + 1u);
}

static uint16_t p10e_gate_mask(uint8_t length)
{
    if (length >= 16u) return 0xffffu;
    return (uint16_t)((1u << length) - 1u);
}

void apsis_controls_string_attractor_editor_reset(
    ApsisStringAttractorEditorSession *session)
{
    if (session) session->active = 0u;
}

uint8_t apsis_controls_string_attractor_context(
    ApsisStringAttractorEditorSession *session,
    uint8_t current_surface,
    const ApsisFm1InputFrame *input,
    ApsisStringAttractorContextResult *out)
{
    uint8_t before, next;
    uint32_t env, sel;

    if (out) {
        out->active = session ? session->active : 0u;
        out->entered = 0u;
        out->exited = 0u;
    }
    if (!session) return 0u;

    before = session->active ? 1u : 0u;
    next = before;
    env = p10e_button_bit(APSIS_FM1_BTN_ENV);
    sel = p10e_button_bit(APSIS_FM1_BTN_SEL);

    if (input && (input->buttons_pressed & env))
        next = (input->buttons_held & sel) ? 1u : 0u;
    else if (current_surface != APSIS_SURFACE_STRING)
        next = 0u;

    session->active = next;
    if (out) {
        out->active = next;
        out->entered = (uint8_t)(!before && next);
        out->exited = (uint8_t)(before && !next);
    }
    return before != next;
}

uint8_t apsis_controls_string_attractor_edit(
    ApsisStringState *string,
    uint8_t editor_active,
    const ApsisFm1InputFrame *input,
    ApsisStringAttractorControlResult *out)
{
    uint16_t active_mask, toggled;
    uint16_t seed;
    uint8_t length;
    uint8_t rec_edge;

    if (out) {
        out->handled = 0u;
        out->state_changed = 0u;
        out->rerolled = 0u;
        out->suppress_string_dispatch = 0u;
        out->suppress_motion = 0u;
        out->toggled_gates = 0u;
        out->seed = string ? string->filter_attractor_seed : 0u;
    }
    if (!string || !input || !editor_active) return 0u;

    if (out) {
        out->handled = 1u;
        out->suppress_string_dispatch = 1u;
        out->suppress_motion = 1u;
    }

    length = p10e_length(string->filter_attractor_length);
    active_mask = p10e_gate_mask(length);
    toggled = (uint16_t)(input->notes_pressed & (uint32_t)active_mask);
    if (toggled) {
        string->filter_attractor_gates ^= toggled;
        if (out) {
            out->state_changed = 1u;
            out->toggled_gates = toggled;
        }
    }

    rec_edge = (input->buttons_pressed & p10e_button_bit(APSIS_FM1_BTN_REC)) != 0u;
    if (rec_edge) {
        seed = p10e_reroll_seed(string->filter_attractor_seed);
        string->filter_attractor_seed = seed;
        if (out) {
            out->state_changed = 1u;
            out->rerolled = 1u;
            out->seed = seed;
        }
    } else if (out) {
        out->seed = string->filter_attractor_seed;
    }

    return 1u;
}
