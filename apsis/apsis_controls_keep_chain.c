#include "apsis_controls_keep_chain.h"

static uint32_t p9f_button_bit(uint8_t button)
{
    return 1u << button;
}

static void p9f_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint8_t p9f_clock_n(uint8_t n)
{
    return n < 3u ? 3u : (n > APSIS_CLOCK_STEPS_MAX ? APSIS_CLOCK_STEPS_MAX : n);
}

static uint16_t p9f_chance_step(uint16_t current, int32_t detents)
{
    int64_t v = (int64_t)(current > APSIS_Q15_ONE ? APSIS_Q15_ONE : current) +
                (int64_t)detents * APSIS_Q15_MILLI(50);
    if (v < 0) return 0u;
    if (v > APSIS_Q15_ONE) return APSIS_Q15_ONE;
    return (uint16_t)v;
}

static uint8_t p9f_morph_step(uint8_t current, int32_t detents)
{
    static const uint8_t values[4] = {
        APSIS_CHAIN_MORPH_NONE, APSIS_CHAIN_MORPH_QUARTER,
        APSIS_CHAIN_MORPH_HALF, APSIS_CHAIN_MORPH_FULL
    };
    int32_t index = 0, d;
    uint8_t i;
    for (i = 0u; i < 4u; ++i) if (values[i] == current) { index = i; break; }
    d = detents % 4;
    index = (index + d) % 4;
    if (index < 0) index += 4;
    return values[index];
}

static uint8_t p9f_next_valid_slot(const ApsisKeepBank *bank, uint8_t current,
                                   int32_t detents, uint8_t *slot_out)
{
    int32_t steps, dir, slot;
    uint8_t probe;
    if (!bank || !slot_out || !detents) return 0u;
    dir = detents > 0 ? 1 : -1;
    steps = detents > 0 ? detents : -detents;
    slot = current < APSIS_KEEP_SLOTS ? current : (dir > 0 ? APSIS_KEEP_SLOTS - 1 : 0);
    while (steps-- > 0) {
        uint8_t found = 0u;
        for (probe = 0u; probe < APSIS_KEEP_SLOTS; ++probe) {
            slot = (slot + dir + APSIS_KEEP_SLOTS) % APSIS_KEEP_SLOTS;
            if (bank->slots[(uint8_t)slot].valid) { found = 1u; break; }
        }
        if (!found) return 0u;
    }
    *slot_out = (uint8_t)slot;
    return 1u;
}

uint8_t apsis_controls_keep_slot_from_fm1(const ApsisFm1InputFrame *input,
                                          uint8_t *slot_out)
{
    uint8_t slot;
    if (!input || !slot_out) return 0u;
    for (slot = 0u; slot < APSIS_KEEP_DIRECT_KEYS; ++slot) {
        if (input->notes_pressed & (1u << slot)) {
            *slot_out = slot;
            return 1u;
        }
    }
    return 0u;
}

ApsisKeepPanelAction apsis_controls_keep_panel_action(const ApsisFm1InputFrame *input)
{
    if (!input) return APSIS_KEEP_PANEL_NONE;
    if (input->buttons_pressed & p9f_button_bit(APSIS_FM1_BTN_REC))
        return APSIS_KEEP_PANEL_ERASE;
    if (input->buttons_pressed & p9f_button_bit(APSIS_FM1_BTN_SAVE))
        return APSIS_KEEP_PANEL_CAPTURE;
    return APSIS_KEEP_PANEL_NONE;
}

ApsisKeepSlotResult apsis_controls_keep_activate_slot(ApsisKeepBank *bank,
                                                      ApsisState *state,
                                                      uint8_t slot)
{
    if (!bank || !state || slot >= APSIS_KEEP_SLOTS) return APSIS_KEEP_SLOT_NONE;
    bank->selected_slot = (int8_t)slot;
    if (!bank->slots[slot].valid) return APSIS_KEEP_SLOT_SELECTED;
    return apsis_keep_apply(state, &bank->slots[slot], 0)
        ? APSIS_KEEP_SLOT_RECALLED : APSIS_KEEP_SLOT_SELECTED;
}

int8_t apsis_controls_keep_capture(ApsisKeepBank *bank, const ApsisState *state)
{
    return apsis_keep_capture_to_bank(bank, state);
}

