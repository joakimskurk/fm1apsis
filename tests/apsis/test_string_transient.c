#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

static void render_samples(ApsisStringTransientPool *pool, uint32_t samples)
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

static int plan_contract(void)
{
    ApsisState s;
    ApsisStringTransientPlan p;
    uint32_t root;
    static const uint16_t main_gain[4] = {1406u,481u,257u,165u};
    static const uint16_t root_gain[4] = {393u,135u,72u,46u};
    static const uint32_t durations[4] = {40572u,47064u,53555u,60047u};
    static const uint32_t offsets[4] = {0u,132u,264u,396u};
    uint8_t i;

    apsis_state_default(&s);
    root = apsis_string_dsp_frequency_millihz(&s, 0);
    CHECK(root == 130813u);

    apsis_string_dsp_transient_plan(&s, 146833u, APSIS_Q15_MILLI(65), 920u,
                                    (int16_t)APSIS_Q15_MILLI(300), 0u,
                                    APSIS_Q15_MILLI(280), &p);
    CHECK(p.count == 8u);
    for (i = 0u; i < 4u; ++i) {
        CHECK(p.partial[i].freq_millihz == 146833u * (uint32_t)(i + 1u));
        CHECK(p.partial[i].start_offset_samples == offsets[i]);
        CHECK(p.partial[i].duration_samples == durations[i]);
        CHECK(p.partial[i].peak_gain_q15 == main_gain[i]);
        CHECK(p.partial[i].waveform == (i < 2u ? APSIS_STRING_DSP_WAVE_TRIANGLE
                                               : APSIS_STRING_DSP_WAVE_SINE));
        CHECK(!p.partial[i].root_shadow);
        CHECK(p.partial[i + 4u].freq_millihz == root * (uint32_t)(i + 1u));
        CHECK(p.partial[i + 4u].start_offset_samples == offsets[i]);
        CHECK(p.partial[i + 4u].duration_samples == durations[i]);
        CHECK(p.partial[i + 4u].peak_gain_q15 == root_gain[i]);
        CHECK(p.partial[i + 4u].root_shadow);
    }
    CHECK(p.partial[0].lowpass_alpha_q15 == 6679u);
    CHECK(p.partial[1].lowpass_alpha_q15 == 6679u);
    CHECK(p.partial[2].lowpass_alpha_q15 == 0u && p.partial[3].lowpass_alpha_q15 == 0u);
    CHECK(p.partial[0].mix_left_q15 == 17118u && p.partial[0].mix_right_q15 == 27934u);
    CHECK(p.partial[1].mix_left_q15 == 25489u && p.partial[1].mix_right_q15 == 20584u);

    apsis_string_dsp_transient_plan(&s, 146833u, APSIS_Q15_MILLI(65), 340u, 0,
                                    1u, APSIS_Q15_MILLI(180), &p);
    CHECK(p.count == 6u);
    CHECK(p.partial[0].peak_gain_q15 == 1406u);
    CHECK(p.partial[1].peak_gain_q15 == 490u);
    CHECK(p.partial[2].peak_gain_q15 == 265u);
    CHECK(p.partial[3].peak_gain_q15 == 253u);
    CHECK(p.partial[4].peak_gain_q15 == 88u);
    CHECK(p.partial[5].peak_gain_q15 == 48u);
    CHECK(p.partial[0].duration_samples == 14994u);
    CHECK(p.partial[1].duration_samples == 13645u);
    CHECK(p.partial[2].duration_samples == 12295u);
    CHECK(p.partial[0].mix_left_q15 == 32767u && p.partial[0].mix_right_q15 == 32767u);

    /* Root shadow folds down by octaves only while root > played * 1.2. */
    apsis_string_dsp_transient_plan(&s, 80000u, APSIS_Q15_MILLI(65), 920u, 0,
                                    0u, APSIS_Q15_MILLI(280), &p);
    CHECK(p.count == 8u);
    CHECK(p.partial[4].freq_millihz == 65406u);

    /* Independent source-derived attenuation anchors at brightness 0.15 and 1.0. */
    s.string.resonance_q15 = APSIS_Q15_ONE;
    s.string.brightness_q15 = APSIS_Q15_MILLI(150);
    apsis_string_dsp_transient_plan(&s, 146833u, 10000u, 920u, 0, 0u, 0u, &p);
    CHECK(p.partial[0].peak_gain_q15 == 10000u);
    CHECK(p.partial[1].peak_gain_q15 == 2926u);
    CHECK(p.partial[2].peak_gain_q15 == 1426u);
    CHECK(p.partial[3].peak_gain_q15 == 856u);
    s.string.brightness_q15 = APSIS_Q15_ONE;
    apsis_string_dsp_transient_plan(&s, 146833u, 10000u, 920u, 0, 0u, 0u, &p);
    CHECK(p.partial[1].peak_gain_q15 == 4118u);
    CHECK(p.partial[2].peak_gain_q15 == 2451u);
    CHECK(p.partial[3].peak_gain_q15 == 1696u);
    return 0;
}

