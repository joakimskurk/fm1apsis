#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apsis_spiral_dsp.h"

#define TEST_FRAMES 44100u
#define STRESS_SECONDS 8u
#define STRESS_EVENT_CAP 2048u

typedef struct {
    uint64_t target_sample;
    uint64_t interval_q32;
} StressEvent;

static uint32_t fnv_u32(uint32_t h, uint32_t v)
{
    h ^= v;
    return h * 16777619u;
}

static uint32_t hash_audio(uint32_t h, const int32_t *out, uint32_t frames)
{
    uint32_t i;
    for (i = 0u; i < frames * 2u; ++i) h = fnv_u32(h, (uint32_t)out[i]);
    return h;
}

static uint32_t peak_audio(const int32_t *out, uint32_t frames)
{
    uint32_t i, peak = 0u;
    for (i = 0u; i < frames * 2u; ++i) {
        int32_t v = out[i];
        uint32_t a = (uint32_t)(v < 0 ? -v : v);
        if (a > peak) peak = a;
    }
    return peak;
}

static void make_binding(ApsisState *s, uint8_t voice, uint8_t cutoff_voice,
                         uint16_t energy, uint64_t interval_q32,
                         uint8_t rate, uint32_t token, uint64_t target,
                         ApsisSpiralAudioBinding *b)
{
    memset(b, 0, sizeof(*b));
    s->spiral.voice = voice;
    apsis_spiral_audio_plan(s, energy, interval_q32, rate, &b->acoustic);
    assert(b->acoustic.valid == 1u);
    b->valid = 1u;
    b->cutoff_voice = cutoff_voice;
    b->token = token;
    b->target_sample = target;
    b->pitch.valid = 1u;
    b->pitch.step = 0;
    b->pitch.tuning_mode = APSIS_TUNING_EQUAL;
    b->pitch.tuning_strength_q15 = APSIS_Q15_ONE;
    b->pitch.tuning_deviation_milli = 0;
}

static void test_two_leg_and_late_cutoff_edge(void)
{
    ApsisState s;
    ApsisSpiralDsp dsp;
    ApsisSpiralAudioBinding b;
    uint64_t interval = (uint64_t)18375u << 32;

    apsis_state_default(&s);
    apsis_spiral_dsp_reset(&dsp);
    make_binding(&s, APSIS_SPIRAL_THREAD, APSIS_SPIRAL_HALO,
                 APSIS_Q15_MILLI(820), interval, 2u, 77u, 5000u, &b);
    assert(b.acoustic.leg_count == 2u);
    assert(apsis_spiral_dsp_trigger_binding(&dsp, &s, &b, 0u) == 2u);
    assert(dsp.leg[0].active == 1u && dsp.leg[1].active == 1u);
    assert(dsp.leg[0].token == 77u && dsp.leg[1].token == 77u);
    assert(dsp.leg[0].topology_voice == APSIS_SPIRAL_THREAD);
    assert(dsp.leg[1].topology_voice == APSIS_SPIRAL_THREAD);
    assert(dsp.leg[0].cutoff_voice == APSIS_SPIRAL_HALO);
    assert(dsp.leg[0].q_milli == 280u);
    assert(dsp.leg[0].cutoff_hz == 2600u);
    assert(dsp.leg[0].osc_count == 2u && dsp.leg[1].osc_count == 2u);
    assert(dsp.leg[0].osc[0].waveform == APSIS_SPIRAL_DSP_TRIANGLE);
    assert(dsp.leg[1].osc[0].waveform == APSIS_SPIRAL_DSP_SINE);
    assert(dsp.leg[1].start_delay_samples - dsp.leg[0].start_delay_samples ==
           b.acoustic.leg[1].start_offset_samples);
    assert(b.acoustic.leg[1].start_offset_samples == 353u);
}

static void test_voice_references(void)
{
    uint8_t voice;
    for (voice = APSIS_SPIRAL_THREAD; voice <= APSIS_SPIRAL_HALO; ++voice) {
        ApsisState s;
        ApsisSpiralDsp dsp;
        ApsisSpiralAudioBinding b;
        int32_t out[TEST_FRAMES * 2u];
        uint32_t hash, peak;

        apsis_state_default(&s);
        apsis_spiral_dsp_reset(&dsp);
        memset(out, 0, sizeof(out));
        make_binding(&s, voice, voice, APSIS_Q15_MILLI(820),
                     (uint64_t)18375u << 32, 2u,
                     (uint32_t)voice + 1u, 0u, &b);
        assert(apsis_spiral_dsp_trigger_binding(&dsp, &s, &b, 0u) == 2u);
        apsis_spiral_dsp_render(&dsp, out, TEST_FRAMES);
        hash = hash_audio(2166136261u, out, TEST_FRAMES);
        peak = peak_audio(out, TEST_FRAMES);
        assert(peak > 0u);
        assert(dsp.steal_count == 0u);
        assert(dsp.drop_count == 0u);
        printf("M4F_REF voice=%u peak=%u hash=%08x legs=%u oscs=%u\n",
               voice, peak, hash, dsp.leg_high_water,
               dsp.oscillator_high_water);
    }
}

