#include <assert.h>
#include <stdio.h>

#include "apsis_runtime.h"

static uint64_t ms_samples(uint32_t ms)
{
    return (uint64_t)(44u * ms) + (uint64_t)((100u * ms + 500u) / 1000u);
}

static void init_seed(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                      ApsisChainSequence *chain, ApsisMotherPending *mother,
                      ApsisBloomVoicingState *bloom, uint32_t seed)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, seed);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static void init_all(ApsisState *s, ApsisRuntime *r, ApsisKeepBank *bank,
                     ApsisChainSequence *chain, ApsisMotherPending *mother,
                     ApsisBloomVoicingState *bloom)
{
    init_seed(s, r, bank, chain, mother, bloom, 0x12345678u);
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

static void cross_sample(ApsisRuntime *r, ApsisState *s, ApsisKeepBank *bank,
                         ApsisChainSequence *chain, ApsisMotherPending *mother,
                         ApsisBloomVoicingState *bloom, uint64_t sample)
{
    process_to(r, s, bank, chain, mother, bloom, sample);
    apsis_runtime_process(r, s, bank, chain, mother, bloom, 1u, 0, 0);
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

static void test_default_lookahead_runtime(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    assert(s.bloom.unfurl_ms == 420u);
    s.bloom.unfurl_ms = 3600u;
    assert(s.bloom.unfurl_ms == 3600u);
    s.bloom.unfurl_ms = 420u;

    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);

    assert(r.cursor_sample == 256u);
    /* Study 126 tick() plans 1.20 s ahead. Default 72 BPM / 1/2 division is
     * 18,375 samples per pulse, so targets 0, 18,375 and 36,750 are planned. */
    assert(r.transport.absolute_pulse == 3u);
    assert(r.event_count[APSIS_EVENT_STRING_FILTER_CLOCK] == 1u);
    assert(r.event_count[APSIS_EVENT_PLAYHEAD] == 1u);
    assert(r.event_count[APSIS_EVENT_MOTHER_PULSE] == 1u);
    assert(r.event_count[APSIS_EVENT_CLOCK_MAIN] == 1u);
    assert(r.event_count[APSIS_EVENT_SPIRAL_TICK] == 1u);
    assert(r.event_count[APSIS_EVENT_CHAIN_PULSE] == 1u);
    assert(r.event_count[APSIS_EVENT_MOTHER_COMMIT_OBSERVE] == 1u);
    assert(r.event_count[APSIS_EVENT_CHAIN_APPLY_OBSERVE] == 1u);
    assert(r.queue_overflowed == 0u);
    assert(r.scheduler_starved == 0u);
    assert(r.queue_high_water > APSIS_TRANSPORT_PULSE_EVENTS_MAX);
}

static void test_step_one_resync_is_planning_time(void)
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

    /* The future Clock-1 is planned before audible target time, resets the
     * attractor to zero, then the three lookahead pulses each consume two
     * cells: positions 0..5, leaving planner position 6. */
    assert(r.spiral_attractor_pos == 6u);
    assert(r.spiral_occurrence_count == 1u);
    assert(r.last_spiral.trigger_pos == 0u);
    assert(r.last_spiral.route_point == 9u);
    assert(r.spiral_bind_count == 1u);
    assert(r.last_spiral_bind.skipped == 0u);
    assert(r.last_spiral_bind.pitch.step == 9);
}

static void test_gravity_quantized_commit_is_target_minus_18ms(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint64_t target, commit;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 4u, 2u,
                                         r.cursor_sample, 0u) == 3u);
    target = r.gravity_pending.target_sample;
    commit = target - ms_samples(18u);
    assert(target > commit && commit > r.cursor_sample);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, commit);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(s.gravity.degree == 4u);
    assert(s.gravity.voicing == 2u && s.gravity.selected_ring == 2u);
    assert(r.gravity_pending.active == 0u);
    assert(r.event_count[APSIS_EVENT_GRAVITY_COMMIT] == 1u);
    assert(r.cursor_sample < target);
}

static void test_mother_horizon_and_recovery_at_pre_target_callback(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint32_t calls = 0u;
    uint16_t before;
    const uint64_t next_clock_one = 220500u;
    const uint64_t observe = next_clock_one - 1147u; /* nearest 26 ms */

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    before = s.drift.amount_q15;
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 1u, -1);
    assert(apsis_mother_queue_param(&mother, 10u, APSIS_Q15_MILLI(777), -1) == 1u);
    apsis_runtime_start(&r);

    /* First Clock-1 callback is due at the timeline origin and has no generic
     * dispatcher. Pending state must survive and later recover. */
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(r.mother_apply_blocked == 1u);
    assert(apsis_mother_pending_count(&mother) == 1u);
    assert(s.drift.amount_q15 == before);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, observe);
    assert(s.drift.amount_q15 == before);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u,
                          mother_apply_test, &calls);
    assert(calls == 1u);
    assert(r.mother_apply_blocked == 0u);
    assert(s.drift.amount_q15 == APSIS_Q15_MILLI(777));
    assert(apsis_mother_pending_count(&mother) == 0u);
    assert(s.mother.sync == 0u);
    assert(r.cursor_sample < next_clock_one);
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

