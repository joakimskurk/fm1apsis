#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_keep.h"
#include "apsis_chain.h"
#include "apsis_mother.h"

static void test_keep_capture_and_full_recall(void)
{
    ApsisState source, target;
    ApsisKeepSnapshot snap;
    ApsisKeepApplyOptions o = {0};

    apsis_state_default(&source);
    source.string.left_split_q15 = APSIS_Q15_MILLI(400);
    source.string.right_split_q15 = APSIS_Q15_MILLI(600);
    source.string.surface_mode = APSIS_STRING_CLASSIC;
    source.string.linked = 0u;
    source.string.left_anchor = APSIS_ANCHOR_THIRD;
    source.string.right_anchor = APSIS_ANCHOR_NINTH;
    source.string.left_follow_gravity = 0u;
    source.string.right_follow_gravity = 0u;
    source.string.left_gravity_degree = 2;
    source.string.right_gravity_degree = 5;
    source.drift.amount_q15 = APSIS_Q15_MILLI(550);
    source.clock.n = 16u;
    source.clock.bounds[0] = 4u;
    source.clock.bounds[1] = 8u;
    source.clock.bounds[2] = 12u;
    source.clock.main_mask = 0x1FFFFu;
    source.clock.inner_mask = 0x0249u;
    source.clock.snap_mask = 0x1080u;
    source.spiral.route_count = 4u;
    source.spiral.route[0] = 0u;
    source.spiral.route[1] = 35u;
    source.spiral.route[2] = 40u;
    source.spiral.route[3] = 7u;
    source.temper.mode = APSIS_TUNING_JUST;
    source.gravity.tonic = 11u;
    source.gravity.scale = APSIS_SCALE_MAJOR;
    source.gravity.degree = 7u;
    source.gravity.voicing = APSIS_GRAVITY_AIR;
    source.gravity.voices = 5u;
    source.gravity.inversion = 4u;

    snap = apsis_keep_capture(&source);
    assert(snap.valid == 1u);
    assert(snap.split_q15 == APSIS_Q15_MILLI(600));
    assert(snap.drift_amount_q15 == APSIS_Q15_MILLI(550));
    assert(snap.clock_n == 16u);
    assert(snap.gravity_ring == APSIS_GRAVITY_AIR);

    apsis_state_default(&target);
    target.string.level_q15 = APSIS_Q15_MILLI(130);
    target.string.send_q15 = APSIS_Q15_MILLI(90);
    target.clock.bpm = 99u;
    target.gravity.scale_lock = 0u;

    assert(apsis_keep_apply(&target, &snap, &o) == 1u);
    assert(target.string.left_split_q15 == APSIS_Q15_MILLI(400));
    assert(target.string.right_split_q15 == APSIS_Q15_MILLI(600));
    assert(target.string.surface_mode == APSIS_STRING_CLASSIC);
    assert(target.string.left_anchor == APSIS_ANCHOR_THIRD);
    assert(target.string.right_anchor == APSIS_ANCHOR_NINTH);
    assert(target.drift.amount_q15 == APSIS_Q15_MILLI(550));
    assert(target.clock.n == 16u);
    assert(target.clock.bounds[0] == 4u && target.clock.bounds[1] == 8u && target.clock.bounds[2] == 12u);
    assert(target.clock.main_mask == 0xFFFFu);
    assert(target.spiral.route_count == 3u);
    assert(target.spiral.route[0] == 0u && target.spiral.route[1] == 35u && target.spiral.route[2] == 7u);
    assert(target.temper.mode == APSIS_TUNING_JUST);
    assert(target.gravity.tonic == 11u);
    assert(target.gravity.scale == APSIS_SCALE_MAJOR);
    assert(target.gravity.degree == 7u);
    assert(target.gravity.voicing == APSIS_GRAVITY_AIR);
    assert(target.gravity.selected_ring == APSIS_GRAVITY_AIR);
    assert(target.gravity.voices == 5u && target.gravity.inversion == 4u);

    /* KEEP is musical memory, not a full preset. */
    assert(target.string.level_q15 == APSIS_Q15_MILLI(130));
    assert(target.string.send_q15 == APSIS_Q15_MILLI(90));
    assert(target.clock.bpm == 99u);
    assert(target.gravity.scale_lock == 0u);
}

