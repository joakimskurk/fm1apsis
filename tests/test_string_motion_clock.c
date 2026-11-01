#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_runtime.h"
#include "apsis_string_motion_clock.h"

static void test_bootstrap_pulse_zero(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisStringMotionPulseBatch b;
    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 2048u, 1u);
    apsis_runtime_start(&r);
    assert(r.queue.count == 0u);
    assert(r.transport.next_sample == r.cursor_sample);
    apsis_string_motion_collect_pulses(&r, &s, 256u, &b);
    assert(b.count == 1u);
    assert(!b.overflowed);
    assert(b.pulse[0].sample == 2048u);
    assert(b.pulse[0].absolute_pulse == 0u);
    assert(b.pulse[0].step == 0u);
    assert(b.pulse[0].interval_samples > 0u);
    assert(r.queue.count == 0u); /* observer never schedules/mutates */
}

static void test_queue_filters_playhead_only(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisStringMotionPulseBatch b;
    ApsisTransportEvent e;
    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 1000u, 1u);
    r.transport.running = 1u;
    r.transport.absolute_pulse = 3u;
    r.transport.cycle_pos = 3u;
    r.transport.next_sample = 9000u;
    apsis_event_queue_reset(&r.queue);

    memset(&e, 0, sizeof(e));
    e.sample = 1100u;
    e.interval_q32 = (uint64_t)2000u << 32;
    e.absolute_pulse = 3u;
    e.clock_step = 3u;
    e.type = APSIS_EVENT_CLOCK_MAIN;
    r.queue.events[r.queue.count++] = e;

    e.sample = 1120u;
    e.type = APSIS_EVENT_PLAYHEAD;
    r.queue.events[r.queue.count++] = e;

    e.sample = 1400u; /* outside this 256-frame block */
    e.absolute_pulse = 4u;
    e.clock_step = 0u;
    e.type = APSIS_EVENT_PLAYHEAD;
    r.queue.events[r.queue.count++] = e;

    apsis_string_motion_collect_pulses(&r, &s, 256u, &b);
    assert(b.count == 1u);
    assert(b.pulse[0].sample == 1120u);
    assert(b.pulse[0].absolute_pulse == 3u);
    assert(b.pulse[0].step == 3u);
    assert(b.pulse[0].interval_samples == 2000u);
    assert(r.queue.count == 3u);
}

static void test_stopped_transport_is_inert(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisStringMotionPulseBatch b;
    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 0u, 1u);
    apsis_string_motion_collect_pulses(&r, &s, 256u, &b);
    assert(b.count == 0u);
    assert(!b.overflowed);
}

static void test_batch_overflow_is_bounded(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisStringMotionPulseBatch b;
    uint8_t i;
    apsis_state_default(&s);
    apsis_runtime_reset(&r, 44100u, 0u, 1u);
    r.transport.running = 1u;
    r.transport.absolute_pulse = 9u;
    r.transport.cycle_pos = 1u;
    r.transport.next_sample = 9999u;
    apsis_event_queue_reset(&r.queue);
    for (i = 0u; i < 6u; ++i) {
        ApsisTransportEvent *e = &r.queue.events[r.queue.count++];
        memset(e, 0, sizeof(*e));
        e->sample = i;
        e->interval_q32 = (uint64_t)1u << 32;
        e->absolute_pulse = i;
        e->clock_step = i;
        e->type = APSIS_EVENT_PLAYHEAD;
    }
    apsis_string_motion_collect_pulses(&r, &s, 256u, &b);
    assert(b.count == APSIS_STRING_MOTION_PULSE_BATCH_MAX);
    assert(b.overflowed);
}

int main(void)
{
    test_bootstrap_pulse_zero();
    test_queue_filters_playhead_only();
    test_stopped_transport_is_inert();
    test_batch_overflow_is_bounded();
    puts("PASS: Phase 10C2 shared-Clock passive Motion observer");
    return 0;
}
