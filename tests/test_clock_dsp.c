#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apsis_clock_dsp.h"

static uint32_t samples_ms(uint32_t ms)
{
    return (APSIS_CLOCK_DSP_RATE * ms + 500u) / 1000u;
}

static uint32_t samples_tenth_ms(uint32_t tenths)
{
    return (APSIS_CLOCK_DSP_RATE * tenths + 5000u) / 10000u;
}

static uint32_t phase_inc(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint32_t mul_q15(uint32_t a, uint32_t b)
{
    return (uint32_t)(((uint64_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint16_t expected_peak(uint16_t milli, uint16_t source, uint16_t scale)
{
    uint32_t g = APSIS_Q15_MILLI(milli);
    g = mul_q15(g, source);
    g = mul_q15(g, scale);
    if (g < APSIS_Q15_MILLI(1)) g = APSIS_Q15_MILLI(1);
    return (uint16_t)g;
}

static uint32_t fnv_i32(const int32_t *p, uint32_t n)
{
    uint32_t h = 2166136261u, i;
    for (i = 0u; i < n; ++i) {
        uint32_t v = (uint32_t)p[i];
        h ^= v & 0xffu; h *= 16777619u;
        h ^= (v >> 8) & 0xffu; h *= 16777619u;
        h ^= (v >> 16) & 0xffu; h *= 16777619u;
        h ^= (v >> 24) & 0xffu; h *= 16777619u;
    }
    return h;
}

static int32_t peak_abs(const int32_t *p, uint32_t n)
{
    uint32_t i;
    int32_t peak = 0;
    for (i = 0u; i < n; ++i) {
        int32_t v = p[i] < 0 ? -p[i] : p[i];
        if (v > peak) peak = v;
    }
    return peak;
}

static ApsisTransportEvent event_at(uint8_t type, uint64_t sample, uint8_t flags)
{
    ApsisTransportEvent e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.sample = sample;
    e.sub_count = 1u;
    e.flags = flags;
    return e;
}

static void test_state_and_bootstrap(void)
{
    ApsisState s;
    ApsisRuntime r;
    ApsisClockEventBatch batch;
    uint32_t random_before;

    apsis_state_default(&s);
    assert(s.clock.voice == APSIS_CLOCK_STONE);
    assert(APSIS_CLOCK_WOOD == APSIS_CLOCK_STONE + 1);
    assert(APSIS_CLOCK_GLASS == APSIS_CLOCK_STONE + 2);
    assert(APSIS_CLOCK_SOFT == APSIS_CLOCK_STONE + 3);
    assert(s.clock.pitch_centi == 100u);

    apsis_runtime_reset(&r, APSIS_CLOCK_DSP_RATE, 0u, 0x12345678u);
    apsis_runtime_start(&r);
    random_before = r.random_state;
    apsis_clock_collect_block(&r, &s, 256u, &batch);
    assert(batch.count == 1u && !batch.overflowed);
    assert(batch.event[0].type == APSIS_EVENT_CLOCK_MAIN);
    assert(batch.event[0].sample == 0u);
    assert((batch.event[0].flags & APSIS_EVENT_FLAG_ACCENT) != 0u);
    assert(r.random_state == random_before);
    assert(r.transport.absolute_pulse == 0u && r.queue.count == 0u);

    s.clock.snap_mask = 1u;
    apsis_clock_collect_block(&r, &s, 256u, &batch);
    assert(batch.count == 2u);
    assert(batch.event[1].type == APSIS_EVENT_CLOCK_SNAP);
    assert(batch.event[1].sample == samples_ms(4u));
    assert(r.random_state == random_before);

    s.clock.snap_mask = 0u;
    s.clock.inner_mask = 1u;
    s.clock.bpm = 180u;
    s.clock.division_num = 1u;
    s.clock.division_den = 4u;
    apsis_clock_collect_block(&r, &s, 2000u, &batch);
    assert(batch.count == 2u);
    assert(batch.event[1].type == APSIS_EVENT_CLOCK_INNER);
    assert(batch.event[1].sample >= 1837u && batch.event[1].sample <= 1838u);
    assert(r.random_state == random_before);
}

static void test_profiles(void)
{
    static const uint16_t start_hz[4][3] = {
        {118u,118u,168u}, {245u,245u,350u}, {760u,760u,1080u}, {88u,88u,132u}
    };
    static const uint16_t end_hz[4][3] = {
        {67u,67u,116u}, {160u,160u,245u}, {520u,520u,790u}, {58u,58u,92u}
    };
    static const uint16_t dur_ms[4] = {120u,75u,280u,170u};
    static const uint16_t gain_milli[4][3] = {
        {84u,58u,18u}, {70u,50u,14u}, {60u,43u,12u}, {56u,41u,11u}
    };
    ApsisState s;
    uint8_t voice;

    for (voice = APSIS_CLOCK_STONE; voice <= APSIS_CLOCK_SOFT; ++voice) {
        ApsisClockDsp dsp;
        ApsisTransportEvent e = event_at(APSIS_EVENT_CLOCK_MAIN, 0u,
                                         APSIS_EVENT_FLAG_ACCENT);
        uint32_t pitch_ms = (dur_ms[voice] * 65u + 50u) / 100u;
        apsis_state_default(&s);
        s.clock.voice = voice;
        s.clock.pitch_centi = 100u;
        apsis_clock_dsp_reset(&dsp);
        assert(apsis_clock_dsp_trigger_event(&dsp, &s, &e, 0u) ==
               (voice == APSIS_CLOCK_GLASS ? 2u : 1u));
        assert(dsp.voice[0].phase_inc == phase_inc((uint32_t)start_hz[voice][0] * 1000u));
        assert(dsp.voice[0].end_phase_inc == phase_inc((uint32_t)end_hz[voice][0] * 1000u));
        assert(dsp.voice[0].attack_samples == samples_tenth_ms(25u));
        assert(dsp.voice[0].decay_end_samples == samples_ms(dur_ms[voice]));
        assert(dsp.voice[0].life_samples == samples_ms(dur_ms[voice] + 35u));
        if (pitch_ms > 90u) pitch_ms = 90u;
        assert(dsp.voice[0].pitch_ramp_samples == samples_ms(pitch_ms));
        assert((dsp.voice[0].peak_gain_q23 >> 8) ==
               expected_peak(gain_milli[voice][0], s.clock.level_q15, APSIS_Q15_ONE));
        if (voice == APSIS_CLOCK_WOOD) {
            assert(dsp.voice[0].waveform == APSIS_CLOCK_DSP_TRIANGLE);
            assert(dsp.voice[0].filter_count == 1u);
        } else {
            assert(dsp.voice[0].waveform == APSIS_CLOCK_DSP_SINE);
        }
        if (voice == APSIS_CLOCK_GLASS) {
            assert(dsp.voice[1].life_samples == samples_ms(450u));
            assert(dsp.voice[1].decay_end_samples == samples_ms(420u));
            assert(dsp.voice[1].phase_inc == dsp.voice[1].end_phase_inc);
        }
    }

    apsis_state_default(&s);
    s.clock.pitch_centi = 50u;
    {
        ApsisClockDsp dsp;
        ApsisTransportEvent e = event_at(APSIS_EVENT_CLOCK_MAIN, 0u, 0u);
        apsis_clock_dsp_reset(&dsp);
        assert(apsis_clock_dsp_trigger_event(&dsp, &s, &e, 0u) == 1u);
        assert(dsp.voice[0].phase_inc == phase_inc(59000u));
    }
    s.clock.pitch_centi = 200u;
    {
        ApsisClockDsp dsp;
        ApsisTransportEvent e = event_at(APSIS_EVENT_CLOCK_MAIN, 0u, 0u);
        apsis_clock_dsp_reset(&dsp);
        assert(apsis_clock_dsp_trigger_event(&dsp, &s, &e, 0u) == 1u);
        assert(dsp.voice[0].phase_inc == phase_inc(236000u));
    }
}

static void test_inner_ghost(void)
{
    ApsisState s;
    ApsisClockDsp inner, ghost;
    ApsisTransportEvent ei = event_at(APSIS_EVENT_CLOCK_INNER, 0u, 0u);
    ApsisTransportEvent eg = event_at(APSIS_EVENT_CLOCK_GHOST, 0u, 0u);
    apsis_state_default(&s);
    apsis_clock_dsp_reset(&inner);
    apsis_clock_dsp_reset(&ghost);
    assert(apsis_clock_dsp_trigger_event(&inner, &s, &ei, 0u) == 1u);
    assert(apsis_clock_dsp_trigger_event(&ghost, &s, &eg, 0u) == 1u);
    assert(inner.voice[0].phase_inc == phase_inc(118000u));
    assert(ghost.voice[0].phase_inc == phase_inc(168000u));
    assert((inner.voice[0].peak_gain_q23 >> 8) ==
           expected_peak(58u, s.clock.level_q15, s.clock.inner_level_q15));
    assert((ghost.voice[0].peak_gain_q23 >> 8) ==
           expected_peak(18u, s.clock.level_q15, APSIS_Q15_MILLI(550)));
}

static uint32_t render_one(uint8_t type, uint8_t voice, int32_t *out,
                           uint32_t frames, int32_t *peak)
{
    ApsisState s;
    ApsisClockDsp dsp;
    ApsisTransportEvent e = event_at(type, 0u,
        type == APSIS_EVENT_CLOCK_MAIN ? APSIS_EVENT_FLAG_ACCENT : 0u);
    apsis_state_default(&s);
    s.clock.voice = voice;
    apsis_clock_dsp_reset(&dsp);
    memset(out, 0, (size_t)frames * 2u * sizeof(*out));
    (void)apsis_clock_dsp_trigger_event(&dsp, &s, &e, 0u);
    apsis_clock_dsp_render(&dsp, out, frames);
    if (peak) *peak = peak_abs(out, frames * 2u);
    return fnv_i32(out, frames * 2u);
}

static void test_deterministic_references(void)
{
    const uint32_t frames = APSIS_CLOCK_DSP_RATE;
    int32_t *a = (int32_t *)calloc((size_t)frames * 2u, sizeof(*a));
    int32_t *b = (int32_t *)calloc((size_t)frames * 2u, sizeof(*b));
    uint32_t stone, glass, snap_a, snap_b;
    int32_t stone_peak, glass_peak, snap_peak;
    assert(a && b);
    stone = render_one(APSIS_EVENT_CLOCK_MAIN, APSIS_CLOCK_STONE, a, frames, &stone_peak);
    glass = render_one(APSIS_EVENT_CLOCK_MAIN, APSIS_CLOCK_GLASS, a, frames, &glass_peak);
    snap_a = render_one(APSIS_EVENT_CLOCK_SNAP, APSIS_CLOCK_STONE, a, frames, &snap_peak);
    snap_b = render_one(APSIS_EVENT_CLOCK_SNAP, APSIS_CLOCK_STONE, b, frames, 0);
    assert(snap_a == snap_b);
    assert(memcmp(a, b, (size_t)frames * 2u * sizeof(*a)) == 0);
    assert(stone_peak == 2081 && stone == 0xedcc4a81u);
    assert(glass_peak == 1880 && glass == 0x0adf9829u);
    assert(snap_peak == 738 && snap_a == 0x04bc514au);
    printf("PASS: M4e frozen Clock refs STONE=2081/edcc4a81 GLASS=1880/0adf9829 SNAP=738/04bc514a\n");
    free(a); free(b);
}

static void append_event(ApsisTransportEvent *events, uint16_t *count,
                         uint8_t type, uint64_t sample, uint8_t flags)
{
    assert(*count < 256u);
    events[*count] = event_at(type, sample, flags);
    (*count)++;
}

static void test_worst_case_pool(void)
{
    ApsisState s;
    ApsisClockDsp dsp;
    ApsisTransportEvent events[256];
    uint16_t count = 0u, ix = 0u;
    uint64_t pulse = 0u;
    uint32_t p;
    const uint32_t total = APSIS_CLOCK_DSP_RATE * 2u;
    int32_t out[512];

    apsis_state_default(&s);
    s.clock.voice = APSIS_CLOCK_GLASS;
    s.clock.pitch_centi = 200u;
    s.clock.level_q15 = APSIS_Q15_MILLI(1250);
    s.clock.inner_level_q15 = APSIS_Q15_MILLI(1250);
    s.clock.snap_level_q15 = APSIS_Q15_MILLI(1250);
    s.clock.snap_tone_hz = 8500u;
    apsis_clock_dsp_reset(&dsp);

    /* 180 BPM, 1/16, swing=.35. Every pulse is treated as a grouping accent,
     * every layer is enabled, and every eligible third-pulse Ghost is forced
     * on. This is at least as dense as any legal stochastic realization. */
    for (p = 0u; pulse < total && count + 4u < 256u; ++p) {
        uint32_t interval = (p & 1u) ? 4961u : 2389u;
        append_event(events, &count, APSIS_EVENT_CLOCK_MAIN, pulse,
                     APSIS_EVENT_FLAG_ACCENT);
        append_event(events, &count, APSIS_EVENT_CLOCK_SNAP, pulse + samples_ms(4u), 0u);
        append_event(events, &count, APSIS_EVENT_CLOCK_INNER, pulse + interval / 2u, 0u);
        if (p % 3u == 2u)
            append_event(events, &count, APSIS_EVENT_CLOCK_GHOST, pulse + interval / 2u, 0u);
        pulse += interval;
    }

    for (p = 0u; p < total + APSIS_CLOCK_DSP_RATE; p += 256u) {
        uint32_t frames = 256u;
        uint64_t end = (uint64_t)p + frames;
        if ((uint64_t)p + frames > (uint64_t)total + APSIS_CLOCK_DSP_RATE)
            frames = (uint32_t)((uint64_t)total + APSIS_CLOCK_DSP_RATE - p);
        memset(out, 0, sizeof(out));
        while (ix < count && events[ix].sample < end) {
            (void)apsis_clock_dsp_trigger_event(&dsp, &s, &events[ix], p);
            ix++;
        }
        apsis_clock_dsp_render(&dsp, out, frames);
    }
    assert(dsp.drop_count == 0u);
    assert(dsp.high_water == 23u);
    assert(APSIS_CLOCK_DSP_VOICES == 32u);
    printf("PASS: M4e worst Clock pool high-water=23/32 triggers=%u drops=0\n",
           (unsigned)dsp.trigger_count);
}

int main(void)
{
    test_state_and_bootstrap();
    test_profiles();
    test_inner_ghost();
    test_deterministic_references();
    test_worst_case_pool();
    puts("PASS: APSIS M4e source-derived Clock dry engine");
    return 0;
}
