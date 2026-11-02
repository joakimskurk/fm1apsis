#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_controls_spiral_attractor.h"

static uint32_t button_bit(uint8_t button)
{
    return 1u << button;
}

static ApsisSpiralState base_spiral(void)
{
    ApsisSpiralState s;
    uint8_t i;
    memset(&s, 0, sizeof(s));
    s.route_count = 3u;
    s.route[0] = 2u;
    s.route[1] = 7u;
    s.route[2] = 11u;
    s.rate = 1u;
    s.note_length_q15 = APSIS_Q15_MILLI(820);
    s.attractor_rate = 2u;
    s.attractor_length = 16u;
    s.attractor_lock_q15 = APSIS_Q15_MILLI(860);
    s.attractor_density_q15 = APSIS_Q15_MILLI(580);
    s.attractor_seed = 911u;
    s.attractor_gates = 0x002D2D2Du;
    for (i = 0u; i < APSIS_ATTRACTOR_STEPS; ++i)
        s.attractor_energy_q15[i] = APSIS_Q15_MILLI(500);
    return s;
}

static void test_context(void)
{
    ApsisSpiralAttractorEditorSession session;
    ApsisSpiralAttractorContextResult out;
    ApsisFm1InputFrame input = {0};

    apsis_controls_spiral_attractor_editor_reset(&session);
    assert(session.active == 0u && session.selected_cell == 0u);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    assert(!apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_MOTHER, &input, &out));
    assert(session.active == 0u);

    assert(apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_SPIRAL, &input, &out));
    assert(session.active == 1u && out.entered == 1u && out.exited == 0u);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 2;
    assert(!apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_SPIRAL, &input, &out));
    assert(session.active == 1u);

    session.selected_cell = 7u;
    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = -1;
    assert(apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_SPIRAL, &input, &out));
    assert(session.active == 0u && session.selected_cell == 0u && out.exited == 1u);

    input.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 1;
    assert(apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_SPIRAL, &input, &out));
    assert(session.active == 1u);
    memset(&input, 0, sizeof(input));
    assert(apsis_controls_spiral_attractor_context(
        &session, APSIS_SURFACE_STRING, &input, &out));
    assert(session.active == 0u && out.exited == 1u);
}

static void test_inactive_noop(void)
{
    ApsisSpiralAttractorEditorSession session = {0};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisSpiralState before = spiral;
    ApsisFm1InputFrame input = {0};

    input.notes_pressed = 0x00ffffffu;
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    input.encoder_steps[APSIS_FM1_ENC_K1] = 4;
    input.encoder_steps[APSIS_FM1_ENC_K2] = 20;
    input.encoder_steps[APSIS_FM1_ENC_K3] = -20;
    input.encoder_steps[APSIS_FM1_ENC_K4] = 10;

    assert(!apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(memcmp(&spiral, &before, sizeof(spiral)) == 0);
    assert(out.handled == 0u && out.suppress_route_dispatch == 0u);
}

static void test_gate_bounds_selection_and_hidden_bits(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 0u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisFm1InputFrame input = {0};
    uint32_t before;

    spiral.attractor_length = 8u;
    spiral.attractor_gates = 0x00a5f00fu;
    before = spiral.attractor_gates;
    input.notes_pressed = (1u << 0) | (1u << 3) | (1u << 7) |
                          (1u << 8) | (1u << 23);

    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(out.handled == 1u && out.suppress_route_dispatch == 1u);
    assert(out.toggled_gates == 0x00000089u);
    assert(spiral.attractor_gates == (before ^ 0x89u));
    assert((spiral.attractor_gates & 0xffffff00u) == (before & 0xffffff00u));
    assert(session.selected_cell == 0u && out.selected_cell == 0u);

    memset(&input, 0, sizeof(input));
    input.notes_pressed = (1u << 6) | (1u << 7);
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(out.toggled_gates == 0xc0u);
    assert(session.selected_cell == 6u);
}

static void test_parameters_and_bounds(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 3u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisFm1InputFrame input = {0};
    uint16_t energy_before;

    input.encoder_steps[APSIS_FM1_ENC_K1] = 1;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_rate == 4u && out.rate == 4u);

    memset(&input, 0, sizeof(input));
    input.encoder_steps[APSIS_FM1_ENC_K1] = -2;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_rate == 1u);

    memset(&input, 0, sizeof(input));
    input.encoder_steps[APSIS_FM1_ENC_K2] = 32767;
    input.encoder_steps[APSIS_FM1_ENC_K3] = -32768;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_lock_q15 == APSIS_Q15_ONE);
    assert(spiral.attractor_density_q15 == APSIS_Q15_MILLI(150));

    memset(&input, 0, sizeof(input));
    input.encoder_steps[APSIS_FM1_ENC_K2] = -32768;
    input.encoder_steps[APSIS_FM1_ENC_K3] = 32767;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_lock_q15 == 0u);
    assert(spiral.attractor_density_q15 == APSIS_Q15_MILLI(900));

    spiral.attractor_energy_q15[3] = APSIS_Q15_MILLI(500);
    energy_before = spiral.attractor_energy_q15[3];
    memset(&input, 0, sizeof(input));
    input.encoder_steps[APSIS_FM1_ENC_K4] = 5;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_energy_q15[3] ==
           (uint16_t)(energy_before + 5u * APSIS_Q15_MILLI(10)));

    memset(&input, 0, sizeof(input));
    input.encoder_steps[APSIS_FM1_ENC_K4] = 32767;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_energy_q15[3] == APSIS_Q15_ONE);
    input.encoder_steps[APSIS_FM1_ENC_K4] = -32768;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_energy_q15[3] == APSIS_Q15_MILLI(120));
}

