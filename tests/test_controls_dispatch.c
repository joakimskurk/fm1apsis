#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "apsis_controls_dispatch.h"
#include "apsis_state.h"

static uint32_t bit(uint8_t b) { return 1u << b; }

static void reset_all(ApsisState *state, ApsisKeepBank *bank,
                      ApsisChainSequence *chain, ApsisMotherPending *pending,
                      ApsisControlsSession *session)
{
    apsis_state_default(state);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(pending);
    apsis_controls_session_reset(session);
}

static void clear_input(ApsisFm1InputFrame *input)
{
    memset(input, 0, sizeof(*input));
}

static void test_session_reset(void)
{
    ApsisControlsSession session;
    memset(&session, 0xA5, sizeof(session));
    apsis_controls_session_reset(&session);
    assert(session.clock_layer == APSIS_CLOCK_CONTROL_MAIN);
    assert(session.string_bank_base == 0u);
    assert(session.spiral_bank == 0u);
    assert(session.spiral_cursor == 0u);
    assert(session.space.page == APSIS_SPACE_CONTROL_ROOM);
    assert(session.space.source == APSIS_MOTHER_SOURCE_STRING);
    assert(session.bloom_grow.open_q15 == 0u);
    assert(session.bloom_grow.fired_mask == 0u);
}

static void test_mother_context_owns_navigation(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;
    uint16_t mix;
    uint8_t bloom_sound;

    reset_all(&state, &bank, &chain, &pending, &session);
    mix = state.mother.mix_q15[APSIS_MOTHER_SOURCE_STRING];
    bloom_sound = state.bloom.sound;
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    input.buttons_pressed = bit(APSIS_FM1_BTN_FX);
    input.buttons_held = bit(APSIS_FM1_BTN_SEL) | bit(APSIS_FM1_BTN_FX);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_MOTHER, &input, 7, &result);
    assert(result.changed == 1u);
    assert(result.suppress_navigation == 1u);
    assert(state.mother.mix_q15[APSIS_MOTHER_SOURCE_STRING] > mix);
    assert(state.bloom.sound != bloom_sound);

    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    input.buttons_held = bit(APSIS_FM1_BTN_REC);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_MOTHER, &input, 9, &result);
    assert(result.changed == 1u);
    assert(state.mother.sync == 1u);
}

static void test_clock_and_gravity_dispatch(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;
    uint32_t before_inner;
    uint8_t before_degree, before_ring, expected_ring;

    reset_all(&state, &bank, &chain, &pending, &session);
    before_inner = state.clock.inner_mask;
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    input.notes_pressed = 1u << 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_CLOCK, &input, 0, &result);
    assert(result.changed == 1u);
    assert(session.clock_layer == APSIS_CLOCK_CONTROL_BETWEEN);
    assert(state.clock.inner_mask == (before_inner ^ (1u << 1)));

    before_degree = state.gravity.degree;
    before_ring = state.gravity.selected_ring;
    expected_ring = (uint8_t)((before_ring + 1u) % 3u);
    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_OCT_UP);
    input.notes_pressed = 1u << 3;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_GRAVITY, &input, 0, &result);
    assert(result.changed == 1u);
    assert(state.gravity.selected_ring == expected_ring);
    assert(state.gravity.degree == before_degree);
    assert(result.gravity_request == 1u);
    assert(result.gravity_degree == 3u);
    assert(result.gravity_ring == expected_ring);
}

static void test_string_live_and_held_params(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;
    uint16_t before, resonance;

    reset_all(&state, &bank, &chain, &pending, &session);
    state.string.surface_mode = APSIS_STRING_DUAL;
    state.string.linked = 0u;
    clear_input(&input);
    input.notes_pressed = 1u << 8;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_STRING, &input, 0, &result);
    assert(result.string_pluck == 1u);
    assert(result.string_side == APSIS_SIDE_RIGHT);
    assert(state.string.right_split_q15 == result.string_split_q15);
    assert(result.string_split_q15 > APSIS_STRING_SPLIT_MID_Q15);

    before = state.string.right_split_q15;
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_STRING, &input, 0, &result);
    assert(result.changed == 1u);
    assert(result.string_pluck == 0u);
    assert(state.string.right_split_q15 > before);

    resonance = state.string.resonance_q15;
    apsis_mother_set_sync(&state.mother, &pending, 1u);
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_STRING, &input, 23, &result);
    assert(result.changed == 1u);
    assert(state.string.resonance_q15 == resonance);
    assert(apsis_mother_pending_count(&pending) == 1u);
    assert(pending.pending_start_abs == 23);
}

