#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_string_attractor.h"

typedef struct {
    uint32_t value[16];
    uint32_t tag[16];
    uint8_t count;
    uint8_t next;
} MockRandom;

static uint32_t mock_random(void *user, uint32_t tag)
{
    MockRandom *m = (MockRandom *)user;
    uint32_t v = m && m->next < 16u ? m->value[m->next] : 0u;
    if (m && m->count < 16u) m->tag[m->count] = tag;
    if (m) { m->count++; m->next++; }
    return v;
}

static void reset_mock(MockRandom *m)
{
    uint8_t i;
    if (!m) return;
    for (i = 0u; i < 16u; ++i) { m->value[i] = 0u; m->tag[i] = 0u; }
    m->count = 0u;
    m->next = 0u;
}

static void enable_attractor(ApsisState *s)
{
    apsis_state_default(s);
    s->string.filter_mode = APSIS_STRING_FILTER_ATTRACTOR;
    s->string.pulse_diag = APSIS_STRING_PULSE_FULL;
    s->string.filter_attractor_every = APSIS_STRING_ATTRACTOR_1;
}

static void test_default_target_and_seed_vector(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    static const uint16_t expected[8] = {
        11760u, 1037u, 28574u, 13174u, 21425u, 21172u, 32192u, 1983u
    };
    uint8_t i;
    apsis_state_default(&s);
    assert(s.string.attractor_target == APSIS_STRING_ATTRACTOR_ALTERNATE);
    apsis_string_attractor_reset(&r, &s);
    assert(r.seed == 317u && r.length == 8u && r.pos == 0u);
    for (i = 0u; i < 8u; ++i) assert(r.cell_q15[i] == expected[i]);
    assert(r.advance_count == 0u && r.mutation_count == 0u && r.excitation_count == 0u);
}

static void test_seed_length_sync(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    uint16_t first;
    enable_attractor(&s);
    apsis_string_attractor_reset(&r, &s);
    first = r.cell_q15[0];
    r.pos = 5u;
    s.string.filter_attractor_length = 4u;
    apsis_string_attractor_sync(&r, &s);
    assert(r.length == 4u && r.pos == 0u && r.cell_q15[0] == first);
    s.string.filter_attractor_length = 12u;
    apsis_string_attractor_sync(&r, &s);
    assert(r.length == 12u && r.pos == 0u && r.cell_q15[0] == first);
    s.string.filter_attractor_length = 16u;
    s.string.filter_attractor_seed = 318u;
    apsis_string_attractor_sync(&r, &s);
    assert(r.length == 16u && r.seed == 318u && r.pos == 0u);
    assert(r.cell_q15[0] != first);
}

static void test_rate_mapping(void)
{
    ApsisState s;
    enable_attractor(&s);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_HALF;
    assert(apsis_string_attractor_advances_for_pulse(&s, 0u) == 2u);
    assert(apsis_string_attractor_advances_for_pulse(&s, 7u) == 2u);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_1;
    assert(apsis_string_attractor_advances_for_pulse(&s, 7u) == 1u);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_2;
    assert(apsis_string_attractor_advances_for_pulse(&s, 1u) == 0u);
    assert(apsis_string_attractor_advances_for_pulse(&s, 2u) == 1u);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_4;
    assert(apsis_string_attractor_advances_for_pulse(&s, 3u) == 0u);
    assert(apsis_string_attractor_advances_for_pulse(&s, 4u) == 1u);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_8;
    assert(apsis_string_attractor_advances_for_pulse(&s, 7u) == 0u);
    assert(apsis_string_attractor_advances_for_pulse(&s, 8u) == 1u);
    s.string.filter_mode = APSIS_STRING_FILTER_OFF;
    assert(apsis_string_attractor_advances_for_pulse(&s, 0u) == 0u);
}

static void test_half_rate_and_lazy_mutation_draws(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    ApsisStringAttractorDecision d[2];
    MockRandom m;
    const uint64_t targets[2] = {1000u, 1500u};
    enable_attractor(&s);
    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_HALF;
    s.string.filter_attractor_lock_q15 = 0u;
    reset_mock(&m);
    m.value[0] = 0xffffffffu; m.value[1] = 0x80000000u;
    m.value[2] = 0xffffffffu; m.value[3] = 0x40000000u;
    apsis_string_attractor_reset(&r, &s);
    assert(apsis_string_attractor_plan_pulse(&r, &s, 0u, targets,
                                              mock_random, &m, d) == 2u);
    assert(m.count == 4u); /* lock + mutation for each half */
    assert(d[0].target_sample == 1000u && d[0].pos == 0u);
    assert(d[1].target_sample == 1500u && d[1].pos == 1u);
    assert(d[0].value_q15 == 16383u);
    assert(d[1].value_q15 == 8191u);
    assert(r.pos == 2u && r.mutation_count == 2u && r.advance_count == 2u);

    s.string.filter_attractor_every = APSIS_STRING_ATTRACTOR_1;
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    reset_mock(&m);
    m.value[0] = 0xffffffffu;
    assert(apsis_string_attractor_plan_pulse(&r, &s, 1u, targets,
                                              mock_random, &m, d) == 1u);
    assert(m.count == 1u); /* no second draw when lock prevents mutation */
    assert(r.mutation_count == 2u);
}

