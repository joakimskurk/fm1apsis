#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_runtime.h"

#define TEST_HASH_OFFSET 2166136261u
#define TEST_HASH_PRIME 16777619u

static uint8_t same_bytes(const void *a, const void *b, size_t n)
{
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;
    while (n--) if (*x++ != *y++) return 0u;
    return 1u;
}

static uint32_t test_lcg(uint32_t x)
{
    if (!x) x = 0x41505349u;
    return 1664525u * x + 1013904223u;
}

static uint32_t test_hash_u32(uint32_t h, uint32_t v)
{
    h ^= v;
    return h * TEST_HASH_PRIME;
}

static void init_all(ApsisState *s, ApsisRuntime *r,
                     ApsisKeepBank *bank, ApsisChainSequence *chain,
                     ApsisMotherPending *mother, ApsisBloomVoicingState *bloom,
                     uint32_t seed)
{
    apsis_state_default(s);
    apsis_runtime_reset(r, 44100u, 0u, seed);
    apsis_keep_bank_reset(bank);
    apsis_chain_sequence_reset(chain);
    apsis_mother_pending_reset(mother);
    apsis_bloom_voicing_reset(bloom);
}

static void test_inactive_context_is_bit_exact_m3c(void)
{
    ApsisState s0, s1;
    ApsisRuntime r0, r1;
    ApsisKeepBank b0, b1;
    ApsisChainSequence c0, c1;
    ApsisMotherPending m0, m1;
    ApsisBloomVoicingState v0, v1;
    ApsisStringAttractorRuntime a;
    uint64_t remaining = 10u * 44100u;

    init_all(&s0, &r0, &b0, &c0, &m0, &v0, 0x12345678u);
    init_all(&s1, &r1, &b1, &c1, &m1, &v1, 0x12345678u);
    apsis_runtime_start(&r0);
    apsis_runtime_start_attractor(&r1, &s1, &a);

    while (remaining) {
        uint32_t frames = remaining > 256u ? 256u : (uint32_t)remaining;
        apsis_runtime_process(&r0, &s0, &b0, &c0, &m0, &v0, frames, 0, 0);
        apsis_runtime_process_attractor(&r1, &s1, &b1, &c1, &m1, &v1,
                                        frames, 0, 0, &a);
        remaining -= frames;
    }

    assert(sizeof(ApsisRuntime) == 21928u);
    assert(same_bytes(&r0, &r1, sizeof(r0)));
    assert(same_bytes(&s0, &s1, sizeof(s0)));
    assert(same_bytes(&b0, &b1, sizeof(b0)));
    assert(same_bytes(&c0, &c1, sizeof(c0)));
    assert(same_bytes(&m0, &m1, sizeof(m0)));
    assert(same_bytes(&v0, &v1, sizeof(v0)));
    assert(a.advance_count == 0u);
    assert(a.pending_high_water == 0u);
    assert(a.bind_count == 0u);
    assert(a.pending_overflowed == 0u);
}

static void test_random_order_precedes_spiral(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisStringAttractorRuntime a;
    uint32_t seed = 0x13579bdfu;
    uint32_t v1, v2, h;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, seed);
    s.clock.bpm = 35u;
    s.clock.division_num = 1u;
    s.clock.division_den = 1u; /* next pulse > 1.2 s lookahead */
    s.clock.ghost_q15 = 0u;
    s.string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_1;
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    s.string.filter_attractor_gates = 0xffffu;
    s.string.pulse_diag = APSIS_STRING_PULSE_FULL;
    s.spiral.attractor_rate = 1u;
    s.spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s.spiral.route_count = 1u;
    s.spiral.route[0] = 0u;
    s.spiral.attractor_gates = 1u;

    apsis_runtime_start_attractor(&r, &s, &a);
    apsis_runtime_process_attractor(&r, &s, &bank, &chain, &mother, &bloom,
                                    1u, 0, 0, &a);

    v1 = test_lcg(seed);
    v2 = test_lcg(v1);
    h = TEST_HASH_OFFSET;
    h = test_hash_u32(h, 0x53410000u); /* String Attractor pos 0 */
    h = test_hash_u32(h, v1);
    h = test_hash_u32(h, 0x53000000u); /* Spiral Attractor pos 0 */
    h = test_hash_u32(h, v2);
    assert(r.random_draw_count == 2u);
    assert(r.random_state == v2);
    assert(r.random_trace_hash == h);
    assert(a.advance_count == 1u);
    assert(a.mutation_count == 0u);
}

static void configure_extreme(ApsisState *s)
{
    uint8_t i;
    s->clock.n = 3u;
    s->clock.bpm = 180u;
    s->clock.division_num = 1u;
    s->clock.division_den = 4u;
    s->clock.swing_q15 = APSIS_Q15_MILLI(350);
    s->clock.main_mask = 0x7u;
    s->clock.snap_mask = 0x7u;
    s->clock.inner_mask = 0x7u;
    s->clock.ghost_q15 = APSIS_Q15_ONE;
    s->clock.step_one_resync = 1u;

    s->string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s->string.rhythm_trigger = APSIS_STRING_TRIGGER_EVERY;
    s->string.rhythm_every = 1u;
    s->string.pulse_diag = APSIS_STRING_PULSE_FULL;
    s->string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s->string.filter_attractor_every = APSIS_STRING_ATTRACTOR_HALF;
    s->string.filter_attractor_length = 16u;
    s->string.attractor_target = APSIS_STRING_ATTRACTOR_BOTH;
    s->string.filter_attractor_lock_q15 = 0u; /* maximal mutation draw load */
    s->string.filter_attractor_strum_q15 = APSIS_Q15_ONE;
    s->string.filter_attractor_strum_level_q15 = APSIS_Q15_ONE;
    s->string.filter_attractor_gates = 0xffffu;

    s->spiral.attractor_rate = 4u;
    s->spiral.route_count = 3u;
    s->spiral.route[0] = 0u;
    s->spiral.route[1] = 4u;
    s->spiral.route[2] = 7u;
    s->spiral.attractor_lock_q15 = APSIS_Q15_ONE;
    s->spiral.attractor_gates = 0x00ffffffu;
    for (i = 0u; i < APSIS_ATTRACTOR_STEPS; ++i)
        s->spiral.attractor_energy_q15[i] = APSIS_Q15_MILLI(900);

    s->bloom.cycles = 1u;
    s->bloom.rate = 4u;
    s->bloom.trigger_step = 0u;
    s->bloom.delay_ms = 0u;
    s->bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    s->bloom.unfurl_ms = 3600u;
}