static void test_keep_bank_selection(void)
{
    ApsisState s;
    ApsisKeepBank bank;
    uint8_t i;
    apsis_state_default(&s);
    apsis_keep_bank_reset(&bank);
    assert(bank.selected_slot == -1);
    assert(apsis_keep_capture_to_bank(&bank, &s) == 0);
    assert(bank.slots[0].valid == 1u);

    bank.selected_slot = -1;
    assert(apsis_keep_capture_to_bank(&bank, &s) == 1);
    for (i = 0u; i < APSIS_KEEP_SLOTS; ++i) bank.slots[i].valid = 1u;
    bank.selected_slot = -1;
    assert(apsis_keep_capture_to_bank(&bank, &s) == 7);
}

static void test_chain_normalization_and_duration(void)
{
    ApsisState s;
    ApsisChainCell c, n;
    ApsisChainSequence seq;
    apsis_state_default(&s);
    s.chain.default_morph_quarters = APSIS_CHAIN_MORPH_HALF;

    c.slot = 9u;
    c.cycles = 99u;
    c.fine_pulses = 99u;
    c.chance_q15 = 65535u;
    c.transpose = 20;
    c.morph_quarters = 3u;
    n = apsis_chain_normalize_cell(&c, &s.chain, 12u);
    assert(n.slot == 7u);
    assert(n.cycles == 32u);
    assert(n.fine_pulses == 11u);
    assert(n.chance_q15 == APSIS_Q15_ONE);
    assert(n.transpose == 12);
    assert(n.morph_quarters == APSIS_CHAIN_MORPH_HALF);

    c = apsis_chain_default_cell(0u, &s.chain, 12u);
    c.cycles = 0u;
    c.fine_pulses = 0u;
    n = apsis_chain_normalize_cell(&c, &s.chain, 12u);
    assert(n.cycles == 1u && n.fine_pulses == 0u);

    c.cycles = 4u;
    c.fine_pulses = 2u;
    assert(apsis_chain_cell_pulses(&c, &s.chain, 12u) == 50u);

    apsis_chain_sequence_reset(&seq);
    seq.count = 2u;
    seq.cells[0] = apsis_chain_default_cell(0u, &s.chain, 12u);
    seq.cells[0].cycles = 4u;
    seq.cells[0].fine_pulses = 0u;
    seq.cells[1] = apsis_chain_default_cell(1u, &s.chain, 12u);
    seq.cells[1].cycles = 0u;
    seq.cells[1].fine_pulses = 5u;
    assert(apsis_chain_content_pulses(&seq, &s.chain, 12u) == 53u);
    s.chain.loop_sync = APSIS_CHAIN_SYNC_ONE;
    assert(apsis_chain_master_pulses(&seq, &s.chain, 12u) == 60u);
    s.chain.loop_sync = APSIS_CHAIN_SYNC_PULSE;
    assert(apsis_chain_master_pulses(&seq, &s.chain, 12u) == 53u);

    apsis_chain_sequence_reset(&seq);
    s.chain.loop_sync = APSIS_CHAIN_SYNC_ONE;
    assert(apsis_chain_master_pulses(&seq, &s.chain, 12u) == 12u);
    s.chain.loop_sync = APSIS_CHAIN_SYNC_PULSE;
    assert(apsis_chain_master_pulses(&seq, &s.chain, 12u) == 1u);
}

