#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_string_attractor_audio.h"

static uint16_t mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t phase_inc(uint32_t mhz)
{
    return (uint32_t)(((uint64_t)mhz * 6382653u + 32768u) >> 16);
}

static uint32_t side_freq(const ApsisState *s, uint8_t side)
{
    ApsisStringPitchMemory m;
    int32_t step;
    apsis_string_pitch_memory_reset(&m);
    step = apsis_string_target_step_milli(s, side, &m, 1u);
    return apsis_string_dsp_frequency_millihz(s, step);
}

static uint16_t active_count(const ApsisStringTransientPool *pool)
{
    return apsis_string_dsp_transient_logical_active(pool);
}

static ApsisStringAttractorBinding binding(uint8_t route, uint16_t value,
                                           uint16_t base, uint64_t target)
{
    ApsisStringAttractorBinding b = {0};
    b.valid = 1u;
    b.route = route;
    b.value_q15 = value;
    b.base_level_q15 = base;
    b.target_sample = target;
    return b;
}

static void test_left_body_is_one_voice_and_target_delayed(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    ApsisStringTransientPool pool;
    ApsisStringAttractorBinding b;
    int32_t silence[2000u * 2u] = {0};
    int32_t sounding[64u * 2u] = {0};
    uint32_t freq;
    uint32_t i;
    uint8_t heard = 0u;

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    apsis_string_transient_pool_reset(&pool);
    s.string.foundation = 1u; /* BODY must still have no root shadow. */
    s.string.level_q15 = APSIS_Q15_MILLI(800);
    s.string.left_level_q15 = APSIS_Q15_MILLI(500);
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_LEFT, APSIS_Q15_MILLI(700),
                APSIS_Q15_MILLI(120), 5000u);
    freq = side_freq(&s, APSIS_SIDE_LEFT);

    assert(apsis_string_attractor_trigger_binding(&pool, &s, &mem, &b, 3000u) == 1u);
    assert(active_count(&pool) == 1u);
    assert(pool.transient[0].waveform == APSIS_STRING_DSP_WAVE_BODY);
    assert(pool.transient[0].phase_inc == phase_inc(freq));
    assert(pool.transient[0].start_delay_samples == 2000u);
    assert(pool.transient[0].duration_samples == 31752u); /* 720 ms */
    assert(pool.transient[0].attack_left_samples == 1323u); /* 30 ms */

    apsis_string_transient_render(&pool, silence, 2000u);
    for (i = 0u; i < 4000u; ++i) assert(silence[i] == 0);
    apsis_string_transient_render(&pool, sounding, 64u);
    for (i = 0u; i < 128u; ++i) if (sounding[i] != 0) heard = 1u;
    assert(heard);
}

static void test_bind_rechecks_sound(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    ApsisStringTransientPool pool;
    ApsisStringAttractorBinding b;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    apsis_string_transient_pool_reset(&pool);
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_LEFT, APSIS_Q15_MILLI(500),
                APSIS_Q15_MILLI(100), 1000u);
    s.string.sound = 0u;
    assert(apsis_string_attractor_trigger_binding(&pool, &s, &mem, &b, 0u) == 0u);
    assert(active_count(&pool) == 0u);
}

static void test_right_late_reads_pitch_and_side_level(void)
{
    ApsisState a, bstate;
    ApsisStringPitchMemory ma, mb;
    ApsisStringTransientPool pa, pb;
    ApsisStringAttractorBinding b;
    uint32_t fa, fb;

    apsis_state_default(&a);
    bstate = a;
    a.string.foundation = 0u;
    bstate.string.foundation = 0u;
    a.string.right_split_q15 = APSIS_Q15_MILLI(560);
    bstate.string.right_split_q15 = APSIS_Q15_MILLI(720);
    a.string.right_level_q15 = APSIS_Q15_MILLI(300);
    bstate.string.right_level_q15 = APSIS_Q15_MILLI(900);
    fa = side_freq(&a, APSIS_SIDE_RIGHT);
    fb = side_freq(&bstate, APSIS_SIDE_RIGHT);
    assert(fa != fb);

    apsis_string_pitch_memory_reset(&ma);
    apsis_string_pitch_memory_reset(&mb);
    apsis_string_transient_pool_reset(&pa);
    apsis_string_transient_pool_reset(&pb);
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_RIGHT, APSIS_Q15_MILLI(400),
                APSIS_Q15_MILLI(120), 4000u); /* no accent */

    assert(apsis_string_attractor_trigger_binding(&pa, &a, &ma, &b, 3000u) == 3u);
    assert(apsis_string_attractor_trigger_binding(&pb, &bstate, &mb, &b, 3000u) == 3u);
    assert(pa.transient[0].phase_inc == phase_inc(fa));
    assert(pb.transient[0].phase_inc == phase_inc(fb));
    assert(pa.transient[0].peak_gain_q15 < pb.transient[0].peak_gain_q15);
}

