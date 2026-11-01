#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_controls_gravity_point.h"

static uint32_t button_bit(uint8_t button)
{
    return 1u << button;
}

static ApsisGravityState gravity_fixture(void)
{
    ApsisGravityState g;
    uint8_t i;
    memset(&g, 0, sizeof(g));
    g.selected_ring = 1u;
    g.selected_point_degree = 2u;
    for (i = 0u; i < APSIS_GRAVITY_POINTS; ++i) {
        g.points[i].enabled = 1u;
        g.points[i].offset = 0;
        g.points[i].octave = 0;
        g.points[i].level_q15 = APSIS_Q15_MILLI(500);
        g.points[i].brightness_q15 = APSIS_Q15_MILLI(500);
        g.points[i].attack_ms = 40u;
        g.points[i].release_ms = 900u;
    }
    return g;
}

static void test_context(void)
{
    ApsisGravityPointEditorSession session;
    ApsisGravityPointContextResult out;
    ApsisFm1InputFrame input = {0};

    apsis_controls_gravity_point_editor_reset(&session);
    assert(session.page == APSIS_GRAVITY_PAGE_HARMONY);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    assert(!apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_MOTHER, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_HARMONY);

    assert(apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_GRAVITY, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_TIMING);

    assert(apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_GRAVITY, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_POINT);
    assert(out.entered_point == 1u);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    assert(apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_GRAVITY, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_HARMONY && out.exited_point == 1u);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = -1;
    assert(apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_GRAVITY, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_POINT);

    memset(&input, 0, sizeof(input));
    assert(apsis_controls_gravity_point_context(
        &session, APSIS_SURFACE_STRING, &input, &out));
    assert(session.page == APSIS_GRAVITY_PAGE_HARMONY && out.exited_point == 1u);
}

static void test_non_point_noop(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_TIMING};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisGravityState before = gravity;
    ApsisFm1InputFrame input = {0};

    input.notes_pressed = 1u << 5;
    input.buttons_held = button_bit(APSIS_FM1_BTN_SEL);
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 3;
    assert(!apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(memcmp(&gravity, &before, sizeof(gravity)) == 0);
    assert(out.handled == 0u);
}

static void test_plain_degree_remains_unsuppressed(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_POINT};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisFm1InputFrame input = {0};

    input.notes_pressed = 1u << 6;
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(out.handled == 1u);
    assert(out.suppress_degree_request == 0u);
    assert(out.state_changed == 0u);
    assert(gravity.selected_point_degree == 2u);
}

static void test_select_without_audition(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_POINT};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisGravityPoint before[APSIS_GRAVITY_POINTS];
    ApsisFm1InputFrame input = {0};

    gravity.selected_ring = 2u;
    memcpy(before, gravity.points, sizeof(before));
    input.notes_pressed = (1u << 5) | (1u << 7);
    input.buttons_held = button_bit(APSIS_FM1_BTN_SEL);
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(out.suppress_degree_request == 1u);
    assert(out.selection_changed == 1u && out.state_changed == 1u);
    assert(gravity.selected_point_degree == 5u);
    assert(out.ring == 2u && out.degree == 5u && out.point_index == 17u);
    assert(memcmp(gravity.points, before, sizeof(before)) == 0);
}

static void test_selected_point_controls(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_POINT};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisGravityPoint before[APSIS_GRAVITY_POINTS];
    ApsisFm1InputFrame input = {0};
    ApsisGravityPoint *point;
    uint8_t index;

    gravity.selected_ring = 1u;
    gravity.selected_point_degree = 4u;
    index = (uint8_t)(4u * APSIS_GRAVITY_RINGS + 1u);
    point = &gravity.points[index];
    memcpy(before, gravity.points, sizeof(before));

    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    input.encoder_steps[APSIS_FM1_ENC_K2] = -1;
    input.encoder_steps[APSIS_FM1_ENC_K3] = 2;
    input.encoder_steps[APSIS_FM1_ENC_K4] = -2;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 3;
    input.encoder_steps[APSIS_FM1_ENC_PRESETS] = 2;
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));

    assert(point->enabled == 0u);
    assert(point->offset == 1);
    assert(point->octave == -1);
    assert(point->level_q15 ==
           (uint16_t)(APSIS_Q15_MILLI(500) + 2u * APSIS_Q15_MILLI(10)));
    assert(point->brightness_q15 ==
           (uint16_t)(APSIS_Q15_MILLI(500) - 2u * APSIS_Q15_MILLI(10)));
    assert(point->attack_ms == 52u);
    assert(point->release_ms == 950u);
    assert(out.point_index == index && out.state_changed == 1u);

    before[index] = *point;
    assert(memcmp(gravity.points, before, sizeof(before)) == 0);
}

static void test_bounds(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_POINT};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisFm1InputFrame input = {0};
    ApsisGravityPoint *point = &gravity.points[2u * APSIS_GRAVITY_RINGS + 1u];

    input.encoder_steps[APSIS_FM1_ENC_K1] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_K2] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_K3] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_K4] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_PRESETS] = 32767;
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(point->offset == 2 && point->octave == 1);
    assert(point->level_q15 == APSIS_Q15_MILLI(1250));
    assert(point->brightness_q15 == APSIS_Q15_MILLI(780));
    assert(point->attack_ms == 220u && point->release_ms == 3000u);

    input.encoder_steps[APSIS_FM1_ENC_K1] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_K2] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_K3] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_K4] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_PRESETS] = -32768;
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(point->offset == -2 && point->octave == -1);
    assert(point->level_q15 == APSIS_Q15_MILLI(150));
    assert(point->brightness_q15 == APSIS_Q15_MILLI(150));
    assert(point->attack_ms == 12u && point->release_ms == 250u);
}

static void test_same_frame_selection_edits_new_point(void)
{
    ApsisGravityPointEditorSession session = {APSIS_GRAVITY_PAGE_POINT};
    ApsisGravityPointControlResult out;
    ApsisGravityState gravity = gravity_fixture();
    ApsisFm1InputFrame input = {0};
    uint8_t old_index = (uint8_t)(2u * APSIS_GRAVITY_RINGS + 1u);
    uint8_t new_index = (uint8_t)(6u * APSIS_GRAVITY_RINGS + 1u);
    uint16_t old_level = gravity.points[old_index].level_q15;

    input.notes_pressed = 1u << 6;
    input.buttons_held = button_bit(APSIS_FM1_BTN_SEL);
    input.encoder_steps[APSIS_FM1_ENC_K3] = 1;
    assert(apsis_controls_gravity_point_edit(&session, &gravity, &input, &out));
    assert(gravity.selected_point_degree == 6u);
    assert(out.suppress_degree_request == 1u && out.point_index == new_index);
    assert(gravity.points[old_index].level_q15 == old_level);
    assert(gravity.points[new_index].level_q15 ==
           (uint16_t)(APSIS_Q15_MILLI(500) + APSIS_Q15_MILLI(10)));
}

int main(void)
{
    test_context();
    test_non_point_noop();
    test_plain_degree_remains_unsuppressed();
    test_select_without_audition();
    test_selected_point_controls();
    test_bounds();
    test_same_frame_selection_edits_new_point();
    puts("PASS: P10G Gravity selected-point editor is bounded, live-grid-backed and audition-safe");
    return 0;
}