static void test_spiral_ordered_edit(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;

    reset_all(&state, &bank, &chain, &pending, &session);
    state.spiral.route_count = 0u;
    clear_input(&input);
    input.notes_pressed = 1u << 2;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPIRAL, &input, 0, &result);
    assert(state.spiral.route_count == 1u && state.spiral.route[0] == 2u);

    clear_input(&input);
    input.notes_pressed = 1u << 4;
    input.buttons_held = bit(APSIS_FM1_BTN_SEL);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPIRAL, &input, 0, &result);
    assert(state.spiral.route_count == 2u);
    assert(state.spiral.route[0] == 4u && state.spiral.route[1] == 2u);
    assert(session.spiral_cursor == 0u);

    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPIRAL, &input, 0, &result);
    assert(state.spiral.route[0] == 5u);

    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPIRAL, &input, 0, &result);
    assert(state.spiral.route_count == 1u && state.spiral.route[0] == 2u);
}

static void test_drift_bloom_space(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;
    uint16_t amount, depth, space_mix;

    reset_all(&state, &bank, &chain, &pending, &session);
    amount = state.drift.amount_q15;
    depth = state.drift.depth_q15;
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 1;
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_DRIFT, &input, 0, &result);
    assert(state.drift.amount_q15 > amount);
    assert(state.drift.depth_q15 > depth);

    clear_input(&input);
    input.notes_pressed = 1u << APSIS_BLOOM_CORE_KEY;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_BLOOM, &input, 0, &result);
    assert(result.bloom_core == 1u);
    assert(result.bloom_petal_mask == 0u);

    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    input.buttons_held = bit(APSIS_FM1_BTN_REC);
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 4;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_BLOOM, &input, 0, &result);
    assert(session.bloom_grow.open_q15 > 0u);
    assert(result.bloom_petal_mask != 0u);

    clear_input(&input);
    input.buttons_released = bit(APSIS_FM1_BTN_REC);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_BLOOM, &input, 0, &result);
    assert(session.bloom_grow.open_q15 == 0u);
    assert(session.bloom_grow.fired_mask == 0u);

    apsis_mother_set_sync(&state.mother, &pending, 1u);
    space_mix = state.space.mix_q15;
    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPACE, &input, 31, &result);
    assert(result.changed == 1u);
    assert(state.space.mix_q15 == space_mix);
    assert(apsis_mother_pending_count(&pending) == 1u);

    clear_input(&input);
    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    input.encoder_steps[APSIS_FM1_ENC_SELECT] = 1;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_SPACE, &input, 31, &result);
    assert(session.space.page == APSIS_SPACE_CONTROL_ECHO_SEND);
    assert(session.space.source == APSIS_MOTHER_SOURCE_DRIFT);
}

static void test_keep_chain_project_intents(void)
{
    ApsisState state;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending pending;
    ApsisControlsSession session;
    ApsisControlsDispatchResult result;
    ApsisFm1InputFrame input;
    uint16_t captured_drift;

    reset_all(&state, &bank, &chain, &pending, &session);
    state.drift.amount_q15 = APSIS_Q15_MILLI(321);
    captured_drift = state.drift.amount_q15;

    clear_input(&input);
    input.notes_pressed = 1u << 0;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_KEEP, &input, 0, &result);
    assert(bank.selected_slot == 0);
    assert(result.project_dirty == 1u);

    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_SAVE);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_KEEP, &input, 0, &result);
    assert(bank.slots[0].valid == 1u);
    assert(result.project_dirty == 1u);
    assert(result.suppress_navigation == 1u);

    state.drift.amount_q15 = APSIS_Q15_MILLI(777);
    clear_input(&input);
    input.notes_pressed = 1u << 0;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_KEEP, &input, 0, &result);
    assert(state.drift.amount_q15 == captured_drift);

    clear_input(&input);
    input.notes_pressed = 1u << 0;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_CHAIN, &input, 0, &result);
    assert(chain.count == 1u && result.project_dirty == 1u);
    clear_input(&input);
    input.notes_pressed = 1u << 0;
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_CHAIN, &input, 0, &result);
    assert(chain.count == 2u);

    chain.selected = 0;
    chain.pos = 0u;
    chain.playing = 1u;
    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    input.buttons_held = bit(APSIS_FM1_BTN_EDIT) | bit(APSIS_FM1_BTN_REC);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_CHAIN, &input, 0, &result);
    assert(chain.count == 1u);
    assert(result.project_dirty == 1u);
    assert(result.chain_apply_current == 1u);

    clear_input(&input);
    input.buttons_pressed = bit(APSIS_FM1_BTN_PLAY);
    input.buttons_held = bit(APSIS_FM1_BTN_SEL) | bit(APSIS_FM1_BTN_PLAY);
    apsis_controls_dispatch(&session, &state, &bank, &chain, &pending,
                            APSIS_SURFACE_CHAIN, &input, 0, &result);
    assert(result.chain_transport == APSIS_CHAIN_TRANSPORT_STOP);
}

int main(void)
{
    test_session_reset();
    test_mother_context_owns_navigation();
    test_clock_and_gravity_dispatch();
    test_string_live_and_held_params();
    test_spiral_ordered_edit();
    test_drift_bloom_space();
    test_keep_chain_project_intents();
    puts("PASS: P9H2 unified FM-1 control dispatcher preserves P9A-G contracts");
    return 0;
}