static void test_route_is_frozen_not_current_surface(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    ApsisStringTransientPool pool;
    ApsisStringAttractorBinding b;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    apsis_string_transient_pool_reset(&pool);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_RIGHT;
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_LEFT, APSIS_Q15_MILLI(300),
                APSIS_Q15_MILLI(100), 1000u);
    assert(apsis_string_attractor_trigger_binding(&pool, &s, &mem, &b, 0u) == 1u);
    assert(active_count(&pool) == 1u);
    assert(pool.transient[0].waveform == APSIS_STRING_DSP_WAVE_BODY);
}

static void test_right_wire_foundation_and_accent(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    ApsisStringTransientPool pool;
    ApsisStringAttractorBinding b;
    uint16_t expected_input, expected_accent;
    uint32_t freq, i;
    ApsisStringTransientVoice *accent = 0;

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    apsis_string_transient_pool_reset(&pool);
    s.string.level_q15 = APSIS_Q15_ONE;
    s.string.right_level_q15 = APSIS_Q15_ONE;
    s.string.foundation = 1u;
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_RIGHT, APSIS_Q15_MILLI(700),
                APSIS_Q15_MILLI(100), 5000u);
    freq = side_freq(&s, APSIS_SIDE_RIGHT);
    expected_input = b.base_level_q15;
    expected_accent = mul_q15(expected_input, APSIS_Q15_MILLI(150));

    /* 3 main pizz partials + 3 Foundation-root partials + one sine accent. */
    assert(apsis_string_attractor_trigger_binding(&pool, &s, &mem, &b, 3000u) == 7u);
    assert(active_count(&pool) == 7u);
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool.transient[i];
        if (v->active && v->waveform == APSIS_STRING_DSP_WAVE_SINE &&
            v->duration_samples == 7779u) {
            accent = v;
            break;
        }
    }
    assert(accent);
    assert(accent->phase_inc == phase_inc(freq * 2u));
    assert(accent->peak_gain_q15 == expected_accent); /* no WIRE .92 here */
    assert(accent->start_delay_samples == 2265u);     /* target delay + 6 ms */
    assert(accent->attack_left_samples == 706u);      /* generic String 16 ms onset */
    assert(accent->mix_left_q15 == 18918u && accent->mix_right_q15 == 26742u);
}

static void test_classic_composite_offsets_and_frozen_route(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    ApsisStringTransientPool pool;
    ApsisStringAttractorBinding b;
    uint32_t i;
    uint8_t body = 0u, accent = 0u;

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    apsis_string_transient_pool_reset(&pool);
    s.string.surface_mode = APSIS_STRING_DUAL; /* changed after planning */
    s.string.attractor_target = APSIS_STRING_ATTRACTOR_LEFT;
    s.string.foundation = 0u;
    b = binding(APSIS_STRING_ATTRACTOR_ROUTE_CLASSIC, APSIS_Q15_MILLI(700),
                APSIS_Q15_MILLI(100), 5000u);

    /* BODY + 3 WIRE pizz partials + accent. */
    assert(apsis_string_attractor_trigger_binding(&pool, &s, &mem, &b, 4000u) == 5u);
    assert(active_count(&pool) == 5u);
    for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i) {
        ApsisStringTransientVoice *v = &pool.transient[i];
        if (!v->active) continue;
        if (v->waveform == APSIS_STRING_DSP_WAVE_BODY) {
            body++;
            assert(v->start_delay_samples == 1000u);
        }
        if (v->waveform == APSIS_STRING_DSP_WAVE_SINE && v->duration_samples == 7779u) {
            accent++;
            assert(v->start_delay_samples == 1441u); /* target +4 ms WIRE +6 ms */
        }
    }
    assert(body == 1u && accent == 1u);
}

static void test_both_scale_is_frozen(void)
{
    ApsisState s;
    ApsisStringPitchMemory m0, m1;
    ApsisStringTransientPool p0, p1;
    ApsisStringAttractorBinding a, b;
    apsis_state_default(&s);
    s.string.foundation = 0u;
    s.string.right_level_q15 = APSIS_Q15_ONE;
    apsis_string_pitch_memory_reset(&m0);
    apsis_string_pitch_memory_reset(&m1);
    apsis_string_transient_pool_reset(&p0);
    apsis_string_transient_pool_reset(&p1);
    a = binding(APSIS_STRING_ATTRACTOR_ROUTE_RIGHT, APSIS_Q15_MILLI(400),
                APSIS_Q15_MILLI(100), 1000u);
    b = a;
    b.both_scale = 1u;
    assert(apsis_string_attractor_trigger_binding(&p0, &s, &m0, &a, 0u) == 3u);
    assert(apsis_string_attractor_trigger_binding(&p1, &s, &m1, &b, 0u) == 3u);
    assert(p1.transient[0].peak_gain_q15 < p0.transient[0].peak_gain_q15);
}

int main(void)
{
    test_left_body_is_one_voice_and_target_delayed();
    test_bind_rechecks_sound();
    test_right_late_reads_pitch_and_side_level();
    test_route_is_frozen_not_current_surface();
    test_right_wire_foundation_and_accent();
    test_classic_composite_offsets_and_frozen_route();
    test_both_scale_is_frozen();
    puts("PASS: APSIS M4c source-derived Attractor late-bind audio topology");
    return 0;
}
