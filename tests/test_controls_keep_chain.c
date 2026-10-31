#include <stdio.h>
#include <string.h>
#include "apsis_controls_keep_chain.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t bit(uint8_t b) { return 1u << b; }
static int32_t absi(int32_t v) { return v < 0 ? -v : v; }

static void state_fixture(ApsisState *s, uint8_t degree)
{
    memset(s, 0, sizeof *s);
    s->string.left_split_q15 = APSIS_Q15_MILLI(470);
    s->string.right_split_q15 = APSIS_Q15_MILLI(530);
    s->string.surface_mode = APSIS_STRING_DUAL;
    s->clock.n = 12u;
    s->clock.main_mask = 0xFFFu;
    s->spiral.route_count = 2u;
    s->spiral.route[0] = 4u;
    s->spiral.route[1] = 9u;
    s->gravity.scale = APSIS_SCALE_DORIAN;
    s->gravity.degree = degree;
    s->gravity.voices = 4u;
    s->gravity.voicing = APSIS_GRAVITY_COLOUR;
    s->temper.mode = APSIS_TUNING_EQUAL;
}

static void fill_keep(ApsisKeepBank *bank, ApsisState *s, uint8_t slot, uint8_t degree)
{
    state_fixture(s, degree);
    bank->slots[slot] = apsis_keep_capture(s);
}

static int test_keep_source_semantics(void)
{
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisState s;
    ApsisFm1InputFrame in;
    uint8_t slot = 99u;

    apsis_keep_bank_reset(&bank);
    apsis_chain_sequence_reset(&chain);
    state_fixture(&s, 2u);
    memset(&in, 0, sizeof in);
    in.notes_pressed = (1u << 5) | (1u << 2);
    CHECK(apsis_controls_keep_slot_from_fm1(&in, &slot) == 1u && slot == 2u);
    CHECK(apsis_controls_keep_activate_slot(&bank, &s, 2u) == APSIS_KEEP_SLOT_SELECTED);
    CHECK(bank.selected_slot == 2);

    s.gravity.degree = 6u;
    CHECK(apsis_controls_keep_capture(&bank, &s) == 2);
    CHECK(bank.slots[2].valid && bank.slots[2].gravity_degree == 6u);
    s.gravity.degree = 1u;
    CHECK(apsis_controls_keep_activate_slot(&bank, &s, 2u) == APSIS_KEEP_SLOT_RECALLED);
    CHECK(s.gravity.degree == 6u && bank.selected_slot == 2);

    bank.selected_slot = -1;
    fill_keep(&bank, &s, 0u, 0u);
    fill_keep(&bank, &s, 1u, 1u);
    CHECK(apsis_controls_keep_capture(&bank, &s) == 3); /* first empty after 0/1/2 */

    memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(APSIS_FM1_BTN_SAVE);
    CHECK(apsis_controls_keep_panel_action(&in) == APSIS_KEEP_PANEL_CAPTURE);
    in.buttons_pressed |= bit(APSIS_FM1_BTN_REC);
    CHECK(apsis_controls_keep_panel_action(&in) == APSIS_KEEP_PANEL_ERASE);
    return 0;
}

static int test_keep_erase_prunes_chain(void)
{
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisState s;
    ApsisChainState settings;
    memset(&settings, 0, sizeof settings); settings.default_cycles = 4u;
    apsis_keep_bank_reset(&bank); apsis_chain_sequence_reset(&chain);
    fill_keep(&bank, &s, 1u, 1u); fill_keep(&bank, &s, 2u, 2u);
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 1u));
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 2u));
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 1u));
    bank.selected_slot = 1;
    CHECK(apsis_controls_keep_erase_selected(&bank, &chain) == 1u);
    CHECK(!bank.slots[1].valid && bank.selected_slot == -1);
    CHECK(chain.count == 1u && chain.cells[0].slot == 2u);
    return 0;
}

static int test_keep_erase_preserves_chain_identity(void)
{
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisState s;
    ApsisChainState settings;
    uint8_t slot;

    memset(&settings, 0, sizeof settings); settings.default_cycles = 4u;
    apsis_keep_bank_reset(&bank); apsis_chain_sequence_reset(&chain);
    for (slot = 0u; slot < 4u; ++slot) fill_keep(&bank, &s, slot, slot);
    for (slot = 0u; slot < 4u; ++slot)
        CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, slot));

    chain.selected = 3;
    chain.pos = 2u;
    chain.pulse = 9u;
    bank.selected_slot = 1;
    CHECK(apsis_controls_keep_erase_selected(&bank, &chain) == 1u);
    CHECK(chain.count == 3u);
    CHECK(chain.cells[0].slot == 0u && chain.cells[1].slot == 2u && chain.cells[2].slot == 3u);
    CHECK(chain.selected == 2); /* same surviving slot-3 object */
    CHECK(chain.pos == 1u && chain.cells[chain.pos].slot == 2u);
    CHECK(chain.pulse == 9u); /* active cell survived */

    chain.playing = 1u;
    chain.pulse = 7u;
    bank.selected_slot = 2;
    CHECK(apsis_controls_keep_erase_selected(&bank, &chain) == 1u);
    CHECK(chain.count == 2u && chain.cells[0].slot == 0u && chain.cells[1].slot == 3u);
    CHECK(chain.selected == 1);
    CHECK(chain.pos == 1u && chain.pulse == 0u); /* active cell was removed */
    return 0;
}

