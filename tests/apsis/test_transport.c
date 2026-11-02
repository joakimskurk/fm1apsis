#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_transport.h"

static uint8_t collect(ApsisEventQueue *q, ApsisTransportEvent *out, uint8_t cap)
{
    uint8_t n = 0u;
    while (n < cap && apsis_event_queue_pop(q, &out[n])) n++;
    return n;
}

static int find_type(const ApsisTransportEvent *events, uint8_t n, uint8_t type)
{
    uint8_t i;
    for (i = 0u; i < n; ++i) if (events[i].type == type) return (int)i;
    return -1;
}

static void assert_sorted(const ApsisTransportEvent *events, uint8_t n)
{
    uint8_t i;
    for (i = 1u; i < n; ++i) {
        assert(events[i - 1u].sample <= events[i].sample);
        if (events[i - 1u].sample == events[i].sample) {
            if (events[i - 1u].type == events[i].type)
                assert(events[i - 1u].serial < events[i].serial);
            else
                assert(events[i - 1u].type < events[i].type);
        }
    }
}

static void test_default_pulse_grid(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent e[16];
    uint8_t n;

    apsis_state_default(&s);
    assert(s.version == APSIS_STATE_VERSION);
    assert(s.string.rhythm_trigger == APSIS_STRING_TRIGGER_EVERY);
    assert(s.string.rhythm_mask == 0x249u);
    assert(s.string.filter_mask == 0x249u);
    assert(s.string.filter_attractor_every == APSIS_STRING_ATTRACTOR_1);
    assert(s.string.pulse_diag == APSIS_STRING_PULSE_FULL);

    assert(apsis_transport_interval_q32(&s.clock, 44100u, 0u) == ((uint64_t)18375u << 32));
    assert(apsis_transport_interval_q32(&s.clock, 44100u, 1u) == ((uint64_t)18375u << 32));

    apsis_transport_reset(&t, 44100u, 0u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) == 7u);
    assert(t.absolute_pulse == 1u && t.cycle_pos == 1u);
    assert(t.next_sample == 18375u && t.next_frac_q32 == 0u);

    n = collect(&q, e, 16u);
    assert(n == 7u);
    assert_sorted(e, n);
    assert(e[0].sample == 0u && e[0].type == APSIS_EVENT_STRING_FILTER_CLOCK);
    assert(e[1].sample == 0u && e[1].type == APSIS_EVENT_PLAYHEAD);
    assert(e[2].sample == 0u && e[2].type == APSIS_EVENT_MOTHER_PULSE);
    assert(e[3].sample == 0u && e[3].type == APSIS_EVENT_CLOCK_MAIN);
    assert(e[3].flags & APSIS_EVENT_FLAG_ACCENT);
    assert(e[4].sample == 0u && e[4].type == APSIS_EVENT_SPIRAL_TICK && e[4].sub_index == 0u);
    assert(e[5].sample == 0u && e[5].type == APSIS_EVENT_CHAIN_PULSE);
    assert(e[6].sample == 9188u && e[6].type == APSIS_EVENT_SPIRAL_TICK && e[6].sub_index == 1u);
}

static void test_step_one_resync_and_gravity_flag(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent e[16];
    uint8_t n;
    apsis_state_default(&s);
    s.clock.step_one_resync = 1u;
    apsis_transport_reset(&t, 44100u, 1000u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) == 8u);
    n = collect(&q, e, 16u);
    assert(n == 8u);
    assert(e[0].sample == 1000u && e[0].type == APSIS_EVENT_STEP_ONE_RESYNC);
    assert(e[0].flags & APSIS_EVENT_FLAG_STEP_ONE);
    assert(e[0].flags & APSIS_EVENT_FLAG_GRAVITY_GRID);
    assert(e[1].type == APSIS_EVENT_STRING_FILTER_CLOCK);
}

static void test_swing_has_no_pair_drift(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    uint64_t a, b;
    apsis_state_default(&s);
    s.clock.swing_q15 = APSIS_Q15_MILLI(100);
    a = apsis_transport_interval_q32(&s.clock, 44100u, 0u);
    b = apsis_transport_interval_q32(&s.clock, 44100u, 1u);
    assert(a + b == ((uint64_t)36750u << 32));
    assert(apsis_transport_round_sample(a) >= 16537u && apsis_transport_round_sample(a) <= 16538u);
    assert(apsis_transport_round_sample(b) >= 20212u && apsis_transport_round_sample(b) <= 20213u);

    apsis_transport_reset(&t, 44100u, 0u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) > 0u);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) > 0u);
    assert(t.next_sample == 36750u && t.next_frac_q32 == 0u);
}

static void test_string_rhythm_clock_offsets(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent e[16];
    uint8_t n;
    int ix;
    apsis_state_default(&s);
    s.string.rhythm = APSIS_STRING_RHYTHM_PIZZ;
    s.string.rhythm_trigger = APSIS_STRING_TRIGGER_SEQUENCE;
    s.string.rhythm_mask = 1u << 0;
    apsis_transport_reset(&t, 44100u, 0u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) == 8u);
    n = collect(&q, e, 16u);
    ix = find_type(e, n, APSIS_EVENT_STRING_RHYTHM);
    assert(ix >= 0 && e[ix].sample == 1147u);

    apsis_state_default(&s);
    s.string.rhythm = APSIS_STRING_RHYTHM_BOW;
    s.string.rhythm_every = 2u;
    s.string.pulse_diag = APSIS_STRING_PULSE_SUSTAIN;
    apsis_transport_reset(&t, 44100u, 0u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    apsis_transport_schedule_next(&t, &s, &q, 0u);
    n = collect(&q, e, 16u);
    assert(find_type(e, n, APSIS_EVENT_STRING_RHYTHM) < 0);
}

