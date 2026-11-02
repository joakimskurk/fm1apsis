#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_runtime.h"
#include "apsis_bloom_audio_runtime.h"
#include "apsis_late_binding.h"

#define P11E_RATE 44100u
#define P11E_MAX_SWING APSIS_Q15_MILLI(350)

static uint64_t ms_samples(uint32_t ms)
{
    return ((uint64_t)P11E_RATE * ms + 500u) / 1000u;
}

static uint64_t interval_span_q32(ApsisClockState *clock,
                                  uint32_t first_pulse,
                                  uint32_t interval_count)
{
    uint64_t total = 0u;
    uint32_t i;
    for (i = 0u; i < interval_count; ++i)
        total += apsis_transport_interval_q32(clock, P11E_RATE,
                                              first_pulse + i);
    return total;
}

static void prove_one_bloom_max_per_pulse(void)
{
    ApsisState s;
    ApsisTransport t;
    ApsisEventQueue q;
    uint32_t pulse;
    uint32_t bloom_events = 0u;

    apsis_state_default(&s);
    s.string.sound = 0u;
    s.spiral.sound = 0u;
    s.clock.sound = 0u;
    s.bloom.sound = 1u;
    s.clock.n = 3u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    s.clock.swing_q15 = P11E_MAX_SWING;
    s.bloom.cycles = 1u;
    s.bloom.rate = 4u;
    s.bloom.trigger_step = 0u;
    s.bloom.delay_ms = 0u;

    apsis_transport_reset(&t, P11E_RATE, 0u);
    apsis_transport_start(&t);
    for (pulse = 0u; pulse < 96u; ++pulse) {
        uint16_t i;
        uint8_t this_bloom = 0u;
        apsis_event_queue_reset(&q);
        assert(apsis_transport_schedule_next(&t, &s, &q, 0u) != 0u);
        for (i = 0u; i < q.count; ++i) {
            if (q.events[i].type == APSIS_EVENT_BLOOM) {
                ++this_bloom;
                ++bloom_events;
            }
        }
        assert(this_bloom <= 1u);
        if (pulse >= 3u) assert(this_bloom == 1u);
    }
    assert(bloom_events == 93u);
}

int main(void)
{
    ApsisState s;
    ApsisClockState clock;
    uint64_t target = 1000000u;
    uint64_t occurrence;
    uint64_t last_voice_target;
    uint64_t last_bind;
    uint64_t lifetime_samples;
    uint64_t lifetime_q32;
    uint64_t min_44_interval_span = UINT64_MAX;
    uint64_t min_43_interval_span = UINT64_MAX;
    uint16_t swing;
    uint8_t parity;

    apsis_state_default(&s);
    clock = s.clock;
    clock.bpm = 180u;
    clock.division_num = 1u;
    clock.division_den = 4u;

    /* The runtime caps unfurl at 3600 ms. A final Bloom voice targets
     * target+3600 ms. Occurrence is target-45 ms and native binding is
     * voice_target-50 ms, so the longest live-slot interval is 3595 ms. */
    occurrence = apsis_late_bloom_occurrence_sample(target, P11E_RATE);
    last_voice_target = target + ms_samples(3600u);
    last_bind = apsis_late_bloom_bind_sample(occurrence, last_voice_target,
                                             P11E_RATE);
    assert(last_bind > occurrence);
    lifetime_samples = last_bind - occurrence;
    assert(lifetime_samples == 158540u);
    lifetime_q32 = lifetime_samples << 32;

    /* Exhaust every representable legal swing value at the fastest legal
     * pulse rate. Forty-four consecutive intervals are 22 complete swing
     * pairs; no starting parity or swing value can fit them inside the maximum
     * slot lifetime. Therefore 45 simultaneous Bloom occurrences are
     * impossible. Forty-three intervals can fit, so 44 remains reachable. */
    for (swing = 0u; swing <= P11E_MAX_SWING; ++swing) {
        clock.swing_q15 = swing;
        for (parity = 0u; parity < 2u; ++parity) {
            uint64_t span44 = interval_span_q32(&clock, parity, 44u);
            uint64_t span43 = interval_span_q32(&clock, parity, 43u);
            if (span44 < min_44_interval_span) min_44_interval_span = span44;
            if (span43 < min_43_interval_span) min_43_interval_span = span43;
        }
    }
    assert(min_44_interval_span > lifetime_q32);
    assert(min_43_interval_span < lifetime_q32);

    /* At n=3/rate=4/cycles=1 the transport actually emits one Bloom event per
     * pulse after the source's first-cycle guard, proving the density premise
     * rather than relying on a comment or a duplicated scheduler model. */
    prove_one_bloom_max_per_pulse();

    printf("P11E_BLOOM_BOUND proven_max=44 current_cap=%u lifetime_samples=%llu "
           "min_span_43_samples=%llu min_span_44_samples=%llu "
           "host_runtime_slot_bytes=%u host_audio_snapshot_bytes=%u\n",
           (unsigned)APSIS_RUNTIME_BLOOM_SLOTS,
           (unsigned long long)lifetime_samples,
           (unsigned long long)((min_43_interval_span + 0xffffffffull) >> 32),
           (unsigned long long)((min_44_interval_span + 0xffffffffull) >> 32),
           (unsigned)sizeof(ApsisRuntimeBloomSlot),
           (unsigned)sizeof(ApsisBloomAudioSnapshot));
    puts("PASS: P11E exact Bloom scheduler concurrency upper bound");
    return 0;
}