static void test_gate_only_preserves_seed_energy_route(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 0u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisFm1InputFrame input = {0};
    uint16_t seed = spiral.attractor_seed;
    uint16_t energy[APSIS_ATTRACTOR_STEPS];
    uint8_t route[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t route_count = spiral.route_count;

    memcpy(energy, spiral.attractor_energy_q15, sizeof(energy));
    memcpy(route, spiral.route, sizeof(route));
    input.notes_pressed = 1u << 5;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(spiral.attractor_seed == seed);
    assert(memcmp(spiral.attractor_energy_q15, energy, sizeof(energy)) == 0);
    assert(spiral.route_count == route_count);
    assert(memcmp(spiral.route, route, sizeof(route)) == 0);
}

static void test_reroll_uses_frozen_authority_and_preserves_route(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 0u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisFm1InputFrame input = {0};
    uint8_t route[APSIS_SPIRAL_ROUTE_MAX];
    uint8_t route_count = spiral.route_count;

    spiral.attractor_length = 24u;
    memcpy(route, spiral.route, sizeof(route));
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(out.rerolled == 1u && out.state_changed == 1u);
    assert(spiral.attractor_seed == 4825u);
    assert((spiral.attractor_gates & 0x00ffffffu) == 0x00fda5fdu);
    assert(spiral.route_count == route_count);
    assert(memcmp(spiral.route, route, sizeof(route)) == 0);

    assert(spiral.attractor_energy_q15[0] >= 18574u &&
           spiral.attractor_energy_q15[0] <= 18576u);
    assert(spiral.attractor_energy_q15[1] >= 7305u &&
           spiral.attractor_energy_q15[1] <= 7307u);
}

static void test_default_length_reroll_tail_vector(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 0u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisFm1InputFrame input = {0};

    /* First 16 cells are the same deterministic prefix as the locked 24-cell
     * FDA5FD vector. Source reset then restores cells 16..23 through
     * set_default_tail(), whose default high byte is 0x2D. */
    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(out.rerolled == 1u);
    assert(spiral.attractor_length == 16u);
    assert(spiral.attractor_seed == 4825u);
    assert(spiral.attractor_gates == 0x002da5fdu);
}

static void test_same_frame_ordering_and_selected_energy(void)
{
    ApsisSpiralAttractorEditorSession session = {1u, 9u};
    ApsisSpiralAttractorControlResult out;
    ApsisSpiralState spiral = base_spiral();
    ApsisSpiralState reference = spiral;
    ApsisFm1InputFrame input = {0};
    uint16_t expected_energy;

    spiral.attractor_length = 24u;
    reference.attractor_length = 24u;
    apsis_spiral_reset_attractor(&reference, 1u);
    expected_energy = (uint16_t)(reference.attractor_energy_q15[3] +
        APSIS_Q15_MILLI(10));
    if (expected_energy > APSIS_Q15_ONE) expected_energy = APSIS_Q15_ONE;

    input.buttons_pressed = button_bit(APSIS_FM1_BTN_REC);
    input.notes_pressed = 1u << 3;
    input.encoder_steps[APSIS_FM1_ENC_K4] = 1;
    assert(apsis_controls_spiral_attractor_edit(&session, &spiral, &input, &out));
    assert(out.rerolled == 1u);
    assert(out.toggled_gates == (1u << 3));
    assert(spiral.attractor_gates == (reference.attractor_gates ^ (1u << 3)));
    assert(session.selected_cell == 3u && out.selected_cell == 3u);
    assert(spiral.attractor_energy_q15[3] == expected_energy);
    assert(out.energy_q15 == expected_energy);
}

int main(void)
{
    test_context();
    test_inactive_noop();
    test_gate_bounds_selection_and_hidden_bits();
    test_parameters_and_bounds();
    test_gate_only_preserves_seed_energy_route();
    test_reroll_uses_frozen_authority_and_preserves_route();
    test_default_length_reroll_tail_vector();
    test_same_frame_ordering_and_selected_energy();
    puts("PASS: P10F Spiral Attractor editor is bounded, route-isolated and M4f-backed");
    return 0;
}