static int test_chain_append_cursor_and_params(void)
{
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisChainState settings;
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisChainCell *c;
    uint16_t chance_native;

    apsis_keep_bank_reset(&bank); apsis_chain_sequence_reset(&chain);
    memset(&settings, 0, sizeof settings);
    settings.default_cycles = 4u;
    settings.default_morph_quarters = APSIS_CHAIN_MORPH_NONE;
    fill_keep(&bank, &s, 0u, 0u); fill_keep(&bank, &s, 3u, 3u); fill_keep(&bank, &s, 7u, 7u);
    CHECK(!apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 2u));
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 0u));
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 3u));
    CHECK(chain.count == 2u && chain.selected == 1);
    CHECK(apsis_controls_chain_cursor_step(&chain, 1) == 0);
    CHECK(apsis_controls_chain_cursor_step(&chain, -1) == 1);

    c = &chain.cells[1];
    c->cycles = 1u; c->fine_pulses = 0u; c->chance_q15 = APSIS_Q15_ONE; c->transpose = 0; c->morph_quarters = 0u;
    memset(&in, 0, sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_PRESETS] = 1; /* slot3 -> slot7 */
    in.encoder_steps[APSIS_FM1_ENC_K1] = -1;
    in.encoder_steps[APSIS_FM1_ENC_K2] = -5;
    in.encoder_steps[APSIS_FM1_ENC_K3] = -2;
    in.encoder_steps[APSIS_FM1_ENC_K4] = 13;
    in.encoder_steps[APSIS_FM1_ENC_ALGORITHM] = 2;
    CHECK(apsis_controls_chain_apply_encoders(&chain, &bank, &settings, 12u, &in) == 6u);
    CHECK(c->slot == 7u);
    CHECK(c->cycles == 0u && c->fine_pulses == 1u); /* source zero/zero => fine=1 */
    chance_native = (uint16_t)(APSIS_Q15_ONE - 2u * APSIS_Q15_MILLI(50));
    CHECK(c->chance_q15 == chance_native);
    CHECK(absi((int32_t)c->chance_q15 - (int32_t)APSIS_Q15_MILLI(900)) <= 1);
    CHECK(c->transpose == 12 && c->morph_quarters == APSIS_CHAIN_MORPH_HALF);
    return 0;
}

static int test_chain_duplicate_delete_transport(void)
{
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisChainState settings;
    ApsisState s;
    ApsisFm1InputFrame in;
    ApsisChainDeleteResult r;

    apsis_keep_bank_reset(&bank); apsis_chain_sequence_reset(&chain);
    memset(&settings, 0, sizeof settings); settings.default_cycles = 4u;
    fill_keep(&bank, &s, 0u, 0u); fill_keep(&bank, &s, 1u, 1u);
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 0u));
    CHECK(apsis_controls_chain_append_slot(&chain, &bank, &settings, 12u, 1u));
    chain.selected = 0;
    CHECK(apsis_controls_chain_duplicate_selected(&chain, &settings, 12u));
    CHECK(chain.count == 3u && chain.selected == 1 && chain.cells[0].slot == 0u && chain.cells[1].slot == 0u && chain.cells[2].slot == 1u);

    chain.playing = 1u; chain.pos = 1u; chain.pulse = 7u; chain.selected = 1;
    r = apsis_controls_chain_delete_selected(&chain);
    CHECK(r.changed && r.active_removed && r.apply_current);
    CHECK(chain.count == 2u && chain.pos == 1u && chain.pulse == 0u);

    memset(&in, 0, sizeof in); in.buttons_pressed = bit(APSIS_FM1_BTN_REC);
    CHECK(apsis_controls_chain_context_action(&in) == APSIS_CHAIN_CONTEXT_DUPLICATE);
    in.buttons_held = bit(APSIS_FM1_BTN_EDIT);
    CHECK(apsis_controls_chain_context_action(&in) == APSIS_CHAIN_CONTEXT_DELETE);

    memset(&in, 0, sizeof in); in.buttons_pressed = bit(APSIS_FM1_BTN_PLAY); in.buttons_held = bit(APSIS_FM1_BTN_SEL);
    chain.playing = 0u;
    CHECK(apsis_controls_chain_transport_intent(&chain, &in) == APSIS_CHAIN_TRANSPORT_START);
    chain.playing = 1u;
    CHECK(apsis_controls_chain_transport_intent(&chain, &in) == APSIS_CHAIN_TRANSPORT_STOP);
    in.buttons_held = 0u;
    CHECK(apsis_controls_chain_transport_intent(&chain, &in) == APSIS_CHAIN_TRANSPORT_NONE);
    return 0;
}

int main(void)
{
    CHECK(test_keep_source_semantics() == 0);
    CHECK(test_keep_erase_prunes_chain() == 0);
    CHECK(test_keep_erase_preserves_chain_identity() == 0);
    CHECK(test_chain_append_cursor_and_params() == 0);
    CHECK(test_chain_duplicate_delete_transport() == 0);
    puts("PASS: APSIS P9F source-locked KEEP/CHAIN physical-control contract");
    return 0;
}
