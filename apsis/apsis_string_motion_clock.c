#include "apsis_string_motion_clock.h"

static void p10c2_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint32_t p10c2_interval_samples(uint64_t interval_q32)
{
    uint64_t rounded = (interval_q32 + 0x80000000ull) >> 32;
    if (!rounded) return 1u;
    return rounded > 0xffffffffull ? 0xffffffffu : (uint32_t)rounded;
}

static void p10c2_add(ApsisStringMotionPulseBatch *out,
                      uint64_t sample,
                      uint64_t interval_q32,
                      uint32_t absolute_pulse,
                      uint8_t step)
{
    ApsisStringMotionPulse *p;
    if (!out) return;
    if (out->count >= APSIS_STRING_MOTION_PULSE_BATCH_MAX) {
        out->overflowed = 1u;
        return;
    }
    p = &out->pulse[out->count++];
    p->sample = sample;
    p->interval_samples = p10c2_interval_samples(interval_q32);
    p->absolute_pulse = absolute_pulse;
    p->step = step;
}

void apsis_string_motion_collect_pulses(const ApsisRuntime *runtime,
                                        const ApsisState *state,
                                        uint32_t frames,
                                        ApsisStringMotionPulseBatch *out)
{
    uint64_t end;
    uint16_t i;
    if (!out) return;
    p10c2_zero(out, (uint32_t)sizeof(*out));
    if (!runtime || !state || !frames || !runtime->transport.running) return;

    end = runtime->cursor_sample + frames;
    if (end < runtime->cursor_sample) end = ~(uint64_t)0;

    /* Same first-pulse condition used by the established Clock DSP observer.
     * At this instant pulse 0 is not in the queue yet, but it is authoritative
     * and must be observable by Motion before runtime scheduling advances. */
    if (runtime->queue.count == 0u &&
        runtime->transport.absolute_pulse == 0u &&
        runtime->transport.cycle_pos == 0u &&
        runtime->transport.next_sample == runtime->cursor_sample) {
        uint64_t interval = apsis_transport_interval_q32(
            &state->clock, runtime->transport.sample_rate, 0u);
        p10c2_add(out, runtime->cursor_sample, interval, 0u, 0u);
        return;
    }

    for (i = 0u; i < runtime->queue.count; ++i) {
        const ApsisTransportEvent *event = &runtime->queue.events[i];
        if (event->sample >= end) break;
        if (event->type != APSIS_EVENT_PLAYHEAD) continue;
        p10c2_add(out, event->sample, event->interval_q32,
                  event->absolute_pulse, event->clock_step);
    }
}
