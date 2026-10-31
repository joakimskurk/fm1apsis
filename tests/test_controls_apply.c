#include <assert.h>
#include <stdio.h>
#include "apsis_controls_apply.h"
#include "apsis_state.h"

static void reset(ApsisState *state, ApsisMotherPending *pending)
{
    apsis_state_default(state);
    apsis_mother_pending_reset(pending);
}

static void commit_all(ApsisState *state, ApsisMotherPending *pending)
{
    uint8_t i;
    apsis_mother_begin_commit(pending);
    for (i = 0u; i < pending->count; ++i)
        assert(apsis_controls_apply_pending_item(state, &pending->items[i], 0) == 1u);
    apsis_mother_finish_commit(&state->mother, pending);
}

static void test_live_string_apply(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisControlParamEdit edit;
    reset(&state, &pending);

    edit.param = APSIS_CONTROL_PARAM_STRING_RESONANCE;
    edit.value = APSIS_Q15_MILLI(770);
    assert(apsis_controls_apply_string_edit(&state, &pending, &edit, 4) == 1u);
    assert(state.string.resonance_q15 == APSIS_Q15_MILLI(770));
    assert(apsis_mother_pending_count(&pending) == 0u);

    assert(apsis_controls_apply_structural_param(
        &state, &pending, 0xFFFFu, 123, 4) == 0u);
    assert(apsis_mother_pending_count(&pending) == 0u);
}

static void test_hold_coalesce_and_commit(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisControlParamEdit edit;
    uint16_t original;
    const ApsisMotherPendingItem *item;
    reset(&state, &pending);
    original = state.string.brightness_q15;
    apsis_mother_set_sync(&state.mother, &pending, 1u);

    edit.param = APSIS_CONTROL_PARAM_STRING_BRIGHTNESS;
    edit.value = APSIS_Q15_MILLI(610);
    assert(apsis_controls_apply_string_edit(&state, &pending, &edit, 11) == 1u);
    assert(state.string.brightness_q15 == original);
    assert(apsis_mother_pending_count(&pending) == 1u);
    assert(pending.pending_start_abs == 11);

    edit.value = APSIS_Q15_MILLI(620);
    assert(apsis_controls_apply_string_edit(&state, &pending, &edit, 12) == 1u);
    assert(apsis_mother_pending_count(&pending) == 1u);
    assert(pending.pending_start_abs == 11);
    item = apsis_mother_pending_at(&pending, 0u);
    assert(item != 0);
    assert(item->kind == APSIS_MOTHER_PENDING_PARAM);
    assert(item->key == APSIS_P9_PARAM_STRING_BRIGHTNESS);
    assert(item->value == APSIS_Q15_MILLI(620));

    commit_all(&state, &pending);
    assert(state.string.brightness_q15 == APSIS_Q15_MILLI(620));
    assert(state.mother.sync == 0u);
    assert(apsis_mother_pending_count(&pending) == 0u);
}

static void test_drift_queue(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisDriftParamEdit edit;
    uint16_t old_depth, old_strength;
    reset(&state, &pending);
    old_depth = state.drift.depth_q15;
    old_strength = state.temper.strength_q15;
    apsis_mother_set_sync(&state.mother, &pending, 1u);

    edit.param = APSIS_DRIFT_PARAM_DEPTH;
    edit.value = APSIS_Q15_MILLI(510);
    assert(apsis_controls_apply_drift_edit(&state, &pending, &edit, 17) == 1u);
    assert(state.drift.depth_q15 == old_depth);

    edit.param = APSIS_DRIFT_PARAM_TUNING_STRENGTH;
    edit.value = APSIS_Q15_MILLI(730);
    assert(apsis_controls_apply_drift_edit(&state, &pending, &edit, 17) == 1u);
    assert(state.temper.strength_q15 == old_strength);
    assert(apsis_mother_pending_count(&pending) == 2u);

    commit_all(&state, &pending);
    assert(state.drift.depth_q15 == APSIS_Q15_MILLI(510));
    assert(state.temper.strength_q15 == APSIS_Q15_MILLI(730));
}

static void test_bloom_and_space_queue(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisBloomParamEdit bloom;
    ApsisSpaceControlEdit space;
    uint16_t old_bloom_send;
    reset(&state, &pending);
    old_bloom_send = state.bloom.send_q15;
    apsis_mother_set_sync(&state.mother, &pending, 1u);

    bloom.param = APSIS_BLOOM_PARAM_UNFURL;
    bloom.value = 1230;
    assert(apsis_controls_apply_bloom_edit(&state, &pending, &bloom, 20) == 1u);
    assert(state.bloom.unfurl_ms != 1230u);

    space.param = APSIS_SPACE_PARAM_DAMPING_HZ;
    space.source = 0u;
    space.value = 9700;
    assert(apsis_controls_apply_space_edit(&state, &pending, &space, 20) == 1u);
    assert(state.space.damping_hz != 9700u);

    space.param = APSIS_SPACE_PARAM_SOURCE_SEND;
    space.source = APSIS_MOTHER_SOURCE_BLOOM;
    space.value = APSIS_Q15_MILLI(440);
    assert(apsis_controls_apply_space_edit(&state, &pending, &space, 20) == 1u);
    assert(state.bloom.send_q15 == old_bloom_send);
    assert(apsis_mother_pending_count(&pending) == 3u);

    commit_all(&state, &pending);
    assert(state.bloom.unfurl_ms == 1230u);
    assert(state.space.damping_hz == 9700u);
    assert(state.bloom.send_q15 == APSIS_Q15_MILLI(440));
}

static void test_defensive_bounds_and_dispatch(void)
{
    ApsisState state;
    ApsisMotherPending pending;
    ApsisMotherPendingItem item;
    reset(&state, &pending);

    assert(apsis_controls_apply_structural_param(
        &state, &pending, APSIS_P9_PARAM_SPACE_DECAY_MS, 99999, 0) == 1u);
    assert(state.space.decay_ms == 12000u);

    assert(apsis_controls_apply_structural_param(
        &state, &pending, APSIS_P9_PARAM_STRING_SPAN,
        APSIS_Q15_MILLI(1010), 0) == 1u);
    assert(state.string.span_q15 == APSIS_Q15_ONE);

    assert(apsis_controls_apply_structural_param(
        &state, &pending, APSIS_P9_PARAM_DRIFT_DEPTH, 0, 0) == 1u);
    assert(state.drift.depth_q15 == APSIS_Q15_MILLI(150));

    item.kind = APSIS_MOTHER_PENDING_TOGGLE;
    item.key = APSIS_P9_PARAM_STRING_RESONANCE;
    item.value = APSIS_Q15_MILLI(500);
    assert(apsis_controls_apply_pending_item(&state, &item, 0) == 0u);

    state.mother.sync = 1u;
    assert(apsis_controls_apply_structural_param(
        &state, 0, APSIS_P9_PARAM_SPACE_RETURN,
        APSIS_Q15_MILLI(300), 0) == 0u);
}

int main(void)
{
    test_live_string_apply();
    test_hold_coalesce_and_commit();
    test_drift_queue();
    test_bloom_and_space_queue();
    test_defensive_bounds_and_dispatch();
    puts("PASS: P9H structural apply bridge preserves live/->1/commit semantics");
    return 0;
}
