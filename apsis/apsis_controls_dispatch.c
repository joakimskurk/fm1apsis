#include "apsis_controls_dispatch.h"
#include "apsis_bloom.h"
#include "apsis_string.h"

static uint32_t p9h2_button_bit(uint8_t button)
{
    return 1u << button;
}

static void p9h2_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static void p9h2_changed(ApsisControlsDispatchResult *result)
{
    if (result) result->changed = 1u;
}

static uint8_t p9h2_bloom_voices(const ApsisState *state)
{
    ApsisBloomHarmony harmony;
    uint8_t voices;
    if (!state) return 0u;
    apsis_bloom_harmony_from_state(state, &harmony);
    voices = apsis_bloom_voice_count(state, &harmony);
    return voices > APSIS_BLOOM_VOICES_MAX ? APSIS_BLOOM_VOICES_MAX : voices;
}

void apsis_controls_session_reset(ApsisControlsSession *session)
{
    if (!session) return;
    p9h2_zero(session, (uint32_t)sizeof(*session));
    session->clock_layer = APSIS_CLOCK_CONTROL_MAIN;
    apsis_controls_space_reset(&session->space);
    apsis_controls_bloom_grow_reset(&session->bloom_grow);
}

static void p9h2_dispatch_mother(ApsisState *state,
                                 ApsisMotherPending *pending,
                                 const ApsisFm1InputFrame *input,
                                 int32_t last_abs_step,
                                 ApsisControlsDispatchResult *result)
{
    ApsisMotherMixEdit edits[APSIS_MOTHER_SOURCE_COUNT];
    uint8_t source, i, count;

    count = apsis_controls_mother_mix_edits(&state->mother, input, edits);
    for (i = 0u; i < count; ++i)
        if (apsis_controls_mother_apply_mix(&state->mother, &edits[i]))
            p9h2_changed(result);

    if (apsis_controls_mother_mute_source_from_fm1(input, &source)) {
        if (apsis_controls_mother_toggle_source_mute(state, source))
            p9h2_changed(result);
        /* Source buttons are also global navigation. On MOTHER the contextual
         * SEL+source mute gesture owns the frame, including SEL+FX. */
        result->suppress_navigation = 1u;
    }
    if (apsis_controls_mother_sync_from_fm1(&state->mother, pending, input))
        p9h2_changed(result);
    if (apsis_controls_mother_horizon_from_fm1(&state->mother, pending, input,
                                               last_abs_step))
        p9h2_changed(result);
}

static void p9h2_dispatch_clock(ApsisControlsSession *session,
                                ApsisState *state,
                                const ApsisFm1InputFrame *input,
                                ApsisControlsDispatchResult *result)
{
    uint8_t layer = apsis_controls_clock_layer_step(
        session->clock_layer, input->encoder_steps[APSIS_FM1_ENC_ALGORITHM]);
    if (layer != session->clock_layer) {
        session->clock_layer = layer;
        p9h2_changed(result);
    }
    if (apsis_controls_clock_apply_keys(&state->clock, session->clock_layer,
                                        input->notes_pressed))
        p9h2_changed(result);
}

static void p9h2_dispatch_gravity(ApsisState *state,
                                  const ApsisFm1InputFrame *input,
                                  ApsisControlsDispatchResult *result)
{
    uint8_t degree, ring;
    ring = apsis_controls_gravity_ring_from_fm1(input,
                                                state->gravity.selected_ring);
    if (ring != state->gravity.selected_ring) {
        state->gravity.selected_ring = ring;
        p9h2_changed(result);
    }
    if (apsis_controls_gravity_degree_from_fm1(input, &degree)) {
        result->gravity_request = 1u;
        result->gravity_degree = degree;
        result->gravity_ring = state->gravity.selected_ring;
        p9h2_changed(result);
    }
}

