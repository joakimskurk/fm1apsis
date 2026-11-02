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

static uint32_t active_spiral_slots(const ApsisRuntime *r)
{
    uint32_t n = 0u;
    uint8_t i;
    for (i = 0u; i < APSIS_RUNTIME_SPIRAL_SLOTS; ++i)
        if (r->spiral_slots[i].active) n++;
    return n;
}

static uint32_t active_bloom_slots(const ApsisRuntime *r)
{
    uint32_t n = 0u;
    uint8_t i;
    for (i = 0u; i < APSIS_RUNTIME_BLOOM_SLOTS; ++i)
        if (r->bloom_slots[i].active) n++;
    return n;
}

static void test_stop_cancels_future_scheduler_and_bind_events(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    uint32_t binds_before;
    uint64_t stopped_at;

    init_all(&s, &r, &bank, &chain, &mother, &bloom);
    s.spiral.route_count = 1u;
    s.spiral.route[0] = 7u;
    s.spiral.attractor_rate = 1u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.attractor_gates = 0x00ffffffu;

    apsis_runtime_start(&r);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 256u, 0, 0);
    assert(r.transport.running == 1u);
    assert(apsis_event_queue_count(&r.queue) > 0u);
    assert(active_spiral_slots(&r) > 0u);
    binds_before = r.spiral_bind_count;
    stopped_at = r.cursor_sample;

    apsis_runtime_stop(&r);
    assert(r.transport.running == 0u);
    assert(apsis_event_queue_count(&r.queue) == 0u);
    assert(active_spiral_slots(&r) == 0u);
    assert(active_bloom_slots(&r) == 0u);
    assert(r.spiral_attractor_pos == 0u);
    assert(r.spiral_step == 0u);

    /* app_audio may continue to call the silent runtime while transport is
     * stopped. No cancelled future bind may leak through. */
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 40000u, 0, 0);
    assert(r.spiral_bind_count == binds_before);
    assert(r.cursor_sample == stopped_at + 40000u);

    apsis_runtime_start(&r);
    assert(r.transport.running == 1u);
    assert(r.transport.next_sample == r.cursor_sample);
    assert(r.transport.absolute_pulse == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(r.spiral_bind_count > binds_before);
}

static void test_stop_preserves_independent_pending_gravity_timer(void)
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
    assert(apsis_runtime_request_gravity(&r, &s, &mother, 5u, 2u,
                                         r.cursor_sample, 0u) == 3u);
    target = r.gravity_pending.target_sample;
    commit = target - 794u; /* nearest 18 ms at 44.1 kHz */
    assert(r.gravity_pending.active == 1u);

    apsis_runtime_stop(&r);
    assert(r.transport.running == 0u);
    assert(apsis_event_queue_count(&r.queue) == 1u);
    assert(r.gravity_pending.active == 1u);

    process_to(&r, &s, &bank, &chain, &mother, &bloom, commit);
    assert(s.gravity.degree == 0u);
    apsis_runtime_process(&r, &s, &bank, &chain, &mother, &bloom, 1u, 0, 0);
    assert(s.gravity.degree == 5u && s.gravity.voicing == 2u);
    assert(r.gravity_pending.active == 0u);
    assert(r.cursor_sample < target);
}

int main(void)
{
    test_stop_cancels_future_scheduler_and_bind_events();
    test_stop_preserves_independent_pending_gravity_timer();
    puts("PASS: APSIS M3c stop/restart scheduler cancellation");
    return 0;
}
