#include <stdint.h>
#include <stdio.h>
#include "apsis_state.h"
#include "apsis_string_dsp.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static int32_t abs_i32(int32_t v) { return v < 0 ? -v : v; }

static uint32_t fnv(uint32_t h, int32_t v)
{
    uint32_t x = (uint32_t)v;
    h ^= x & 255u; h *= 16777619u;
    h ^= (x >> 8) & 255u; h *= 16777619u;
    h ^= (x >> 16) & 255u; h *= 16777619u;
    h ^= (x >> 24) & 255u; return h * 16777619u;
}

static void render_silently(ApsisStringTransientPool *pool, uint32_t samples)
{
    int32_t buf[512];
    while (samples) {
        uint32_t i;
        uint32_t frames = samples > 256u ? 256u : samples;
        for (i = 0u; i < frames * 2u; ++i) buf[i] = 0;
        apsis_string_transient_render(pool, buf, frames);
        samples -= frames;
    }
}

static int body_contract(void)
{
    ApsisState s;
    ApsisStringTransientPool a, b;
    ApsisStringTransientVoice *v;
    int32_t xa[512], xb[512];
    uint32_t left = 56000u, peak = 0u, nonzero = 0u;
    uint64_t energy_l = 0u, energy_r = 0u;
    uint32_t ha = 2166136261u, hb = 2166136261u;

    apsis_state_default(&s);
    apsis_string_transient_pool_reset(&a);
    apsis_string_transient_pool_reset(&b);

    CHECK(apsis_string_dsp_trigger_body(&a, &s, 130813u,
                                        APSIS_Q15_MILLI(52), 1180u,
                                        (int16_t)-APSIS_Q15_MILLI(300),
                                        APSIS_Q15_MILLI(440), 48u, 0u) == 1u);
    CHECK(apsis_string_dsp_trigger_body(&b, &s, 130813u,
                                        APSIS_Q15_MILLI(52), 1180u,
                                        (int16_t)-APSIS_Q15_MILLI(300),
                                        APSIS_Q15_MILLI(440), 48u, 0u) == 1u);
    CHECK(apsis_string_dsp_transient_logical_active(&a) == 1u);
    v = &a.transient[0];
    CHECK(v->active && v->waveform == APSIS_STRING_DSP_WAVE_BODY);
    CHECK(v->duration_samples == 52038u);
    CHECK(v->attack_left_samples == 2117u);
    CHECK(v->body_hold_left_samples == 3087u);
    CHECK(v->peak_gain_q15 > 1000u);
    CHECK(v->mix_left_q15 > v->mix_right_q15);
    CHECK(v->lowpass_alpha_q15 > 7000u && v->lowpass_alpha_q15 < 8500u);

    while (left) {
        uint32_t i;
        uint32_t frames = left > 256u ? 256u : left;
        for (i = 0u; i < frames * 2u; ++i) xa[i] = xb[i] = 0;
        apsis_string_transient_render(&a, xa, frames);
        apsis_string_transient_render(&b, xb, frames);
        for (i = 0u; i < frames * 2u; i += 2u) {
            uint32_t al = (uint32_t)abs_i32(xa[i]);
            uint32_t ar = (uint32_t)abs_i32(xa[i + 1u]);
            CHECK(xa[i] == xb[i]);
            CHECK(xa[i + 1u] == xb[i + 1u]);
            if (al > peak) peak = al;
            if (ar > peak) peak = ar;
            if (xa[i] || xa[i + 1u]) ++nonzero;
            energy_l += al;
            energy_r += ar;
            ha = fnv(ha, xa[i]); ha = fnv(ha, xa[i + 1u]);
            hb = fnv(hb, xb[i]); hb = fnv(hb, xb[i + 1u]);
        }
        left -= frames;
    }
    CHECK(ha == hb);
    CHECK(nonzero > 30000u);
    CHECK(peak > 100u && peak < 8000u);
    CHECK(energy_l > energy_r);
    CHECK(apsis_string_dsp_transient_physical_active(&a) == 0u);
    printf("PASS: M4b BODY render peak=%u hash=%08x energy=%llu/%llu\n",
           peak, ha, (unsigned long long)energy_l, (unsigned long long)energy_r);
    return 0;
}

static int composite_pool_contract(void)
{
    ApsisState s;
    ApsisStringTransientPool pool;
    uint32_t trigger;
    uint8_t saw_steal = 0u;

    apsis_state_default(&s);
    apsis_string_transient_pool_reset(&pool);

    /* Study-126 Classic composite: BODY at t x.66 plus WIRE pizz at t+4 ms
       x.78. With Foundation on the right pizz primitive is 3+3=6 voices, so
       one composite gesture occupies seven logical voices. */
    for (trigger = 0u; trigger < 70u; ++trigger) {
        uint16_t i;
        CHECK(apsis_string_dsp_trigger_body(&pool, &s, 146833u,
                                            APSIS_Q15_MILLI(34), 1180u,
                                            (int16_t)-APSIS_Q15_MILLI(300),
                                            APSIS_Q15_MILLI(440), 48u, 0u) == 1u);
        CHECK(apsis_string_dsp_trigger_classic_offset(&pool, &s, 146833u,
                                                       APSIS_Q15_MILLI(40), 660u,
                                                       (int16_t)APSIS_Q15_MILLI(300),
                                                       1u, APSIS_Q15_MILLI(50),
                                                       176u) == 6u); /* 4 ms */
        for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
            if (pool.transient[i].active && pool.transient[i].stealing)
                saw_steal = 1u;
        render_silently(&pool, 1323u); /* 30 ms direct-gesture debounce */
    }
    CHECK(saw_steal);
    CHECK(pool.transient_drop_count == 0u);
    CHECK(pool.transient_logical_high_water == APSIS_STRING_TRANSIENT_LOGICAL_CAP);
    CHECK(pool.transient_physical_high_water <= APSIS_STRING_TRANSIENT_POOL_CAP);
    CHECK(apsis_string_dsp_transient_logical_active(&pool) <= APSIS_STRING_TRANSIENT_LOGICAL_CAP);
    render_silently(&pool, 220000u);
    CHECK(apsis_string_dsp_transient_physical_active(&pool) == 0u);
    printf("PASS: M4b composite pool logical=%u physical=%u steals=%u drops=%u\n",
           pool.transient_logical_high_water, pool.transient_physical_high_water,
           pool.transient_steal_count, pool.transient_drop_count);
    return 0;
}

int main(void)
{
    CHECK(body_contract() == 0);
    CHECK(composite_pool_contract() == 0);
    puts("PASS: APSIS M4b native Study-126 BODY/composite transient topology");
    return 0;
}