uint8_t apsis_controls_keep_erase_selected(ApsisKeepBank *bank,
                                           ApsisChainSequence *chain)
{
    int8_t selected;
    uint8_t read, write = 0u, removed = 0u;
    uint8_t old_count, old_pos, old_selected = 0u;
    uint8_t removed_before_pos = 0u, removed_before_selected = 0u;
    uint8_t selected_valid, active_removed = 0u;

    if (!bank) return 0u;
    selected = bank->selected_slot;
    if (selected < 0 || selected >= (int8_t)APSIS_KEEP_SLOTS) return 0u;

    p9f_zero(&bank->slots[(uint8_t)selected], (uint32_t)sizeof(bank->slots[0]));
    bank->selected_slot = -1;
    if (!chain) return 1u;

    if (chain->count > APSIS_CHAIN_MAX) chain->count = APSIS_CHAIN_MAX;
    old_count = chain->count;
    old_pos = chain->pos < old_count ? chain->pos : 0u;
    selected_valid = chain->selected >= 0 && chain->selected < (int8_t)old_count;
    if (selected_valid) old_selected = (uint8_t)chain->selected;

    for (read = 0u; read < old_count; ++read) {
        if (chain->cells[read].slot == (uint8_t)selected) {
            if (read < old_pos) ++removed_before_pos;
            if (selected_valid && read < old_selected) ++removed_before_selected;
            if (read == old_pos) active_removed = 1u;
            ++removed;
            continue;
        }
        if (write != read) chain->cells[write] = chain->cells[read];
        ++write;
    }
    chain->count = write;
    while (write < old_count) {
        p9f_zero(&chain->cells[write], (uint32_t)sizeof(chain->cells[0]));
        ++write;
    }

    if (!removed) return 1u;
    if (!chain->count) {
        chain->selected = -1;
        chain->playing = 0u;
        chain->armed = 0u;
        chain->pos = 0u;
        chain->pulse = 0u;
        chain->phrase_pulse = 0u;
        chain->master_pulses = 0u;
        chain->hold_tail = 0u;
        chain->last_applied = 1u;
        return 1u;
    }

    if (selected_valid) {
        uint8_t next_selected = (uint8_t)(old_selected - removed_before_selected);
        if (next_selected >= chain->count) next_selected = (uint8_t)(chain->count - 1u);
        chain->selected = (int8_t)next_selected;
    } else {
        chain->selected = -1;
    }

    {
        uint8_t next_pos = (uint8_t)(old_pos - removed_before_pos);
        if (next_pos >= chain->count) next_pos = (uint8_t)(chain->count - 1u);
        chain->pos = next_pos;
    }
    if (active_removed) chain->pulse = 0u;
    return 1u;
}

uint8_t apsis_controls_chain_append_slot(ApsisChainSequence *chain,
                                         const ApsisKeepBank *bank,
                                         const ApsisChainState *settings,
                                         uint8_t clock_n,
                                         uint8_t slot)
{
    if (!chain || !bank || slot >= APSIS_KEEP_SLOTS || !bank->slots[slot].valid)
        return 0u;
    if (chain->count >= APSIS_CHAIN_MAX) return 0u;
    chain->cells[chain->count] = apsis_chain_default_cell(slot, settings, clock_n);
    chain->selected = (int8_t)chain->count;
    ++chain->count;
    return 1u;
}

int8_t apsis_controls_chain_cursor_step(ApsisChainSequence *chain, int32_t detents)
{
    int32_t current, count, delta;
    if (!chain || !chain->count) {
        if (chain) chain->selected = -1;
        return -1;
    }
    if (!detents) {
        if (chain->selected < 0 || chain->selected >= (int8_t)chain->count)
            chain->selected = 0;
        return chain->selected;
    }
    count = chain->count;
    current = (chain->selected >= 0 && chain->selected < (int8_t)chain->count)
        ? chain->selected : 0;
    delta = detents % count;
    current = (current + delta) % count;
    if (current < 0) current += count;
    chain->selected = (int8_t)current;
    return chain->selected;
}

uint8_t apsis_controls_chain_apply_encoders(ApsisChainSequence *chain,
                                            const ApsisKeepBank *bank,
                                            const ApsisChainState *settings,
                                            uint8_t clock_n,
                                            const ApsisFm1InputFrame *input)
{
    ApsisChainCell *cell;
    uint8_t changed = 0u, n, slot;
    int32_t d, v, max_fine;
    if (!chain || !input || chain->selected < 0 || chain->selected >= (int8_t)chain->count)
        return 0u;
    cell = &chain->cells[(uint8_t)chain->selected];
    n = p9f_clock_n(clock_n);
    max_fine = (int32_t)n - 1;

    d = input->encoder_steps[APSIS_FM1_ENC_PRESETS];
    if (d && p9f_next_valid_slot(bank, cell->slot, d, &slot) && slot != cell->slot) {
        cell->slot = slot;
        ++changed;
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d) {
        v = (int32_t)cell->cycles + d;
        cell->cycles = (uint8_t)(v < 0 ? 0 : (v > 32 ? 32 : v));
        if (!cell->cycles && !cell->fine_pulses) cell->fine_pulses = 1u;
        ++changed;
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K2];
    if (d) {
        v = (int32_t)cell->fine_pulses + d;
        cell->fine_pulses = (uint8_t)(v < 0 ? 0 : (v > max_fine ? max_fine : v));
        if (!cell->cycles && !cell->fine_pulses) cell->fine_pulses = 1u;
        ++changed;
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K3];
    if (d) {
        cell->chance_q15 = p9f_chance_step(cell->chance_q15, d);
        ++changed;
    }
    d = input->encoder_steps[APSIS_FM1_ENC_K4];
    if (d) {
        v = (int32_t)cell->transpose + d;
        cell->transpose = (int8_t)(v < -12 ? -12 : (v > 12 ? 12 : v));
        ++changed;
    }
    d = input->encoder_steps[APSIS_FM1_ENC_ALGORITHM];
    if (d) {
        cell->morph_quarters = p9f_morph_step(cell->morph_quarters, d);
        ++changed;
    }
    (void)settings;
    return changed;
}

