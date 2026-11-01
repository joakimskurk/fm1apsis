#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_space_target.h"
#include "apsis_transport.h"
#include "p11h_probe.h"

ApsisP11hProbe apsis_p11h_probe;

static void probe_reset(void)
{
    memset(&apsis_p11h_probe, 0, sizeof(apsis_p11h_probe));
}

static void run_cathedral(uint32_t blocks, uint32_t frames)
{
    ApsisSpaceTarget target;
    ApsisSpaceState space;
    int32_t out[APSIS_SPACE_ROOM_BLOCK_MAX * 2u];
    uint32_t b;

    memset(&target, 0, sizeof(target));
    memset(&space, 0, sizeof(space));
    space.mix_q15 = APSIS_Q15_MILLI(500);
    space.decay_ms = 8000u;
    space.predelay_ms = 40u;
    space.damping_hz = 4500u;
    space.echo_time_ms = 400u;
    space.echo_feedback_q15 = APSIS_Q15_MILLI(250);

    assert(frames <= APSIS_SPACE_ROOM_BLOCK_MAX);
    assert(apsis_space_target_bind(&target));

    for (b = 0u; b < blocks; ++b) {
        memset(out, 0, sizeof(out));
        apsis_space_target_begin_block(&target, frames);
        apsis_space_target_render(&target, &space, out, frames);
    }
}

static uint64_t rounded_next(const ApsisTransport *t)
{
    return t->next_sample + (t->next_frac_q32 >= 0x80000000u ? 1u : 0u);
}

static void drain_queue(ApsisEventQueue *q)
{
    ApsisTransportEvent e;
    while (apsis_event_queue_pop(q, &e)) { }
}

static void run_max_rate_transport(uint64_t total_frames)
{
    ApsisState state;
    ApsisTransport transport;
    ApsisEventQueue queue;

    memset(&state, 0, sizeof(state));
    state.clock.n = 12u;
    state.clock.bpm = 180u;
    state.clock.division_num = 1u;
    state.clock.division_den = 4u;
    state.clock.main_mask = 0x0fffu;
    state.clock.inner_mask = 0x0fffu;
    state.clock.snap_mask = 0x0fffu;
    state.clock.sound = 1u;
    state.spiral.attractor_rate = 4u;
    state.string.sound = 1u;
    state.string.rhythm = APSIS_STRING_RHYTHM_BOW;
    state.string.rhythm_every = 1u;
    state.string.pulse_diag = APSIS_STRING_PULSE_FULL;
    state.bloom.sound = 1u;
    state.bloom.cycles = 1u;
    state.bloom.rate = 4u;
    state.bloom.trigger_step = 0u;

    apsis_event_queue_reset(&queue);
    apsis_transport_reset(&transport, 44100u, 0u);
    apsis_transport_start(&transport);

    while (rounded_next(&transport) < total_frames) {
        uint8_t added = apsis_transport_schedule_next(
            &transport, &state, &queue, 0x41505349u);
        assert(added > 0u);
        drain_queue(&queue);
    }
}

int main(void)
{
    const uint32_t blocks = 256u;
    const uint32_t frames = 256u;
    const uint64_t output_frames = (uint64_t)blocks * frames;
    ApsisP11hProbe cathedral;
    ApsisP11hProbe transport;

    probe_reset();
    run_cathedral(blocks, frames);
    cathedral = apsis_p11h_probe;

    assert(cathedral.target_render_calls == 256u);
    assert(cathedral.output_frames == 65536u);
    assert(cathedral.room_frames == 16384u);
    assert(cathedral.room_render_calls == 256u);
    assert(cathedral.room_frame_iterations == 16384u);
    assert(cathedral.smooth_param_steps == 16384u);
    assert(cathedral.delay_read_calls == 196608u);
    assert(cathedral.stereo_write_calls == 32768u);
    assert(cathedral.room_biquad_calls == 131072u);
    assert(cathedral.room_mul_q15_calls == 294912u);
    assert(cathedral.fdn_render_calls == 256u);
    assert(cathedral.fdn_frames == 16384u);
    assert(cathedral.fdn_line_read_calls == 131072u);
    assert(cathedral.fdn_line_write_calls == 131072u);
    assert(cathedral.fdn_mul_q15_calls == 655360u);

    probe_reset();
    run_max_rate_transport(output_frames);
    transport = apsis_p11h_probe;

    /* Fastest legal Clock interval: 180 BPM, 1/4 division => 3,675 samples.
     * Over the same 65,536 output frames, pulses are scheduled at sample 0
     * through 62,475: exactly 18 schedule calls. */
    assert(transport.transport_schedule_calls == 18u);
    assert(transport.transport_events_added > transport.transport_schedule_calls);
    assert(cathedral.room_frame_iterations /
           transport.transport_schedule_calls > 900u);

    printf("P11H cathedral blocks=%u output_frames=%llu room_frames=%llu "
           "delay_reads=%llu biquads=%llu room_mul=%llu fdn_frames=%llu "
           "fdn_line_reads=%llu fdn_line_writes=%llu fdn_mul=%llu\n",
           blocks,
           (unsigned long long)cathedral.output_frames,
           (unsigned long long)cathedral.room_frame_iterations,
           (unsigned long long)cathedral.delay_read_calls,
           (unsigned long long)cathedral.room_biquad_calls,
           (unsigned long long)cathedral.room_mul_q15_calls,
           (unsigned long long)cathedral.fdn_frames,
           (unsigned long long)cathedral.fdn_line_read_calls,
           (unsigned long long)cathedral.fdn_line_write_calls,
           (unsigned long long)cathedral.fdn_mul_q15_calls);
    printf("P11H transport same_frames=%llu schedule_calls=%llu events_added=%llu "
           "room_frames_per_maxrate_schedule=%llu\n",
           (unsigned long long)output_frames,
           (unsigned long long)transport.transport_schedule_calls,
           (unsigned long long)transport.transport_events_added,
           (unsigned long long)(cathedral.room_frame_iterations /
                                transport.transport_schedule_calls));
    puts("PASS: P11H deterministic dynamic workload census");
    return 0;
}