static void test_clock_secondary_offsets_and_ghost(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent e[16];
    uint8_t n;
    int snap, inner, ghost;
    apsis_state_default(&s);
    s.clock.snap_mask = 1u;
    s.clock.inner_mask = 1u;
    s.clock.ghost_q15 = APSIS_Q15_ONE;
    apsis_transport_reset(&t, 44100u, 0u);
    t.melody_step = 2u;
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0xFFFFFFFFu) == 10u);
    n = collect(&q, e, 16u);
    assert_sorted(e, n);
    snap = find_type(e, n, APSIS_EVENT_CLOCK_SNAP);
    inner = find_type(e, n, APSIS_EVENT_CLOCK_INNER);
    ghost = find_type(e, n, APSIS_EVENT_CLOCK_GHOST);
    assert(snap >= 0 && e[snap].sample == 176u);
    assert(inner >= 0 && e[inner].sample == 9188u);
    assert(ghost >= 0 && e[ghost].sample == 9188u);
    assert(inner < ghost);
}

static void test_bloom_occurrence_and_delay(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent e[16];
    uint8_t n;
    int bloom;
    apsis_state_default(&s);
    apsis_transport_reset(&t, 44100u, 100u);
    t.absolute_pulse = 24u;
    t.cycle_pos = 0u;
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    apsis_transport_schedule_next(&t, &s, &q, 0u);
    n = collect(&q, e, 16u);
    bloom = find_type(e, n, APSIS_EVENT_BLOOM);
    assert(bloom >= 0 && e[bloom].sample == 2746u); /* 100 + 60 ms */

    apsis_state_default(&s);
    s.bloom.rate = 2u;
    apsis_transport_reset(&t, 44100u, 0u);
    t.absolute_pulse = 30u;
    t.cycle_pos = 6u;
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    apsis_transport_schedule_next(&t, &s, &q, 0u);
    n = collect(&q, e, 16u);
    assert(find_type(e, n, APSIS_EVENT_BLOOM) >= 0);
}

static void test_gravity_boundary_prediction(void)
{
    ApsisState s;
    ApsisTransport t;
    uint64_t at;
    apsis_state_default(&s);
    assert(apsis_transport_gravity_division(2u, 12u) == 2u);
    assert(apsis_transport_gravity_division(2u, 11u) == 1u);
    assert(apsis_transport_gravity_division(4u, 12u) == 4u);
    assert(apsis_transport_gravity_division(5u, 12u) == 2u);
    assert(apsis_transport_is_gravity_boundary(0u, 12u, 2u) == 1u);
    assert(apsis_transport_is_gravity_boundary(6u, 12u, 2u) == 1u);
    assert(apsis_transport_is_gravity_boundary(5u, 12u, 2u) == 0u);

    apsis_transport_reset(&t, 44100u, 100u);
    t.cycle_pos = 1u;
    at = apsis_transport_next_gravity_boundary(&t, &s.clock, 2u, 100u, 0u);
    assert(at == 91975u); /* five default 1/8 pulses to step 6 */
}

static void test_long_running_absolute_sample(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    const ApsisTransportEvent *e;
    const uint64_t start = 10000000000ull;
    apsis_state_default(&s);
    apsis_transport_reset(&t, 44100u, start);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    assert(apsis_transport_schedule_next(&t, &s, &q, 0u) == 7u);
    e = apsis_event_queue_peek(&q);
    assert(e && e->sample == start);
    assert(t.next_sample == start + 18375u);
}

static void test_many_pulses_heap_order(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    ApsisTransportEvent prev = {0}, cur;
    uint8_t have_prev = 0u;
    uint8_t i;
    apsis_state_default(&s);
    s.clock.snap_mask = 0x0FFFu;
    s.clock.inner_mask = 0x0FFFu;
    s.spiral.attractor_rate = 4u;
    apsis_transport_reset(&t, 44100u, 0u);
    apsis_event_queue_reset(&q);
    apsis_transport_start(&t);
    for (i = 0u; i < 12u; ++i) assert(apsis_transport_schedule_next(&t, &s, &q, 0u) > 0u);
    assert(q.overflowed == 0u);
    while (apsis_event_queue_pop(&q, &cur)) {
        if (have_prev) {
            assert(prev.sample <= cur.sample);
            if (prev.sample == cur.sample) {
                if (prev.type == cur.type) assert(prev.serial < cur.serial);
                else assert(prev.type < cur.type);
            }
        }
        prev = cur;
        have_prev = 1u;
    }
}

int main(void)
{
    test_default_pulse_grid();
    test_step_one_resync_and_gravity_flag();
    test_swing_has_no_pair_drift();
    test_string_rhythm_clock_offsets();
    test_clock_secondary_offsets_and_ghost();
    test_bloom_occurrence_and_delay();
    test_gravity_boundary_prediction();
    test_long_running_absolute_sample();
    test_many_pulses_heap_order();
    puts("PASS: APSIS sample-referenced transport/event grid");
    return 0;
}