static void test_chain_discrete_precedes_target_but_morph_starts_on_target(void)
{
    ApsisState s, source;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    const uint64_t target = 18375u;
    const uint64_t observe = target - 1058u; /* nearest 24 ms */
    const uint16_t cell0_left = APSIS_Q15_MILLI(400);

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    apsis_state_default(&source);
    source.gravity.tonic = 3u;
    source.string.left_split_q15 = cell0_left;
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
    chain.cells[0].morph_quarters = APSIS_CHAIN_MORPH_NONE;
    chain.cells[1] = apsis_chain_default_cell(1u, &s.chain, s.clock.n);
    chain.cells[1].cycles = 0u;
    chain.cells[1].fine_pulses = 1u;
    chain.cells[1].morph_quarters = APSIS_CHAIN_MORPH_QUARTER;

    apsis_runtime_chain_start(&r, &s, &chain);
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(chain.pos == 0u && s.gravity.tonic == 3u);
    assert(s.string.left_split_q15 == cell0_left);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, observe);
    assert(chain.pos == 0u && s.gravity.tonic == 3u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(chain.pos == 1u);
    assert(s.gravity.tonic == 7u); /* discrete memory changed at target-24 ms */
    assert(r.chain_morph_active == 1u);
    assert(r.chain_morph_start_sample == target);
    assert(s.string.left_split_q15 == cell0_left); /* continuous lane has not moved */

    process_to(&r, &s, &bank, &chain, &mother, &bloom, target + 4000u);
    assert(s.string.left_split_q15 < cell0_left);
    assert(s.string.left_split_q15 > APSIS_Q15_MILLI(350));
}

static void test_bloom_first_petal_precedes_mother_later_petal_observes_commit(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    const uint64_t target = 220500u;
    const uint64_t occurrence = target - 1985u; /* nearest 45 ms */
    const uint64_t mother_observe = target - 1147u; /* nearest 26 ms */
    const uint64_t second_target = target + 6174u; /* 420 ms / (4-1) */
    const uint64_t second_bind = second_target - 2205u; /* 50 ms */

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.bloom.cycles = 1u;
    s.bloom.rate = 1u;
    s.bloom.trigger_step = 0u;
    s.bloom.delay_ms = 0u;
    s.bloom.unfurl_ms = 420u;
    apsis_mother_set_sync(&s.mother, &mother, 1u);
    apsis_mother_set_horizon(&s.mother, &mother, 12u, -1);
    apsis_mother_queue_gravity(&mother, 3u, 2u, -1);

    apsis_runtime_start(&r);
    process_to(&r, &s, &bank, &chain, &mother, &bloom, occurrence);
    assert(r.bloom_occurrence_count == 0u);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(r.bloom_occurrence_count == 1u);
    assert(r.bloom_bind_count == 1u); /* petal 0 effective bind == occurrence */
    assert(r.last_bloom.sample == target);
    assert(r.last_bloom.harmony.degree == 0u);
    assert(r.last_bloom_bind.pitch.harmony.degree == 0u);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, mother_observe);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(s.gravity.degree == 3u && s.gravity.voicing == 2u);
    assert(r.cursor_sample < target);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, second_bind);
    assert(r.bloom_bind_count == 1u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(r.bloom_bind_count == 2u);
    assert(r.last_bloom_bind.target_sample == second_target);
    assert(r.last_bloom_bind.pitch.voice_index == 1u);
    assert(r.last_bloom_bind.pitch.harmony.degree == 3u);
    assert(r.last_bloom_bind.pitch.harmony.voicing == 2u);
}

static void test_spiral_mute_is_rechecked_at_bind(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    const uint64_t first_future_target = 18375u;
    const uint64_t first_future_bind = first_future_target - 3528u;
    const uint64_t second_future_target = 36750u;
    const uint64_t second_future_bind = second_future_target - 3528u;
    uint32_t before;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.spiral.route_count = 1u;
    s.spiral.route[0] = 5u;
    s.spiral.attractor_rate = 1u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 0x00ffffffu;
    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(r.spiral_occurrence_count == 3u); /* all three lookahead pulses planned */
    before = r.spiral_bind_skipped;

    s.spiral.sound = 0u;
    cross_sample(&r, &s, &bank, &chain, &mother, &bloom, first_future_bind);
    assert(r.spiral_bind_skipped == before + 1u);
    assert(r.last_spiral_bind.target_sample == first_future_target);
    assert(r.last_spiral_bind.skipped == 1u);

    s.spiral.sound = 1u;
    cross_sample(&r, &s, &bank, &chain, &mother, &bloom, second_future_bind);
    assert(r.last_spiral_bind.target_sample == second_future_target);
    assert(r.last_spiral_bind.skipped == 0u);
    assert(r.last_spiral_bind.pitch.step == 5);
}