static void p9h2_dispatch_string(ApsisControlsSession *session,
                                 ApsisState *state,
                                 ApsisMotherPending *pending,
                                 const ApsisFm1InputFrame *input,
                                 int32_t last_abs_step,
                                 ApsisControlsDispatchResult *result)
{
    ApsisControlParamEdit edits[4];
    uint8_t bank, mode, side, key, i, count;
    uint16_t split, current;
    int32_t d;

    bank = apsis_controls_string_bank_from_fm1(input,
                                               session->string_bank_base);
    if (bank != session->string_bank_base) {
        session->string_bank_base = bank;
        p9h2_changed(result);
    }

    mode = apsis_controls_string_mode_step(
        state->string.surface_mode,
        input->encoder_steps[APSIS_FM1_ENC_ALGORITHM]);
    if (mode != state->string.surface_mode) {
        state->string.surface_mode = mode;
        p9h2_changed(result);
    }

    side = apsis_controls_string_side_from_fm1(&state->string, input);
    if (apsis_controls_string_key_from_fm1(input, &key)) {
        split = apsis_controls_string_split_for_key(
            state, side, key, session->string_bank_base);
        apsis_string_set_side_split(&state->string, side, split, 1u);
        result->string_pluck = 1u;
        result->string_side = side;
        result->string_split_q15 = split;
        p9h2_changed(result);
    }

    d = input->encoder_steps[APSIS_FM1_ENC_SELECT];
    if (d) {
        current = side == APSIS_SIDE_LEFT ? state->string.left_split_q15
                                         : state->string.right_split_q15;
        split = apsis_controls_string_split_nudge(side, current, d);
        if (split != current) {
            apsis_string_set_side_split(&state->string, side, split, 1u);
            p9h2_changed(result);
        }
        if (result->string_pluck) result->string_split_q15 = split;
    }

    count = apsis_controls_string_param_edits(&state->string, input, edits);
    for (i = 0u; i < count; ++i)
        if (apsis_controls_apply_string_edit(state, pending, &edits[i],
                                             last_abs_step))
            p9h2_changed(result);
}

static void p9h2_dispatch_drift(ApsisState *state,
                                ApsisMotherPending *pending,
                                const ApsisFm1InputFrame *input,
                                int32_t last_abs_step,
                                ApsisControlsDispatchResult *result)
{
    ApsisDriftParamEdit edits[3];
    uint8_t i, count;
    if (apsis_controls_drift_apply_live(state, input))
        p9h2_changed(result);
    count = apsis_controls_drift_param_edits(state, input, edits);
    for (i = 0u; i < count; ++i)
        if (apsis_controls_apply_drift_edit(state, pending, &edits[i],
                                            last_abs_step))
            p9h2_changed(result);
}

static void p9h2_dispatch_spiral(ApsisControlsSession *session,
                                 ApsisState *state,
                                 const ApsisFm1InputFrame *input,
                                 ApsisControlsDispatchResult *result)
{
    uint8_t bank, point;
    int32_t d;

    bank = apsis_controls_spiral_bank_from_fm1(input, session->spiral_bank);
    if (bank != session->spiral_bank) {
        session->spiral_bank = bank;
        p9h2_changed(result);
    }

    d = input->encoder_steps[APSIS_FM1_ENC_SELECT];
    if (d && state->spiral.route_count) {
        uint8_t cursor = apsis_controls_spiral_cursor_step(
            session->spiral_cursor, state->spiral.route_count, d);
        if (cursor != session->spiral_cursor) {
            session->spiral_cursor = cursor;
            p9h2_changed(result);
        }
    }

    if (apsis_controls_spiral_point_from_fm1(input, session->spiral_bank,
                                             &point)) {
        uint8_t changed;
        if (input->buttons_held & p9h2_button_bit(APSIS_FM1_BTN_SEL))
            changed = apsis_controls_spiral_insert(&state->spiral, point,
                                                   &session->spiral_cursor);
        else
            changed = apsis_controls_spiral_append(&state->spiral, point,
                                                   &session->spiral_cursor);
        if (changed) p9h2_changed(result);
    }

    d = input->encoder_steps[APSIS_FM1_ENC_K1];
    if (d && apsis_controls_spiral_move_selected(
                 &state->spiral, session->spiral_cursor, d))
        p9h2_changed(result);

    if ((input->buttons_pressed & p9h2_button_bit(APSIS_FM1_BTN_REC)) &&
        apsis_controls_spiral_delete_selected(&state->spiral,
                                              &session->spiral_cursor))
        p9h2_changed(result);
}

