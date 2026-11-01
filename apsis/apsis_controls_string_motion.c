#include "apsis_controls_string_motion.h"

static uint32_t p10c_motion_bit(uint8_t button)
{
    return 1u << button;
}

static uint8_t p10c_motion_target_normalize(const ApsisState *state,
                                            uint8_t target)
{
    if (!state || state->string.surface_mode == APSIS_STRING_CLASSIC)
        return APSIS_STRING_MOTION_TARGET_RIGHT;
    if (state->string.linked)
        return APSIS_STRING_MOTION_TARGET_BOTH;
    return target <= APSIS_STRING_MOTION_TARGET_BOTH
        ? target : APSIS_STRING_MOTION_TARGET_BOTH;
}

static uint8_t p10c_motion_target_step(const ApsisState *state,
                                       uint8_t current,
                                       int32_t detents)
{
    int32_t next;
    current = p10c_motion_target_normalize(state, current);
    if (!state || state->string.surface_mode == APSIS_STRING_CLASSIC ||
        state->string.linked || !detents)
        return current;
    next = (int32_t)current + detents;
    next %= 3;
    if (next < 0) next += 3;
    return (uint8_t)next;
}

uint8_t apsis_controls_string_motion(const ApsisState *state,
                                     uint8_t current_target,
                                     const ApsisFm1InputFrame *input,
                                     ApsisStringMotionControlResult *out)
{
    uint32_t rec, sel, oct_down;
    uint8_t target, next_target;
    uint8_t rec_edge, oct_edge, rec_held, oct_held;

    if (!out) return 0u;
    out->action = APSIS_STRING_MOTION_ACTION_NONE;
    out->record_target = p10c_motion_target_normalize(state, current_target);
    out->target_changed = 0u;
    out->suppress_string_bank = 0u;
    if (!state || !input) return 0u;

    rec = p10c_motion_bit(APSIS_FM1_BTN_REC);
    sel = p10c_motion_bit(APSIS_FM1_BTN_SEL);
    oct_down = p10c_motion_bit(APSIS_FM1_BTN_OCT_DOWN);
    rec_edge = (input->buttons_pressed & rec) != 0u;
    oct_edge = (input->buttons_pressed & oct_down) != 0u;
    rec_held = (input->buttons_held & rec) != 0u;
    oct_held = (input->buttons_held & oct_down) != 0u;

    /* CLEAR is symmetric in chord order and wins over both ordinary REC arm and
     * OCT- String-bank navigation. */
    if ((rec_edge && oct_held) || (oct_edge && rec_held)) {
        out->action = APSIS_STRING_MOTION_ACTION_CLEAR;
        out->suppress_string_bank = 1u;
        return 1u;
    }

    target = out->record_target;
    next_target = p10c_motion_target_step(
        state, target, input->encoder_steps[APSIS_FM1_ENC_PRESETS]);
    if (next_target != target) {
        out->record_target = next_target;
        out->target_changed = 1u;
    }

    if (rec_edge) {
        out->action = (input->buttons_held & sel)
            ? APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE
            : APSIS_STRING_MOTION_ACTION_ARM_TOGGLE;
        return 1u;
    }
    return out->target_changed;
}