typedef struct {
    uint16_t queue_high_water;
    uint16_t spiral_slots;
    uint16_t bloom_slots;
    uint32_t random_hash;
    uint32_t late_hash;
    uint32_t spiral_occurrences;
    uint32_t spiral_binds;
    uint32_t bloom_occurrences;
    uint32_t bloom_binds;
} StressResult;

static StressResult run_stress(uint32_t seed)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    StressResult out;
    uint8_t i;
    const uint64_t duration = 30u * 44100u;

    init_seed(&s, &r, &bank, &chain, &mother, &bloom, seed);
    s.clock.n = 3u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    s.clock.main_mask = 0x7u;
    s.clock.snap_mask = 0x7u;
    s.clock.inner_mask = 0x7u;
    s.clock.ghost_q15 = APSIS_Q15_ONE;
    s.clock.step_one_resync = 1u;
    s.string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s.string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s.string.rhythm_every = 1u;
    s.string.pulse_diag = APSIS_STRING_PULSE_FULL;
    s.spiral.attractor_rate = 4u;
    s.spiral.route_count = 3u;
    s.spiral.route[0] = 0u;
    s.spiral.route[1] = 4u;
    s.spiral.route[2] = 7u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 0x00ffffffu;
    for (i = 0u; i < APSIS_ATTRACTOR_STEPS; ++i)
        s.spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(800);
    s.bloom.cycles = 1u;
    s.bloom.rate = 4u;
    s.bloom.trigger_step = 0u;
    s.bloom.delay_ms = 0u;
    s.bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    s.bloom.unfurl_ms = 3600u;

    apsis_runtime_start(&r);
    process_to(&r, &s, &bank, &chain, &mother, &bloom, duration);

    assert(r.queue_overflowed == 0u);
    assert(r.late_slot_overflowed == 0u);
    assert(r.scheduler_starved == 0u);
    assert(r.spiral_occurrence_count > 256u * 3u); /* proves uint32 ordinal path */
    assert(r.spiral_bind_count > 0u && r.bloom_bind_count > 0u);

    out.queue_high_water = r.queue_high_water;
    out.spiral_slots = r.spiral_slot_high_water;
    out.bloom_slots = r.bloom_slot_high_water;
    out.random_hash = r.random_trace_hash;
    out.late_hash = r.late_trace_hash;
    out.spiral_occurrences = r.spiral_occurrence_count;
    out.spiral_binds = r.spiral_bind_count;
    out.bloom_occurrences = r.bloom_occurrence_count;
    out.bloom_binds = r.bloom_bind_count;
    return out;
}

static StressResult test_runtime_stress_and_determinism(void)
{
    StressResult a = run_stress(0x31415926u);
    StressResult b = run_stress(0x31415926u);
    StressResult c = run_stress(0x27182818u);

    assert(a.queue_high_water == b.queue_high_water);
    assert(a.spiral_slots == b.spiral_slots);
    assert(a.bloom_slots == b.bloom_slots);
    assert(a.random_hash == b.random_hash);
    assert(a.late_hash == b.late_hash);
    assert(a.spiral_occurrences == b.spiral_occurrences);
    assert(a.spiral_binds == b.spiral_binds);
    assert(a.bloom_occurrences == b.bloom_occurrences);
    assert(a.bloom_binds == b.bloom_binds);
    assert(c.random_hash != a.random_hash || c.late_hash != a.late_hash);
    return a;
}

int main(void)
{
    StressResult stress;
    test_default_lookahead_runtime();
    test_step_one_resync_is_planning_time();
    test_gravity_quantized_commit_is_target_minus_18ms();
    test_mother_horizon_and_recovery_at_pre_target_callback();
    test_mother_gravity_owns_runtime_request();
    test_chain_discrete_precedes_target_but_morph_starts_on_target();
    test_bloom_first_petal_precedes_mother_later_petal_observes_commit();
    test_spiral_mute_is_rechecked_at_bind();
    stress = test_runtime_stress_and_determinism();
    printf("PASS: APSIS M3c runtime (sizeof=%lu, queue=%u/%u, spiral-slots=%u/%u, bloom-slots=%u/%u, random=%08x, late=%08x)\n",
           (unsigned long)sizeof(ApsisRuntime),
           (unsigned)stress.queue_high_water, (unsigned)APSIS_TRANSPORT_QUEUE_CAP,
           (unsigned)stress.spiral_slots, (unsigned)APSIS_RUNTIME_SPIRAL_SLOTS,
           (unsigned)stress.bloom_slots, (unsigned)APSIS_RUNTIME_BLOOM_SLOTS,
           (unsigned)stress.random_hash, (unsigned)stress.late_hash);
    return 0;
}