static void p9h2_dispatch_bloom(ApsisControlsSession *session,
                                ApsisState *state,
                                ApsisMotherPending *pending,
                                const ApsisFm1InputFrame *input,
                                int32_t last_abs_step,
                                ApsisControlsDispatchResult *result)
{
    ApsisBloomControlFrame frame;
    ApsisBloomParamEdit edits[4];
    uint8_t i, count;

    if (apsis_controls_bloom_frame(state, input, &frame)) {
        if (frame.grow_begin) {
            apsis_controls_bloom_grow_reset(&session->bloom_grow);
            p9h2_changed(result);
        }
        if (frame.grow_detents) {
            uint16_t old_open = session->bloom_grow.open_q15;
            uint8_t old_mask = session->bloom_grow.fired_mask;
            uint8_t newly = apsis_controls_bloom_grow_update(
                &session->bloom_grow, p9h2_bloom_voices(state),
                frame.grow_detents);
            result->bloom_petal_mask |= newly;
            if (old_open != session->bloom_grow.open_q15 ||
                old_mask != session->bloom_grow.fired_mask)
                p9h2_changed(result);
        }
        if (frame.trigger == APSIS_BLOOM_CONTROL_TRIGGER_CORE) {
            result->bloom_core = 1u;
            p9h2_changed(result);
        } else if (frame.trigger == APSIS_BLOOM_CONTROL_TRIGGER_PETAL &&
                   frame.petal_index < APSIS_BLOOM_VOICES_MAX) {
            result->bloom_petal_mask |= (uint8_t)(1u << frame.petal_index);
            p9h2_changed(result);
        }
        if (frame.grow_end) {
            apsis_controls_bloom_grow_reset(&session->bloom_grow);
            p9h2_changed(result);
        }
    }

    count = apsis_controls_bloom_param_edits(&state->bloom, input, edits);
    for (i = 0u; i < count; ++i)
        if (apsis_controls_apply_bloom_edit(state, pending, &edits[i],
                                            last_abs_step))
            p9h2_changed(result);
}

static void p9h2_dispatch_keep(ApsisState *state,
                               ApsisKeepBank *bank,
                               ApsisChainSequence *chain,
                               const ApsisFm1InputFrame *input,
                               ApsisControlsDispatchResult *result)
{
    ApsisKeepPanelAction action;
    uint8_t slot;

    if (!bank) return;
    if (apsis_controls_keep_slot_from_fm1(input, &slot)) {
        if (apsis_controls_keep_activate_slot(bank, state, slot) !=
            APSIS_KEEP_SLOT_NONE) {
            result->project_dirty = 1u; /* selected slot is persisted */
            p9h2_changed(result);
        }
    }

    action = apsis_controls_keep_panel_action(input);
    if (input->buttons_pressed & p9h2_button_bit(APSIS_FM1_BTN_SAVE))
        result->suppress_navigation = 1u;
    if (action == APSIS_KEEP_PANEL_CAPTURE) {
        if (apsis_controls_keep_capture(bank, state) >= 0) {
            result->project_dirty = 1u;
            p9h2_changed(result);
        }
    } else if (action == APSIS_KEEP_PANEL_ERASE) {
        if (apsis_controls_keep_erase_selected(bank, chain)) {
            result->project_dirty = 1u;
            p9h2_changed(result);
        }
    }
}