uint8_t apsis_controls_chain_duplicate_selected(ApsisChainSequence *chain,
                                                const ApsisChainState *settings,
                                                uint8_t clock_n)
{
    uint8_t ix, insert, i;
    ApsisChainCell copy;
    if (!chain || chain->count >= APSIS_CHAIN_MAX || chain->selected < 0 ||
        chain->selected >= (int8_t)chain->count) return 0u;
    ix = (uint8_t)chain->selected;
    copy = apsis_chain_normalize_cell(&chain->cells[ix], settings, clock_n);
    insert = (uint8_t)(ix + 1u);
    for (i = chain->count; i > insert; --i) chain->cells[i] = chain->cells[i - 1u];
    chain->cells[insert] = copy;
    ++chain->count;
    chain->selected = (int8_t)insert;
    return 1u;
}

ApsisChainDeleteResult apsis_controls_chain_delete_selected(ApsisChainSequence *chain)
{
    ApsisChainDeleteResult r = {0u, 0u, 0u};
    uint8_t ix, i, old_pos;
    if (!chain || chain->selected < 0 || chain->selected >= (int8_t)chain->count)
        return r;
    ix = (uint8_t)chain->selected;
    old_pos = chain->pos < chain->count ? chain->pos : 0u;
    r.changed = 1u;
    r.active_removed = ix == old_pos ? 1u : 0u;

    for (i = (uint8_t)(ix + 1u); i < chain->count; ++i)
        chain->cells[i - 1u] = chain->cells[i];
    p9f_zero(&chain->cells[chain->count - 1u], (uint32_t)sizeof(chain->cells[0]));
    --chain->count;

    if (!chain->count) {
        chain->selected = -1;
        chain->playing = 0u;
        chain->armed = 0u;
        chain->pos = 0u;
        chain->pulse = 0u;
        chain->phrase_pulse = 0u;
        chain->master_pulses = 0u;
        chain->hold_tail = 0u;
        chain->last_applied = 1u;
        return r;
    }

    chain->selected = (int8_t)(ix < chain->count ? ix : (uint8_t)(chain->count - 1u));
    if (ix < old_pos) chain->pos = (uint8_t)(old_pos - 1u);
    else if (ix == old_pos) {
        chain->pos = ix < chain->count ? ix : (uint8_t)(chain->count - 1u);
        chain->pulse = 0u;
        r.apply_current = chain->playing ? 1u : 0u;
    } else chain->pos = old_pos < chain->count ? old_pos : (uint8_t)(chain->count - 1u);
    return r;
}

ApsisChainContextAction apsis_controls_chain_context_action(const ApsisFm1InputFrame *input)
{
    uint32_t rec, edit;
    if (!input) return APSIS_CHAIN_CONTEXT_NONE;
    rec = p9f_button_bit(APSIS_FM1_BTN_REC);
    edit = p9f_button_bit(APSIS_FM1_BTN_EDIT);
    if (!(input->buttons_pressed & rec)) return APSIS_CHAIN_CONTEXT_NONE;
    return (input->buttons_held & edit) ? APSIS_CHAIN_CONTEXT_DELETE
                                       : APSIS_CHAIN_CONTEXT_DUPLICATE;
}

ApsisChainTransportIntent apsis_controls_chain_transport_intent(const ApsisChainSequence *chain,
                                                                const ApsisFm1InputFrame *input)
{
    uint32_t play, sel;
    if (!chain || !input) return APSIS_CHAIN_TRANSPORT_NONE;
    play = p9f_button_bit(APSIS_FM1_BTN_PLAY);
    sel = p9f_button_bit(APSIS_FM1_BTN_SEL);
    if (!(input->buttons_pressed & play) || !(input->buttons_held & sel))
        return APSIS_CHAIN_TRANSPORT_NONE;
    if (chain->playing) return APSIS_CHAIN_TRANSPORT_STOP;
    return chain->count ? APSIS_CHAIN_TRANSPORT_START : APSIS_CHAIN_TRANSPORT_NONE;
}