static uint64_t q32_mul_ratio(uint64_t value, uint32_t num, uint32_t den)
{
    return (value * num + den / 2u) / den;
}

static uint32_t build_stress_events(StressEvent ev[STRESS_EVENT_CAP],
                                    uint64_t end_sample)
{
    uint64_t base = (uint64_t)3675u << 32; /* 180 BPM, 1/16 source division */
    uint64_t at = 0u;
    uint32_t pulse = 0u, count = 0u;
    while (((at + 0x80000000ull) >> 32) < end_sample &&
           count + 4u <= STRESS_EVENT_CAP) {
        uint64_t interval = q32_mul_ratio(base, (pulse & 1u) ? 135u : 65u, 100u);
        uint8_t sub;
        for (sub = 0u; sub < 4u; ++sub) {
            uint64_t target_q32 = at + (interval * sub) / 4u;
            uint64_t target = (target_q32 + 0x80000000ull) >> 32;
            if (target < end_sample) {
                ev[count].target_sample = target;
                ev[count].interval_q32 = interval;
                ++count;
            }
        }
        at += interval;
        ++pulse;
    }
    return count;
}

static void test_worst_case_capacity_probe(void)
{
    static StressEvent ev[STRESS_EVENT_CAP];
    ApsisState s;
    ApsisSpiralDsp dsp;
    uint64_t end_sample = (uint64_t)APSIS_SPIRAL_DSP_RATE * STRESS_SECONDS;
    uint32_t count, next = 0u, token = 1u;
    uint64_t block_start;
    uint32_t hash = 2166136261u;

    apsis_state_default(&s);
    s.spiral.sound = 1u;
    s.spiral.voice = APSIS_SPIRAL_HALO;
    s.spiral.note_length_q15 = APSIS_Q15_MILLI(1350);
    s.spiral.level_q15 = APSIS_Q15_MILLI(1250);
    s.drift.sound = 1u;
    s.drift.amount_q15 = APSIS_Q15_ONE;
    s.drift.depth_q15 = APSIS_Q15_ONE;
    s.drift.level_q15 = APSIS_Q15_MILLI(1250);
    s.drift.width_q15 = APSIS_Q15_ONE;
    apsis_spiral_dsp_reset(&dsp);
    count = build_stress_events(ev, end_sample);
    assert(count > 100u);

    for (block_start = 0u; block_start < end_sample; block_start += 256u) {
        uint64_t block_end = block_start + 256u;
        uint32_t frames = block_end > end_sample
            ? (uint32_t)(end_sample - block_start) : 256u;
        int32_t out[512];
        memset(out, 0, sizeof(out));

        while (next < count) {
            uint64_t bind = ev[next].target_sample > 3528u
                ? ev[next].target_sample - 3528u : 0u;
            ApsisSpiralAudioBinding b;
            if (bind >= block_end) break;
            make_binding(&s, APSIS_SPIRAL_HALO, APSIS_SPIRAL_HALO,
                         APSIS_Q15_ONE, ev[next].interval_q32, 4u,
                         token++, ev[next].target_sample, &b);
            assert(apsis_spiral_dsp_trigger_binding(&dsp, &s, &b,
                                                    block_start) == 2u);
            ++next;
        }

        apsis_spiral_dsp_render(&dsp, out, frames);
        hash = hash_audio(hash, out, frames);
    }

    assert(next == count);
    assert(dsp.steal_count == 0u);
    assert(dsp.drop_count == 0u);
    printf("M4F_STRESS seconds=%u hits=%u logical_hw=%u leg_hw=%u osc_hw=%u steals=%u drops=%u hash=%08x\n",
           STRESS_SECONDS, count, dsp.logical_high_water, dsp.leg_high_water,
           dsp.oscillator_high_water, dsp.steal_count, dsp.drop_count, hash);
}

int main(void)
{
    test_two_leg_and_late_cutoff_edge();
    test_voice_references();
    test_worst_case_capacity_probe();
    puts("PASS: APSIS M4f finite Spiral renderer and capacity probe");
    return 0;
}
