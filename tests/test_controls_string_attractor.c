#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_controls_string_attractor.h"

static uint32_t button_bit(uint8_t button)
{
    return 1u << button;
}

static void test_context(void)
{
    ApsisStringAttractorEditorSession session;
    ApsisStringAttractorContextResult out;
    ApsisFm1InputFrame input = {0};

    apsis_controls_string_attractor_editor_reset(&session);
    assert(session.active == 0u);

    input.buttons_pressed = button_bit(APSIS_FM1_BTN_ENV);
    input.buttons_held = button_bit(APSIS_FM1_BTN_SEL);
    assert(apsis_controls_string_attractor_context(
        &session, APSIS_SURFACE_MOTHER, &input, &out));
    assert(session.active == 1u && out.active == 1u);
    assert(out.entered == 1u && out.exited == 0u);

    memset(&input, 0, sizeof(input));
    assert(!apsis_controls_string_attractor_context(
        &session, APSIS_SURFACE_STRING, &input, &out));
    assert(session.active == 1u && out.active == 1u);

    input.buttons_pressed = button_bit(APSIS_FM1_BTN_ENV);
    assert(apsis_controls_string_attractor_context(
        &session, APSIS_SURFACE_STRING, &input, &out));
    assert(session.active == 0u && out.exited == 1u);

    input.buttons_held = button_bit(APSIS_FM1_BTN_SEL);
    assert(apsis_controls_string_attractor_context(
        &session, APSIS_SURFACE_STRING, &input, &out));
    assert(session.active == 1u && out.entered == 1u);

    memset(&input, 0, sizeof(input));
    assert(apsis_controls_string_attractor_context(
        &session, APSIS_SURFACE_MOTHER, &input, &out));
    assert(session.active == 0u && out.exited == 1u);
}

static void test_inactive_noop(void)
{
    ApsisStringState string = {0};
    ApsisStringState before;
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};

    string.filter_attractor_length = 8u;
    string.filter_attractor_gates = 0x9535u;
    string.filter_attractor_seed = 317u;
    before = string;
    input.notes_pressed = 0xffffu;
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);

    assert(!apsis_controls_string_attractor_edit(&string, 0u, &input, &out));
    assert(memcmp(&string, &before, sizeof(string)) == 0);
    assert(out.handled == 0u);
    assert(out.state_changed == 0u);
    assert(out.suppress_string_dispatch == 0u);
    assert(out.suppress_motion == 0u);
}

static void test_gate_bounds_and_hidden_bits(void)
{
    ApsisStringState string = {0};
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};
    uint16_t before;

    string.filter_attractor_length = 8u;
    string.filter_attractor_gates = 0x9535u;
    string.filter_attractor_seed = 317u;
    before = string.filter_attractor_gates;
    input.notes_pressed = (1u << 0) | (1u << 3) | (1u << 7) | (1u << 15);

    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(out.handled == 1u);
    assert(out.suppress_string_dispatch == 1u);
    assert(out.suppress_motion == 1u);
    assert(out.state_changed == 1u);
    assert(out.toggled_gates == 0x0089u);
    assert(string.filter_attractor_gates == (uint16_t)(before ^ 0x0089u));
    assert((string.filter_attractor_gates & 0xff00u) == (before & 0xff00u));

    memset(&input, 0, sizeof(input));
    input.notes_pressed = 1u << 15;
    before = string.filter_attractor_gates;
    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(out.handled == 1u && out.state_changed == 0u);
    assert(out.toggled_gates == 0u);
    assert(string.filter_attractor_gates == before);
}

static void test_length_normalization(void)
{
    ApsisStringState string = {0};
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};

    string.filter_attractor_length = 0u;
    input.notes_pressed = (1u << 3) | (1u << 4);
    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(out.toggled_gates == (1u << 3));
    assert(string.filter_attractor_gates == (1u << 3));
    assert(string.filter_attractor_length == 0u);

    memset(&input, 0, sizeof(input));
    string.filter_attractor_length = 99u;
    string.filter_attractor_gates = 0u;
    input.notes_pressed = (1u << 15) | (1u << 16);
    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(out.toggled_gates == (1u << 15));
    assert(string.filter_attractor_gates == (1u << 15));
    assert(string.filter_attractor_length == 99u);
}