static void p9h2_dispatch_chain(ApsisState *state,
                                ApsisKeepBank *bank,
                                ApsisChainSequence *chain,
                                const ApsisFm1InputFrame *input,
                                ApsisControlsDispatchResult *result)
{
    ApsisChainContextAction context;
    ApsisChainDeleteResult deleted;
    int8_t before_selected;
    uint8_t slot;

    if (!bank || !chain) return;

    if (apsis_controls_keep_slot_from_fm1(input, &slot) &&
        apsis_controls_chain_append_slot(chain, bank, &state->chain,
                                         state->clock.n, slot)) {
        result->project_dirty = 1u;
        p9h2_changed(result);
    }

    if (input->encoder_steps[APSIS_FM1_ENC_SELECT]) {
        before_selected = chain->selected;
        (void)apsis_controls_chain_cursor_step(
            chain, input->encoder_steps[APSIS_FM1_ENC_SELECT]);
        if (chain->selected != before_selected) {
            result->project_dirty = 1u; /* editor-selected cell is persisted */
            p9h2_changed(result);
        }
    }

    if (apsis_controls_chain_apply_encoders(chain, bank, &state->chain,
                                            state->clock.n, input)) {
        result->project_dirty = 1u;
        p9h2_changed(result);
    }

    context = apsis_controls_chain_context_action(input);
    if (context == APSIS_CHAIN_CONTEXT_DUPLICATE) {
        if (apsis_controls_chain_duplicate_selected(chain, &state->chain,
                                                    state->clock.n)) {
            result->project_dirty = 1u;
            p9h2_changed(result);
        }
    } else if (context == APSIS_CHAIN_CONTEXT_DELETE) {
        deleted = apsis_controls_chain_delete_selected(chain);
        if (deleted.changed) {
            result->project_dirty = 1u;
            result->chain_apply_current = deleted.apply_current;
            p9h2_changed(result);
        }
    }

    result->chain_transport = (uint8_t)apsis_controls_chain_transport_intent(
        chain, input);
    if (result->chain_transport != APSIS_CHAIN_TRANSPORT_NONE)
        p9h2_changed(result);
}

static void p9h2_dispatch_space(ApsisControlsSession *session,
                                ApsisState *state,
                                ApsisMotherPending *pending,
                                const ApsisFm1InputFrame *input,
                                int32_t last_abs_step,
                                ApsisControlsDispatchResult *result)
{
    ApsisSpaceControlEdit edits[4];
    uint8_t i, count;
    if (apsis_controls_space_page_step(
            &session->space,
            input->encoder_steps[APSIS_FM1_ENC_ALGORITHM]))
        p9h2_changed(result);
    if (apsis_controls_space_source_step(
            &session->space,
            input->encoder_steps[APSIS_FM1_ENC_SELECT]))
        p9h2_changed(result);
    count = apsis_controls_space_edits(state, &session->space, input, edits);
    for (i = 0u; i < count; ++i)
        if (apsis_controls_apply_space_edit(state, pending, &edits[i],
                                            last_abs_step))
            p9h2_changed(result);
}

void apsis_controls_dispatch(ApsisControlsSession *session,
                             ApsisState *state,
                             ApsisKeepBank *keep_bank,
                             ApsisChainSequence *chain,
                             ApsisMotherPending *mother_pending,
                             uint8_t surface,
                             const ApsisFm1InputFrame *input,
                             int32_t last_abs_step,
                             ApsisControlsDispatchResult *result)
{
    if (result) p9h2_zero(result, (uint32_t)sizeof(*result));
    if (!session || !state || !input || !result || surface >= APSIS_SURFACE_COUNT)
        return;

    switch (surface) {
    case APSIS_SURFACE_MOTHER:
        p9h2_dispatch_mother(state, mother_pending, input, last_abs_step, result);
        break;
    case APSIS_SURFACE_STRING:
        p9h2_dispatch_string(session, state, mother_pending, input,
                            last_abs_step, result);
        break;
    case APSIS_SURFACE_DRIFT:
        p9h2_dispatch_drift(state, mother_pending, input,
                           last_abs_step, result);
        break;
    case APSIS_SURFACE_CLOCK:
        p9h2_dispatch_clock(session, state, input, result);
        break;
    case APSIS_SURFACE_SPIRAL:
        p9h2_dispatch_spiral(session, state, input, result);
        break;
    case APSIS_SURFACE_BLOOM:
        p9h2_dispatch_bloom(session, state, mother_pending, input,
                           last_abs_step, result);
        break;
    case APSIS_SURFACE_GRAVITY:
        p9h2_dispatch_gravity(state, input, result);
        break;
    case APSIS_SURFACE_CHAIN:
        p9h2_dispatch_chain(state, keep_bank, chain, input, result);
        break;
    case APSIS_SURFACE_KEEP:
        p9h2_dispatch_keep(state, keep_bank, chain, input, result);
        break;
    case APSIS_SURFACE_SPACE:
        p9h2_dispatch_space(session, state, mother_pending, input,
                           last_abs_step, result);
        break;
    default:
        break;
    }
}
