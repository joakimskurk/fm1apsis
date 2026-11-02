#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_controls.h"
#include "apsis_controls_string_motion.h"

static void clear_input(ApsisFm1InputFrame *in)
{
    memset(in, 0, sizeof(*in));
}

static uint32_t bit(uint8_t b)
{
    return 1u << b;
}

static void test_target_modes(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisStringMotionControlResult r;
    apsis_state_default(&s);
    clear_input(&in);

    s.string.surface_mode = APSIS_STRING_CLASSIC;
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = 1;
    assert(!apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_LEFT, &in, &r));
    assert(r.record_target == APSIS_STRING_MOTION_TARGET_RIGHT);

    clear_input(&in);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 1u;
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = -1;
    assert(!apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_LEFT, &in, &r));
    assert(r.record_target == APSIS_STRING_MOTION_TARGET_BOTH);

    clear_input(&in);
    s.string.linked = 0u;
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = 1;
    assert(apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_LEFT, &in, &r));
    assert(r.target_changed);
    assert(r.record_target == APSIS_STRING_MOTION_TARGET_RIGHT);
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = 1;
    assert(apsis_controls_string_motion(&s, r.record_target, &in, &r));
    assert(r.record_target == APSIS_STRING_MOTION_TARGET_BOTH);
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = 1;
    assert(apsis_controls_string_motion(&s, r.record_target, &in, &r));
    assert(r.record_target == APSIS_STRING_MOTION_TARGET_LEFT);
}

static void test_rec_actions(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisStringMotionControlResult r;
    apsis_state_default(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 0u;

    clear_input(&in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    in.buttons_held = bit(APSIS_FM1_BTN_REC);
    assert(apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_BOTH, &in, &r));
    assert(r.action == APSIS_STRING_MOTION_ACTION_ARM_TOGGLE);

    clear_input(&in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    in.buttons_held = bit(APSIS_FM1_BTN_REC) | bit(APSIS_FM1_BTN_SEL);
    assert(apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_BOTH, &in, &r));
    assert(r.action == APSIS_STRING_MOTION_ACTION_PLAY_TOGGLE);
}

static void test_clear_chord_both_orders(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisStringMotionControlResult r;
    apsis_state_default(&s);

    clear_input(&in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    in.buttons_held = bit(APSIS_FM1_BTN_REC) | bit(APSIS_FM1_BTN_OCT_DOWN);
    assert(apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_RIGHT, &in, &r));
    assert(r.action == APSIS_STRING_MOTION_ACTION_CLEAR);
    assert(r.suppress_string_bank);

    clear_input(&in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_DOWN);
    in.buttons_held = bit(APSIS_FM1_BTN_OCT_DOWN) | bit(APSIS_FM1_BTN_REC);
    assert(apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_RIGHT, &in, &r));
    assert(r.action == APSIS_STRING_MOTION_ACTION_CLEAR);
    assert(r.suppress_string_bank);
}

static void test_unrelated_input_is_inert(void)
{
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisStringMotionControlResult r;
    apsis_state_default(&s);
    clear_input(&in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_UP);
    assert(!apsis_controls_string_motion(&s, APSIS_STRING_MOTION_TARGET_RIGHT, &in, &r));
    assert(r.action == APSIS_STRING_MOTION_ACTION_NONE);
    assert(!r.suppress_string_bank);
}

int main(void)
{
    test_target_modes();
    test_rec_actions();
    test_clear_chord_both_orders();
    test_unrelated_input_is_inert();
    puts("PASS: Phase 10C1 String Motion physical-control contract");
    return 0;
}
