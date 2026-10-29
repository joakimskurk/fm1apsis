#include <assert.h>
#include <stdio.h>

#include "apsis_runtime.h"

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, 0x12345678u);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static void process_to(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                       ApsisChainSequence *chain, ApsisMotherPending *mother,
                       ApsisBloomVoicingState *bloom, uint64_t target)
{
    while (r->cursor_sample < target) {
        uint64_t remain = target - r->cursor_sample;
        uint32_t frames = remain > 256u ? 256u : (uint32_t)remain;
        apsis_runtime_process(r, s, bank, chain, mother, bloom, frames, 0, 0);
    }
}

static uint8_t mother_apply_test(ApsisState *s, const ApsisMotherPendingItem *item, void *user)
{
    uint32_t *calls = (uint32_t *)user;
    if (calls) (*calls)++;
    if (!s || !item) return 0u;
    if (item->kind == APSIS_MOTHER_PENDING_PARAM && item->key == 10u) {
        s->drift.amount_q15 = (uint16_t)item->value;
        return 1u;
    }
    return 0u;
}

static void test_default_block_runtime(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);

    assert(r.cursor_sample == 256u);
    assert(r.event_count[APSIS_EVENT_STRING_FILTER_CLOCK] == 1u);
    assert(r.event_count[APSIS_EVENT_PLAYHEAD] == 1u);
    assert(r.event_count[APSIS_EVENT_MOTHER_PULSE] == 1u);
    assert(r.event_count[APSIS_EVENT_CLOCK_MAIN] == 1u);
    assert(r.event_count[APSIS_EVENT_SPIRAL_TICK] == 1u);
    assert(r.event_count[APSIS_EVENT_CHAIN_PULSE] == 1u);
    assert(r.event_count[APSIS_EVENT_STEP_ONE_RESYNC] == 0u);
    assert(r.queue_high_water >= 7u);
    assert(r.queue_overflowed == 0u);
    assert(apsis_event_queue_count(&r.queue) >= 1u); /* second 2x Spiral tick */
}

static void test_step_one_resync_precedes_spiral(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.clock.step_one_resync = 1u;
    s.spiral.route_count = 1u;
    s.spiral.route[0] = 9u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 1u;
    r.spiral_attractor_pos = 5u;
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);

    assert(r.event_count[APSIS_EVENT_STEP_ONE_RESYNC] == 1u);
    assert(r.spiral_attractor_pos == 1u);
    assert(r.spiral_occurrence_count == 1u);
    assert(r.last_spiral.trigger_pos == 0u);
    assert(r.last_spiral.route_point == 9u);
}

static void test_gravity_quantized_boundary(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint64_t target;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u, 2u,
                                         r.cursor_sample, 0u) == 3u);
    target = r.gravity_pending.target_sample;
    assert(target > r.cursor_sample);
    process_to(&r, &s, &bank, &chain, &mother, &bloom, target);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(s.gravity.degree == 4u);
    assert(s.gravity.voicing == 2u && s.gravity.selected_ring == 2u);
    assert(r.gravity_pending.active == 0u);
}

static void test_mother_horizon_and_ordered_gravity(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint32_t calls = 0u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 1u, -1);
    assert(apsis_mother_queue_param(&mother, 10u, APSIS_Q15_MILLI(777), -1) == 1u);
    apsis_mother_queue_gravity(&mother, 3u, 2u, -1);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u,
                          mother_apply_test, &calls);

    assert(calls == 1u);
    assert(s.drift.amount_q15 == APSIS_Q15_MILLI(777));
    assert(s.gravity.degree == 3u && s.gravity.voicing == 2u);
    assert(s.mother.sync == 0u);
    assert(apsis_mother_pending_count(&mother) == 0u);
    assert(r.mother_apply_blocked == 0u);
}

static void test_mother_gravity_owns_runtime_request(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_runtime_start(&r);
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 6u, 1u, 0u, 0u) == 2u);
    assert(r.gravity_pending.active == 0u);
    assert(mother.gravity_pending == 1u);
    assert(s.gravity.degree == 0u);
}

