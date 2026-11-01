#include <stdio.h>
#include <string.h>
#include "apsis_controls_space.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)

static void fixture(ApsisState *state, ApsisSpaceControlState *control)
{
    memset(state, 0, sizeof *state);
    state->space.mix_q15 = APSIS_Q15_MILLI(480);
    state->space.decay_ms = 8400u;
    state->space.predelay_ms = 72u;
    state->space.damping_hz = 6200u;
    state->space.echo_time_ms = 362u;
    state->space.echo_feedback_q15 = APSIS_Q15_MILLI(180);
    state->string.send_q15 = APSIS_Q15_MILLI(720);
    state->drift.send_q15 = APSIS_Q15_MILLI(200);
    state->clock.send_q15 = APSIS_Q15_MILLI(60);
    state->spiral.send_q15 = APSIS_Q15_MILLI(620);
    state->bloom.send_q15 = APSIS_Q15_MILLI(580);
    apsis_controls_space_reset(control);
}

static int test_room_page(void)
{
    ApsisState state, before;
    ApsisSpaceControlState control;
    ApsisFm1InputFrame in;
    ApsisSpaceControlEdit edits[4];
    uint8_t count;

    fixture(&state, &control);
    before = state;
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = 2;
    in.encoder_steps[APSIS_FM1_ENC_K2] = -5;
    in.encoder_steps[APSIS_FM1_ENC_K3] = 100;
    in.encoder_steps[APSIS_FM1_ENC_K4] = -100;
    count = apsis_controls_space_edits(&state, &control, &in, edits);
    CHECK(count == 4u);
    CHECK(edits[0].param == APSIS_SPACE_PARAM_RETURN);
    CHECK(edits[0].value == (int32_t)(APSIS_Q15_MILLI(480) + 2u * APSIS_Q15_MILLI(10)));
    CHECK(edits[1].param == APSIS_SPACE_PARAM_DECAY_MS && edits[1].value == 7900);
    CHECK(edits[2].param == APSIS_SPACE_PARAM_PREDELAY_MS && edits[2].value == 140);
    CHECK(edits[3].param == APSIS_SPACE_PARAM_DAMPING_HZ && edits[3].value == 2200);
    CHECK(memcmp(&state, &before, sizeof state) == 0); /* intents only */

    state.space.mix_q15 = APSIS_Q15_MILLI(719);
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = 5;
    CHECK(apsis_controls_space_edits(&state, &control, &in, edits) == 1u);
    CHECK(edits[0].value == APSIS_Q15_MILLI(720));
    return 0;
}

static int test_page_and_source_cursor(void)
{
    ApsisState state;
    ApsisSpaceControlState control;
    fixture(&state, &control);
    CHECK(control.page == APSIS_SPACE_CONTROL_ROOM && control.source == APSIS_MOTHER_SOURCE_STRING);
    CHECK(!apsis_controls_space_source_step(&control, 1));
    CHECK(apsis_controls_space_page_step(&control, 1));
    CHECK(control.page == APSIS_SPACE_CONTROL_ECHO_SEND);
    CHECK(apsis_controls_space_source_step(&control, 4));
    CHECK(control.source == APSIS_MOTHER_SOURCE_BLOOM);
    CHECK(apsis_controls_space_source_step(&control, 1));
    CHECK(control.source == APSIS_MOTHER_SOURCE_STRING);
    CHECK(apsis_controls_space_source_step(&control, -1));
    CHECK(control.source == APSIS_MOTHER_SOURCE_BLOOM);
    CHECK(apsis_controls_space_page_step(&control, -1));
    CHECK(control.page == APSIS_SPACE_CONTROL_ROOM);
    return 0;
}

static int test_echo_send_page(void)
{
    ApsisState state, before;
    ApsisSpaceControlState control;
    ApsisFm1InputFrame in;
    ApsisSpaceControlEdit edits[4];
    uint8_t count;

    fixture(&state, &control);
    CHECK(apsis_controls_space_page_step(&control, 1));
    CHECK(apsis_controls_space_source_step(&control, 2));
    CHECK(control.source == APSIS_MOTHER_SOURCE_CLOCK);
    before = state;
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = -400;
    in.encoder_steps[APSIS_FM1_ENC_K2] = 100;
    in.encoder_steps[APSIS_FM1_ENC_K3] = 5;
    in.encoder_steps[APSIS_FM1_ENC_K4] = 99;
    count = apsis_controls_space_edits(&state, &control, &in, edits);
    CHECK(count == 3u); /* K4 intentionally reserved */
    CHECK(edits[0].param == APSIS_SPACE_PARAM_ECHO_TIME_MS && edits[0].value == 80);
    CHECK(edits[1].param == APSIS_SPACE_PARAM_ECHO_FEEDBACK && edits[1].value == APSIS_Q15_MILLI(580));
    CHECK(edits[2].param == APSIS_SPACE_PARAM_SOURCE_SEND);
    CHECK(edits[2].source == APSIS_MOTHER_SOURCE_CLOCK);
    CHECK(edits[2].value == (int32_t)(APSIS_Q15_MILLI(60) + 5u * APSIS_Q15_MILLI(10)));
    CHECK(memcmp(&state, &before, sizeof state) == 0);
    return 0;
}

int main(void)
{
    CHECK(test_room_page() == 0);
    CHECK(test_page_and_source_cursor() == 0);
    CHECK(test_echo_send_page() == 0);
    puts("PASS: APSIS P9G2 source-locked SPACE physical-control contract");
    return 0;
}