static void test_step_one_resync_cursor_only(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    ApsisStringAttractorDecision d[2];
    MockRandom m;
    const uint64_t targets[2] = {2000u, 0u};
    uint16_t cell0;
    enable_attractor(&s);
    s.clock.step_one_resync = 1u;
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    apsis_string_attractor_reset(&r, &s);
    cell0 = r.cell_q15[0];
    r.pos = 5u;
    reset_mock(&m);
    assert(apsis_string_attractor_plan_pulse(&r, &s, 0u, targets,
                                              mock_random, &m, d) == 1u);
    assert(d[0].pos == 0u && d[0].value_q15 == cell0);
    assert(r.pos == 1u && r.cell_q15[0] == cell0);
}

static void test_pulse_diag_semantics(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    ApsisStringAttractorDecision d[2];
    MockRandom m;
    const uint64_t targets[2] = {3000u, 0u};
    enable_attractor(&s);
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    apsis_string_attractor_reset(&r, &s);

    s.string.pulse_diag = APSIS_STRING_PULSE_FILTER;
    reset_mock(&m);
    assert(apsis_string_attractor_plan_pulse(&r, &s, 0u, targets,
                                              mock_random, &m, d) == 1u);
    assert(m.count == 1u && d[0].excite == 0u && r.advance_count == 1u);

    s.string.pulse_diag = APSIS_STRING_PULSE_SUSTAIN;
    reset_mock(&m);
    assert(apsis_string_attractor_plan_pulse(&r, &s, 1u, targets,
                                              mock_random, &m, d) == 0u);
    assert(m.count == 0u && r.advance_count == 1u);

    s.string.pulse_diag = APSIS_STRING_PULSE_EXCITE;
    reset_mock(&m);
    assert(apsis_string_attractor_plan_pulse(&r, &s, 2u, targets,
                                              mock_random, &m, d) == 1u);
    assert(d[0].excite == ((d[0].gate && s.string.sound) ? 1u : 0u));
}

static void test_planning_time_routes_and_sound_gate(void)
{
    ApsisState s;
    ApsisStringAttractorRuntime r;
    ApsisStringAttractorDecision d[2];
    MockRandom m;
    const uint64_t targets[2] = {4000u, 0u};
    enable_attractor(&s);
    s.string.filter_attractor_lock_q15 = APSIS_Q15_ONE;
    s.string.filter_attractor_gates = 0xffffu;
    apsis_string_attractor_reset(&r, &s);

    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_ALTERNATE;
    reset_mock(&m);
    apsis_string_attractor_plan_pulse(&r, &s, 0u, targets, mock_random, &m, d);
    assert(d[0].pos == 0u && d[0].route_mask == APSIS_STRING_ATTRACTOR_ROUTE_LEFT);
    assert(d[0].excite == 1u && d[0].base_level_q15 > 0u);
    reset_mock(&m);
    apsis_string_attractor_plan_pulse(&r, &s, 1u, targets, mock_random, &m, d);
    assert(d[0].pos == 1u && d[0].route_mask == APSIS_STRING_ATTRACTOR_ROUTE_RIGHT);

    s.string.attractor_target = APSIS_STRING_ATTRACTOR_BOTH;
    reset_mock(&m);
    apsis_string_attractor_plan_pulse(&r, &s, 2u, targets, mock_random, &m, d);
    assert(d[0].route_mask == (APSIS_STRING_ATTRACTOR_ROUTE_LEFT |
                                APSIS_STRING_ATTRACTOR_ROUTE_RIGHT));

    s.string.surface_mode = APSIS_STRING_CLASSIC;
    reset_mock(&m);
    apsis_string_attractor_plan_pulse(&r, &s, 3u, targets, mock_random, &m, d);
    assert(d[0].route_mask == APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC);

    s.string.sound = 0u;
    reset_mock(&m);
    apsis_string_attractor_plan_pulse(&r, &s, 4u, targets, mock_random, &m, d);
    assert(d[0].excite == 0u); /* S&H still advances while voice plan is skipped */
}

int main(void)
{
    test_default_target_and_seed_vector();
    test_seed_length_sync();
    test_rate_mapping();
    test_half_rate_and_lazy_mutation_draws();
    test_step_one_resync_cursor_only();
    test_pulse_diag_semantics();
    test_planning_time_routes_and_sound_gate();
    puts("PASS: APSIS M4c source-derived pure String Attractor planner");
    return 0;
}