static void test_chain_lane_scope_transpose_and_morph(void)
{
    ApsisState source, target;
    ApsisKeepBank bank;
    ApsisChainCell cell;
    ApsisChainMorphTargets mt;

    apsis_state_default(&source);
    source.gravity.tonic = 10u;
    source.gravity.scale = APSIS_SCALE_MAJOR;
    source.gravity.degree = 5u;
    source.gravity.voicing = APSIS_GRAVITY_AIR;
    source.gravity.voices = 5u;
    source.gravity.inversion = 3u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(400);
    source.string.right_split_q15 = APSIS_Q15_MILLI(600);
    source.drift.amount_q15 = APSIS_Q15_MILLI(700);

    apsis_keep_bank_reset(&bank);
    bank.slots[0] = apsis_keep_capture(&source);

    apsis_state_default(&target);
    target.gravity.degree = 2u;
    cell = apsis_chain_default_cell(0u, &target.chain, target.clock.n);
    cell.transpose = 3;
    assert(apsis_chain_apply_cell(&target, &bank, &target.chain, &cell, 1u, &mt) == 1u);
    assert(target.gravity.tonic == 1u);
    assert(target.gravity.scale == APSIS_SCALE_MAJOR);
    assert(target.gravity.degree == 2u); /* default scope: key/scale only */

    apsis_state_default(&target);
    target.chain.scope_key_scale = 0u;
    target.chain.scope_chord = 1u;
    cell = apsis_chain_default_cell(0u, &target.chain, target.clock.n);
    assert(apsis_chain_apply_cell(&target, &bank, &target.chain, &cell, 1u, &mt) == 1u);
    assert(target.gravity.tonic == 0u);
    assert(target.gravity.degree == 5u);
    assert(target.gravity.voicing == APSIS_GRAVITY_AIR);

    apsis_state_default(&target);
    target.chain.scope_key_scale = 0u;
    target.chain.scope_tension = 1u;
    target.chain.scope_drift = 1u;
    cell = apsis_chain_default_cell(0u, &target.chain, target.clock.n);
    cell.morph_quarters = APSIS_CHAIN_MORPH_QUARTER;
    assert(apsis_chain_apply_cell(&target, &bank, &target.chain, &cell, 1u, &mt) == 1u);
    assert(target.string.left_split_q15 == APSIS_Q15_MILLI(470));
    assert(target.string.right_split_q15 == APSIS_Q15_MILLI(530));
    assert(target.drift.amount_q15 == APSIS_Q15_MILLI(420));
    assert(mt.morph_quarters == APSIS_CHAIN_MORPH_QUARTER);
    assert(mt.has_tension == 1u && mt.has_drift == 1u);
    assert(mt.target_left_split_q15 == APSIS_Q15_MILLI(400));
    assert(mt.target_right_split_q15 == APSIS_Q15_MILLI(600));
    assert(mt.target_drift_amount_q15 == APSIS_Q15_MILLI(700));

    cell.morph_quarters = APSIS_CHAIN_MORPH_NONE;
    assert(apsis_chain_apply_cell(&target, &bank, &target.chain, &cell, 1u, &mt) == 1u);
    assert(target.string.left_split_q15 == APSIS_Q15_MILLI(400));
    assert(target.string.right_split_q15 == APSIS_Q15_MILLI(600));
    assert(target.drift.amount_q15 == APSIS_Q15_MILLI(700));

    assert(apsis_chain_should_apply(0u, 1u) == 0u);
    assert(apsis_chain_should_apply(0u, 0u) == 1u); /* JS Math.random() <= 0 edge */
    assert(apsis_chain_should_apply(APSIS_Q15_ONE, 0xFFFFFFFFu) == 1u);
}