static void test_chain_start_advance_and_morph(void)
{
    ApsisState s, source;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint64_t next_pulse = 18375u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_state_default(&source);
    source.gravity.tonic = 3u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(400);
    source.string.right_split_q15 = APSIS_Q15_MILLI(600);
    bank.slots[0] = apsis_keep_capture(&source);
    source.gravity.tonic = 7u;
    source.string.left_split_q15 = APSIS_Q15_MILLI(350);
    source.string.right_split_q15 = APSIS_Q15_MILLI(650);
    bank.slots[1] = apsis_keep_capture(&source);

    s.chain.scope_key_scale = 1u;
    s.chain.scope_tension = 1u;
    chain.count = 2u;
    chain.cells[0] = apsis_chain_default_cell(0u, &s.chain, s.clock.n);
    chain.cells[0].cycles = 0u;
    chain.cells[0].fine_pulses = 1u;
    chain.cells[0].morph_quarters = APSIS_CHAIN_MORPH_QUARTER;
    chain.cells[1] = apsis_chain_default_cell(1u, &s.chain, s.clock.n);
    chain.cells[1].cycles = 0u;
    chain.cells[1].fine_pulses = 1u;

    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(chain.armed == 0u && chain.pos == 0u);
    assert(s.gravity.tonic == 3u);
    assert(r.chain_morph_active == 1u);

    /* A quarter-cycle morph lasts 55,125 samples at the default clock. At
     * 256 samples the smoothstep can legitimately quantize to the exact Q15
     * start value, so inspect it after meaningful progress but before the
     * one-pulse cell advances at sample 18,375. */
    process_to(&r, &s, &bank, &chain, &mother, &bloom, 10000u);
    assert(s.string.left_split_q15 > APSIS_Q15_MILLI(400));
    assert(s.string.left_split_q15 < APSIS_Q15_MILLI(470));

    process_to(&r, &s, &bank, &chain, &mother, &bloom, next_pulse);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(chain.pos == 1u);
    assert(s.gravity.tonic == 7u);
}

static void test_spiral_symbolic_occurrences(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.spiral.route_count = 2u;
    s.spiral.route[0] = 2u;
    s.spiral.route[1] = 5u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = (1u << 0) | (1u << 2);
    s.spiral.attractor_energy_q15[0] = APSIS_Q15_MILLI(820);
    s.spiral.attractor_energy_q15[1] = APSIS_Q15_MILLI(340);
    s.spiral.attractor_energy_q15[2] = APSIS_Q15_MILLI(680);

    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(r.spiral_occurrence_count == 1u);
    assert(r.last_spiral.route_index == 0u && r.last_spiral.route_point == 2u);
    assert(r.last_spiral.trigger_pos == 0u);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, 18376u);
    assert(r.spiral_occurrence_count == 2u);
    assert(r.last_spiral.route_index == 1u && r.last_spiral.route_point == 5u);
    assert(r.last_spiral.trigger_pos == 2u);
}

static void test_mother_commit_precedes_bloom_plan(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint64_t cycle_one = 220500u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.bloom.cycles = 1u;
    s.bloom.rate = 1u;
    s.bloom.trigger_step = 0u;
    s.bloom.delay_ms = 0u;
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 12u, -1);
    apsis_mother_queue_gravity(&mother, 3u, 2u, -1);

    apsis_runtime_start(&r);
    process_to(&r, &s, &bank, &chain, &mother, &bloom, cycle_one);
    assert(r.bloom_occurrence_count == 0u);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(s.gravity.degree == 3u);
    assert(r.bloom_occurrence_count == 1u);
    assert(r.last_bloom.valid == 1u);
    assert(r.last_bloom.sample == cycle_one);
    assert(r.last_bloom.harmony.degree == 3u);
}

int main(void)
{
    test_default_block_runtime();
    test_step_one_resync_precedes_spiral();
    test_gravity_quantized_boundary();
    test_mother_horizon_and_ordered_gravity();
    test_mother_gravity_owns_runtime_request();
    test_chain_start_advance_and_morph();
    test_spiral_symbolic_occurrences();
    test_mother_commit_precedes_bloom_plan();
    puts("PASS: APSIS runtime coordinator");
    return 0;
}
