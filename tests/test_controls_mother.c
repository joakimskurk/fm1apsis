#include <stdio.h>
#include <string.h>
#include "apsis_controls_mother.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t bit(uint8_t b) { return 1u << b; }

static void fixture(ApsisState *state, ApsisMotherPending *pending)
{
    uint8_t i;
    memset(state, 0, sizeof *state);
    for (i = 0u; i < APSIS_MOTHER_SOURCE_COUNT; ++i) state->mother.mix_q15[i] = APSIS_Q15_ONE;
    state->mother.horizon = 8u;
    state->string.sound = 1u;
    state->drift.sound = 1u;
    state->clock.sound = 1u;
    state->spiral.sound = 1u;
    state->bloom.sound = 1u;
    apsis_mother_pending_reset(pending);
}

static int test_live_mix_mapping(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisFm1InputFrame in;
    ApsisMotherMixEdit edits[APSIS_MOTHER_SOURCE_COUNT];
    uint8_t count, i;

    fixture(&state, &pending);
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    in.encoder_steps[APSIS_FM1_ENC_K2] = -40;
    in.encoder_steps[APSIS_FM1_ENC_K3] = 20;
    in.encoder_steps[APSIS_FM1_ENC_SELECT] = 9;
    count = apsis_controls_mother_mix_edits(&state.mother, &in, edits);
    CHECK(count == 4u);
    CHECK(edits[0].source == APSIS_MOTHER_SOURCE_STRING);
    CHECK(edits[0].value_q15 == (uint16_t)(APSIS_Q15_ONE + APSIS_MOTHER_MIX_STEP_Q15));
    CHECK(edits[1].source == APSIS_MOTHER_SOURCE_DRIFT && edits[1].value_q15 == 0u);
    CHECK(edits[2].source == APSIS_MOTHER_SOURCE_CLOCK && edits[2].value_q15 == APSIS_MOTHER_MIX_MAX_Q15);
    CHECK(edits[3].source == APSIS_MOTHER_SOURCE_BLOOM && edits[3].value_q15 == APSIS_MOTHER_MIX_MAX_Q15);

    state.mother.sync = 1u;
    CHECK(apsis_mother_queue_param(&pending, 17u, 1234, 10));
    for (i = 0u; i < count; ++i) CHECK(apsis_controls_mother_apply_mix(&state.mother, &edits[i]));
    CHECK(pending.count == 1u); /* mixer remains live under ->1 */
    CHECK(state.mother.mix_q15[APSIS_MOTHER_SOURCE_DRIFT] == 0u);
    CHECK(apsis_mother_should_hold(&state.mother, &pending, APSIS_MOTHER_DOMAIN_MOTHER) == 0u);
    return 0;
}

static int test_live_mutes(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisFm1InputFrame in;
    uint8_t source = 99u;

    fixture(&state, &pending);
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_ENV) | bit(APSIS_FM1_BTN_FX);
    CHECK(!apsis_controls_mother_mute_source_from_fm1(&in, &source));
    in.buttons_held = bit(APSIS_FM1_BTN_SEL);
    CHECK(apsis_controls_mother_mute_source_from_fm1(&in, &source) && source == APSIS_MOTHER_SOURCE_STRING);
    CHECK(apsis_controls_mother_toggle_source_mute(&state, source));
    CHECK(!state.string.sound && (state.mother.mute_mask & (1u << APSIS_MOTHER_SOURCE_STRING)));
    CHECK(apsis_controls_mother_toggle_source_mute(&state, source));
    CHECK(state.string.sound && !(state.mother.mute_mask & (1u << APSIS_MOTHER_SOURCE_STRING)));

    memset(&in, 0, sizeof in);
    in.buttons_held = bit(APSIS_FM1_BTN_SEL);
    in.buttons_pressed = bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_mother_mute_source_from_fm1(&in, &source) && source == APSIS_MOTHER_SOURCE_BLOOM);
    CHECK(apsis_controls_mother_toggle_source_mute(&state, source));
    CHECK(!state.bloom.sound && (state.mother.mute_mask & (1u << APSIS_MOTHER_SOURCE_BLOOM)));
    return 0;
}

static int test_sync_and_horizon(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisFm1InputFrame in;

    fixture(&state, &pending);
    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    CHECK(apsis_controls_mother_sync_from_fm1(&state.mother, &pending, &in));
    CHECK(state.mother.sync == 1u);
    CHECK(apsis_mother_queue_param(&pending, 22u, 4567, 100));
    CHECK(pending.count == 1u && pending.pending_start_abs == 100);

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_UP);
    CHECK(apsis_controls_mother_horizon_from_fm1(&state.mother, &pending, &in, 123));
    CHECK(state.mother.horizon == 16u && pending.pending_start_abs == 123);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_DOWN);
    CHECK(apsis_controls_mother_horizon_from_fm1(&state.mother, &pending, &in, 124));
    CHECK(state.mother.horizon == 8u && pending.pending_start_abs == 124);

    state.mother.horizon = 1u;
    CHECK(apsis_controls_mother_horizon_from_fm1(&state.mother, &pending, &in, 125));
    CHECK(state.mother.horizon == 32u);
    in.buttons_pressed = bit(APSIS_FM1_BTN_OCT_DOWN) | bit(APSIS_FM1_BTN_OCT_UP);
    CHECK(!apsis_controls_mother_horizon_from_fm1(&state.mother, &pending, &in, 126));
    CHECK(state.mother.horizon == 32u);

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    CHECK(apsis_controls_mother_sync_from_fm1(&state.mother, &pending, &in));
    CHECK(state.mother.sync == 0u);
    CHECK(pending.count == 0u && pending.pending_start_abs == -1);
    return 0;
}

int main(void)
{
    CHECK(test_live_mix_mapping() == 0);
    CHECK(test_live_mutes() == 0);
    CHECK(test_sync_and_horizon() == 0);
    puts("PASS: APSIS P9G1 source-locked MOTHER physical-control contract");
    return 0;
}