static void test_mother_queue_and_horizon(void)
{
    ApsisState s;
    ApsisMotherPending p;
    const ApsisMotherPendingItem *item;

    apsis_state_default(&s);
    apsis_mother_pending_reset(&p);
    assert(apsis_mother_pending_count(&p) == 0u);
    assert(p.pending_start_abs == -1);

    apsis_mother_set_sync(&s.mother, &p, 1u);
    assert(apsis_mother_should_hold(&s.mother, &p, APSIS_MOTHER_DOMAIN_STRUCTURAL) == 1u);
    assert(apsis_mother_should_hold(&s.mother, &p, APSIS_MOTHER_DOMAIN_MOTHER) == 0u);
    assert(apsis_mother_should_hold(&s.mother, &p, APSIS_MOTHER_DOMAIN_OUTPUT) == 0u);

    assert(apsis_mother_queue_param(&p, 10u, 111, 100) == 1u);
    assert(p.count == 1u && p.pending_start_abs == 100);
    assert(apsis_mother_queue_param(&p, 10u, 222, 101) == 1u);
    assert(p.count == 1u);
    item = apsis_mother_pending_at(&p, 0u);
    assert(item && item->kind == APSIS_MOTHER_PENDING_PARAM && item->key == 10u && item->value == 222);

    assert(apsis_mother_queue_param(&p, 20u, 333, 102) == 1u);
    assert(apsis_mother_queue_toggle(&p, 30u, 103) == 1u);
    assert(p.count == 3u);
    assert(apsis_mother_queue_toggle(&p, 30u, 104) == 1u);
    assert(p.count == 2u);
    assert(apsis_mother_pending_at(&p, 0u)->key == 10u);
    assert(apsis_mother_pending_at(&p, 1u)->key == 20u);

    apsis_mother_queue_gravity(&p, 9u, 4u, 105);
    assert(apsis_mother_pending_count(&p) == 3u);
    assert(p.gravity_degree == 7u && p.gravity_ring == 2u);
    assert(apsis_mother_queue_control(&p, 40u, 444, 106) == 1u);
    assert(apsis_mother_pending_count(&p) == 4u);
    assert(apsis_mother_pending_at(&p, 2u)->kind == APSIS_MOTHER_PENDING_CONTROL);

    apsis_mother_set_horizon(&s.mother, &p, 4u, 200);
    assert(s.mother.horizon == 4u && p.pending_start_abs == 200);
    assert(apsis_mother_should_commit(&s.mother, &p, 203, 0u) == 0u);
    assert(apsis_mother_should_commit(&s.mother, &p, 204, 1u) == 0u);
    assert(apsis_mother_should_commit(&s.mother, &p, 204, 0u) == 1u);

    apsis_mother_begin_commit(&p);
    assert(apsis_mother_should_hold(&s.mother, &p, APSIS_MOTHER_DOMAIN_STRUCTURAL) == 0u);
    apsis_mother_finish_commit(&s.mother, &p);
    assert(s.mother.sync == 0u && apsis_mother_pending_count(&p) == 0u && p.pending_start_abs == -1);

    apsis_mother_set_sync(&s.mother, &p, 1u);
    assert(apsis_mother_queue_toggle(&p, 77u, 10) == 1u);
    assert(apsis_mother_queue_toggle(&p, 77u, 11) == 1u);
    assert(apsis_mother_pending_count(&p) == 0u && p.pending_start_abs == -1);

    assert(apsis_mother_queue_param(&p, 88u, 1, -1) == 1u);
    apsis_mother_set_horizon(&s.mother, &p, 1u, -1);
    assert(apsis_mother_should_commit(&s.mother, &p, 0, 0u) == 1u);
    apsis_mother_set_sync(&s.mother, &p, 0u);
    assert(apsis_mother_pending_count(&p) == 0u);
}

int main(void)
{
    test_keep_capture_and_full_recall();
    test_keep_bank_selection();
    test_chain_normalization_and_duration();
    test_chain_lane_scope_transpose_and_morph();
    test_mother_queue_and_horizon();
    puts("PASS: APSIS source-derived KEEP / CHAIN / MOTHER semantics");
    return 0;
}