static void test_reroll_changes_only_seed(void)
{
    ApsisStringState string = {0};
    ApsisStringState before;
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};

    string.filter_attractor_length = 8u;
    string.filter_attractor_gates = 0x9535u;
    string.filter_attractor_lock_q15 = APSIS_Q15_MILLI(860);
    string.filter_attractor_seed = 317u;
    before = string;
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);

    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(out.rerolled == 1u && out.state_changed == 1u);
    assert(out.seed == 8236u && string.filter_attractor_seed == 8236u);
    assert(out.suppress_motion == 1u);

    string.filter_attractor_seed = before.filter_attractor_seed;
    assert(memcmp(&string, &before, sizeof(string)) == 0);

    string.filter_attractor_seed = 9999u;
    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(string.filter_attractor_seed == 7919u);

    string.filter_attractor_seed = 0u;
    assert(apsis_controls_string_attractor_edit(&string, 1u, &input, &out));
    assert(string.filter_attractor_seed == 8236u);
}

static void test_reroll_regenerates_existing_runtime(void)
{
    ApsisState state = {0};
    ApsisStringAttractorRuntime runtime;
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};
    uint16_t before[APSIS_STRING_ATTRACTOR_CELLS_MAX];
    uint8_t i, different = 0u;

    state.string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    state.string.filter_attractor_length = 8u;
    state.string.filter_attractor_seed = 317u;
    state.string.filter_attractor_gates = 0x9535u;
    apsis_string_attractor_reset(&runtime, &state);
    memcpy(before, runtime.cell_q15, sizeof(before));

    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    assert(apsis_controls_string_attractor_edit(&state.string, 1u, &input, &out));
    assert(state.string.filter_attractor_seed == 8236u);
    apsis_string_attractor_sync(&runtime, &state);
    assert(runtime.seed == 8236u);
    assert(runtime.length == 8u);
    assert(runtime.pos == 0u);
    for (i = 0u; i < 8u; ++i)
        if (runtime.cell_q15[i] != before[i]) different = 1u;
    assert(different);

    runtime.pos = 3u;
    apsis_string_attractor_sync(&runtime, &state);
    assert(runtime.pos == 3u);
}

static void test_gate_edit_does_not_regenerate(void)
{
    ApsisState state = {0};
    ApsisStringAttractorRuntime runtime;
    ApsisStringAttractorControlResult out;
    ApsisFm1InputFrame input = {0};
    uint16_t before[APSIS_STRING_ATTRACTOR_CELLS_MAX];

    state.string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    state.string.filter_attractor_length = 8u;
    state.string.filter_attractor_seed = 317u;
    state.string.filter_attractor_gates = 0x9535u;
    apsis_string_attractor_reset(&runtime, &state);
    runtime.pos = 5u;
    memcpy(before, runtime.cell_q15, sizeof(before));

    input.notes_pressed = 1u << 2;
    assert(apsis_controls_string_attractor_edit(&state.string, 1u, &input, &out));
    assert(out.toggled_gates == (1u << 2));
    apsis_string_attractor_sync(&runtime, &state);
    assert(runtime.pos == 5u);
    assert(runtime.seed == 317u);
    assert(memcmp(runtime.cell_q15, before, sizeof(before)) == 0);
}

int main(void)
{
    test_context();
    test_inactive_noop();
    test_gate_bounds_and_hidden_bits();
    test_length_normalization();
    test_reroll_changes_only_seed();
    test_reroll_regenerates_existing_runtime();
    test_gate_edit_does_not_regenerate();
    puts("PASS: P10E String Attractor editor is bounded, isolated and M4c-backed");
    return 0;
}