typedef struct {
    uint16_t queue_high_water;
    uint16_t pending_high_water;
    uint32_t advance_count;
    uint32_t mutation_count;
    uint32_t excitation_count;
    uint32_t bind_count;
    uint32_t random_draw_count;
    uint32_t random_trace_hash;
    uint32_t late_trace_hash;
} ExtremeResult;

static ExtremeResult run_extreme(uint32_t seed)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisStringAttractorRuntime a;
    ExtremeResult out;
    uint64_t remaining = 60u * 44100u;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, seed);
    configure_extreme(&s);
    apsis_runtime_start_attractor(&r, &s, &a);

    while (remaining) {
        uint32_t frames = remaining > 512u ? 512u : (uint32_t)remaining;
        apsis_runtime_process_attractor(&r, &s, &bank, &chain, &mother, &bloom,
                                        frames, 0, 0, &a);
        assert(a.binding_count <= APSIS_STRING_ATTRACTOR_BINDS_PER_BLOCK_MAX);
        remaining -= frames;
    }

    assert(r.queue_overflowed == 0u);
    assert(r.scheduler_starved == 0u);
    assert(r.late_slot_overflowed == 0u);
    assert(a.pending_overflowed == 0u);
    assert(a.bind_stale_count == 0u);
    assert(a.pending_high_water < APSIS_STRING_ATTRACTOR_PENDING_CAP);
    assert(r.spiral_slot_high_water < APSIS_RUNTIME_SPIRAL_SLOTS);
    assert(r.bloom_slot_high_water < APSIS_RUNTIME_BLOOM_SLOTS);
    assert(a.advance_count > 1000u);
    assert(a.mutation_count > 1000u);
    assert(a.excitation_count == a.advance_count);
    assert(a.bind_count > 2000u);
    assert(r.spiral_occurrence_count > 2000u);
    assert(r.bloom_occurrence_count > 500u);

    out.queue_high_water = r.queue_high_water;
    out.pending_high_water = a.pending_high_water;
    out.advance_count = a.advance_count;
    out.mutation_count = a.mutation_count;
    out.excitation_count = a.excitation_count;
    out.bind_count = a.bind_count;
    out.random_draw_count = r.random_draw_count;
    out.random_trace_hash = r.random_trace_hash;
    out.late_trace_hash = r.late_trace_hash;
    return out;
}

static void test_extreme_capacity_and_determinism(void)
{
    ExtremeResult a = run_extreme(0x5a17c0deu);
    ExtremeResult b = run_extreme(0x5a17c0deu);
    assert(same_bytes(&a, &b, sizeof(a)));
    printf("PASS: M4c extreme shared timeline (queue=%u/%u, pending=%u/%u, advances=%lu, mutations=%lu, binds=%lu, random=%08lx, late=%08lx)\n",
           (unsigned)a.queue_high_water, (unsigned)APSIS_TRANSPORT_QUEUE_CAP,
           (unsigned)a.pending_high_water, (unsigned)APSIS_STRING_ATTRACTOR_PENDING_CAP,
           (unsigned long)a.advance_count, (unsigned long)a.mutation_count,
           (unsigned long)a.bind_count,
           (unsigned long)a.random_trace_hash,
           (unsigned long)a.late_trace_hash);
}

static void test_start_stop_regenerates_and_cancels(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisKeepBank bank;
    ApsisChainSequence chain;
    ApsisMotherPending mother;
    ApsisBloomVoicingState bloom;
    ApsisStringAttractorRuntime a;
    uint16_t first;

    init_all(&s, &r, &bank, &chain, &mother, &bloom, 0x11111111u);
    s.string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_HALF;
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_BOTH;
    s.string.filter_attractor_gates = 0xffffu;
    apsis_runtime_start_attractor(&r, &s, &a);
    first = a.cell_q15[0];
    apsis_runtime_process_attractor(&r, &s, &bank, &chain, &mother, &bloom,
                                    256u, 0, 0, &a);
    assert(a.pending_high_water > 0u);
    a.cell_q15[0] ^= 0x1234u;
    apsis_runtime_stop_attractor(&r, &a);
    assert(a.pos == 0u);
    apsis_runtime_start_attractor(&r, &s, &a);
    assert(a.cell_q15[0] == first);
    assert(a.pending_high_water == 0u);
    assert(a.bind_count == 0u);
    assert(a.pending_overflowed == 0u);
}

int main(void)
{
    test_inactive_context_is_bit_exact_m3c();
    test_random_order_precedes_spiral();
    test_extreme_capacity_and_determinism();
    test_start_stop_regenerates_and_cancels();
    puts("PASS: APSIS M4c shared-heap String Attractor runtime");
    return 0;
}