static int render_contract(void)
{
    ApsisState s;
    ApsisStringTransientPool a, b;
    int32_t xa[512], xb[512];
    uint32_t left = 61000u, peak = 0u, nonzero = 0u;
    uint32_t ha = 2166136261u, hb = 2166136261u;

    apsis_state_default(&s);
    apsis_string_transient_pool_reset(&a);
    apsis_string_transient_pool_reset(&b);
    CHECK(apsis_string_dsp_trigger_classic(&a, &s, 146833u, APSIS_Q15_MILLI(65),
                                           920u, (int16_t)APSIS_Q15_MILLI(300),
                                           0u, APSIS_Q15_MILLI(280)) == 8u);
    CHECK(apsis_string_dsp_trigger_classic(&b, &s, 146833u, APSIS_Q15_MILLI(65),
                                           920u, (int16_t)APSIS_Q15_MILLI(300),
                                           0u, APSIS_Q15_MILLI(280)) == 8u);
    CHECK(apsis_string_dsp_transient_logical_active(&a) == 8u);
    while (left) {
        uint32_t i;
        uint32_t frames = left > 256u ? 256u : left;
        for (i = 0u; i < frames * 2u; ++i) xa[i] = xb[i] = 0;
        apsis_string_transient_render(&a, xa, frames);
        apsis_string_transient_render(&b, xb, frames);
        for (i = 0u; i < frames * 2u; ++i) {
            uint32_t av = (uint32_t)abs_i32(xa[i]);
            CHECK(xa[i] == xb[i]);
            if (av > peak) peak = av;
            if (xa[i]) ++nonzero;
            ha = fnv(ha, xa[i]);
            hb = fnv(hb, xb[i]);
        }
        left -= frames;
    }
    CHECK(ha == hb);
    CHECK(nonzero > 50000u);
    CHECK(peak > 100u && peak < 10000u);
    CHECK(apsis_string_dsp_transient_physical_active(&a) == 0u);
    CHECK(a.transient_drop_count == 0u && a.transient_steal_count == 0u);
    printf("PASS: M4b1 transient render peak=%u hash=%08x\n", peak, ha);
    return 0;
}

static int legal_burst_pool_contract(void)
{
    ApsisState s;
    ApsisStringTransientPool d;
    uint32_t trigger;
    uint8_t saw_stealing = 0u;
    apsis_state_default(&s);
    apsis_string_transient_pool_reset(&d);

    /* Study-126 direct String gestures debounce at 30 ms. A full normal
       Study-98 transient + root shadow consumes eight partial voices. */
    for (trigger = 0u; trigger < 70u; ++trigger) {
        uint16_t i;
        CHECK(apsis_string_dsp_trigger_classic(&d, &s, 146833u,
                                               APSIS_Q15_MILLI(65), 1050u, 0,
                                               0u, APSIS_Q15_MILLI(280)) == 8u);
        for (i = 0u; i < APSIS_STRING_TRANSIENT_POOL_CAP; ++i)
            if (d.transient[i].active && d.transient[i].stealing) saw_stealing = 1u;
        render_samples(&d, 1323u); /* 30 ms */
    }
    CHECK(saw_stealing);
    CHECK(d.transient_steal_count > 0u);
    CHECK(d.transient_drop_count == 0u);
    CHECK(d.transient_logical_high_water == APSIS_STRING_TRANSIENT_LOGICAL_CAP);
    CHECK(d.transient_physical_high_water == APSIS_STRING_TRANSIENT_POOL_CAP);
    CHECK(apsis_string_dsp_transient_logical_active(&d) <= APSIS_STRING_TRANSIENT_LOGICAL_CAP);

    render_samples(&d, 220000u);
    CHECK(apsis_string_dsp_transient_physical_active(&d) == 0u);
    printf("PASS: M4b1 pool logical=%u physical=%u steals=%u drops=%u\n",
           d.transient_logical_high_water, d.transient_physical_high_water,
           d.transient_steal_count, d.transient_drop_count);
    return 0;
}

int main(void)
{
    CHECK(plan_contract() == 0);
    CHECK(render_contract() == 0);
    CHECK(legal_burst_pool_contract() == 0);
    puts("PASS: APSIS M4b1 source-derived finite String transient engine");
    return 0;
}
