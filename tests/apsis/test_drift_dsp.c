#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_drift.h"
#include "apsis_drift_dsp.h"
#include "apsis_string_dsp.h"

static int32_t abs32(int32_t v) { return v < 0 ? -v : v; }

static uint16_t mul_q15(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

static uint32_t fnv_byte(uint32_t h, uint8_t b)
{
    h ^= b;
    return h * 16777619u;
}

static uint32_t hash_i32(uint32_t h, int32_t v)
{
    uint32_t u = (uint32_t)v;
    h = fnv_byte(h, (uint8_t)u);
    h = fnv_byte(h, (uint8_t)(u >> 8));
    h = fnv_byte(h, (uint8_t)(u >> 16));
    return fnv_byte(h, (uint8_t)(u >> 24));
}

static uint32_t gravity_root_millihz(const ApsisState *s)
{
    int32_t step = apsis_scale_at(s->gravity.tonic, s->gravity.scale,
                                  s->gravity.degree > 7u ? 7u : s->gravity.degree) * 1000;
    return apsis_string_dsp_frequency_millihz(s, step);
}

static void test_targets(void)
{
    ApsisState s;
    ApsisDriftDspTargets t;
    uint32_t carrier;
    uint16_t delta, expected_gain;

    apsis_state_default(&s);
    apsis_drift_dsp_targets(&s, 1u, &t);
    carrier = gravity_root_millihz(&s);
    delta = apsis_drift_audible_delta_millihz(&s.drift);
    expected_gain = mul_q15(APSIS_Q15_MILLI(13), s.drift.level_q15);

    assert(delta >= 586u && delta <= 588u);
    assert(t.freq_millihz[APSIS_DRIFT_DSP_LEFT] < carrier);
    assert(t.freq_millihz[APSIS_DRIFT_DSP_RIGHT] > carrier);
    assert((t.freq_millihz[APSIS_DRIFT_DSP_LEFT] +
            t.freq_millihz[APSIS_DRIFT_DSP_RIGHT]) / 2u == carrier);
    assert(t.freq_millihz[APSIS_DRIFT_DSP_RIGHT] -
           t.freq_millihz[APSIS_DRIFT_DSP_LEFT] ==
           (uint32_t)(((delta + 1u) / 2u) * 2u));
    assert(t.gain_q15[0] == expected_gain && t.gain_q15[1] == expected_gain);

    /* Source-history STEREO WIDTH law: width=0 centres both voices. */
    s.drift.width_q15 = 0u;
    apsis_drift_dsp_targets(&s, 1u, &t);
    assert(abs32((int32_t)t.mix_left_q15[0] - (int32_t)t.mix_right_q15[0]) <= 1);
    assert(t.mix_left_q15[0] == t.mix_left_q15[1]);
    assert(t.mix_right_q15[0] == t.mix_right_q15[1]);

    s.drift.width_q15 = APSIS_Q15_ONE;
    apsis_drift_dsp_targets(&s, 1u, &t);
    assert(t.mix_left_q15[0] > t.mix_right_q15[0]);
    assert(t.mix_right_q15[1] > t.mix_left_q15[1]);
    assert(t.mix_left_q15[0] == t.mix_right_q15[1]);
    assert(t.mix_right_q15[0] == t.mix_left_q15[1]);

    /* Fresh mute: no source gain and no beat separation, carrier retained. */
    s.drift.sound = 0u;
    apsis_drift_dsp_targets(&s, 1u, &t);
    carrier = gravity_root_millihz(&s);
    assert(t.gain_q15[0] == 0u && t.gain_q15[1] == 0u);
    assert(t.freq_millihz[0] == carrier && t.freq_millihz[1] == carrier);

    /* Transport gating controls gain, not the live target pitch model. */
    s.drift.sound = 1u;
    apsis_drift_dsp_targets(&s, 0u, &t);
    assert(t.gain_q15[0] == 0u && t.gain_q15[1] == 0u);
}

static void test_depth_floor_and_tuning(void)
{
    ApsisState s;
    ApsisDriftDspTargets t;
    uint16_t base, floor_delta;
    uint32_t equal_carrier, just_carrier, pyth_carrier;

    apsis_state_default(&s);
    s.drift.depth_q15 = 0u;
    base = apsis_drift_delta_millihz(s.drift.amount_q15);
    floor_delta = (uint16_t)(((uint32_t)base * APSIS_Q15_MILLI(150) +
                              APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
    floor_delta = (uint16_t)(((uint32_t)floor_delta * s.drift.level_q15 +
                              APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
    assert(abs32((int32_t)apsis_drift_audible_delta_millihz(&s.drift) -
                 (int32_t)floor_delta) <= 1);

    /* Pick D in C Dorian: tuning systems do not all agree on this degree. */
    s.gravity.degree = 1u;
    s.temper.mode = APSIS_TUNING_EQUAL;
    apsis_drift_dsp_targets(&s, 1u, &t);
    equal_carrier = (t.freq_millihz[0] + t.freq_millihz[1]) / 2u;
    assert(abs32((int32_t)equal_carrier - (int32_t)gravity_root_millihz(&s)) <= 1);

    s.temper.mode = APSIS_TUNING_JUST;
    apsis_drift_dsp_targets(&s, 1u, &t);
    just_carrier = (t.freq_millihz[0] + t.freq_millihz[1]) / 2u;
    assert(abs32((int32_t)just_carrier - (int32_t)gravity_root_millihz(&s)) <= 1);

    s.temper.mode = APSIS_TUNING_PYTH;
    apsis_drift_dsp_targets(&s, 1u, &t);
    pyth_carrier = (t.freq_millihz[0] + t.freq_millihz[1]) / 2u;
    assert(abs32((int32_t)pyth_carrier - (int32_t)gravity_root_millihz(&s)) <= 1);

    assert(equal_carrier != just_carrier || equal_carrier != pyth_carrier);

    /* Strength zero collapses a non-equal tuning back to equal temperament. */
    s.temper.mode = APSIS_TUNING_JUST;
    s.temper.strength_q15 = 0u;
    apsis_drift_dsp_targets(&s, 1u, &t);
    assert(abs32((int32_t)((t.freq_millihz[0] + t.freq_millihz[1]) / 2u) -
                 (int32_t)equal_carrier) <= 1);
}

static void test_smoothing_and_mute_tail(void)
{
    ApsisState s;
    ApsisDriftDsp dsp;
    int32_t out[2u * 1103u] = {0};
    int32_t before;

    apsis_state_default(&s);
    apsis_drift_dsp_reset(&dsp);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    assert(dsp.voice[0].gain_q23 == 0);
    assert(dsp.voice[0].target_gain_q23 > 0);
    apsis_drift_dsp_render(&dsp, out, 1103u);

    /* One 25 ms time constant should be about 63.2 % of target. */
    assert(dsp.voice[0].gain_q23 * 100 > dsp.voice[0].target_gain_q23 * 59);
    assert(dsp.voice[0].gain_q23 * 100 < dsp.voice[0].target_gain_q23 * 67);

    before = dsp.voice[0].gain_q23;
    s.drift.sound = 0u;
    apsis_drift_dsp_update(&dsp, &s, 1u);
    assert(dsp.voice[0].target_gain_q23 == 0);
    out[0] = out[1] = 0;
    apsis_drift_dsp_render(&dsp, out, 1u);
    assert(dsp.voice[0].gain_q23 > 0);
    assert(dsp.voice[0].gain_q23 < before);
}

static void render_reference(uint32_t *hash_out, int32_t *peak_out)
{
    ApsisState s;
    ApsisDriftDsp dsp;
    int32_t out[2u * 256u];
    uint32_t hash = 2166136261u;
    uint32_t done = 0u;
    int32_t peak = 0;

    apsis_state_default(&s);
    apsis_drift_dsp_reset(&dsp);
    apsis_drift_dsp_update(&dsp, &s, 1u);
    while (done < APSIS_DRIFT_DSP_RATE * 2u) {
        uint32_t frames = APSIS_DRIFT_DSP_RATE * 2u - done;
        uint32_t i;
        if (frames > 256u) frames = 256u;
        for (i = 0u; i < frames * 2u; ++i) out[i] = 0;
        apsis_drift_dsp_render(&dsp, out, frames);
        for (i = 0u; i < frames * 2u; ++i) {
            int32_t a = out[i] < 0 ? -out[i] : out[i];
            if (a > peak) peak = a;
            hash = hash_i32(hash, out[i]);
        }
        done += frames;
    }
    *hash_out = hash;
    *peak_out = peak;
}

static void test_reference_render(void)
{
    uint32_t h1, h2;
    int32_t p1, p2;
    render_reference(&h1, &p1);
    render_reference(&h2, &p2);
    assert(h1 == h2 && p1 == p2);
    assert(p1 == 488);
    assert(h1 == 0x0aec6bc2u);
    printf("PASS: M4d Drift reference peak=%ld hash=%08lx\n",
           (long)p1, (unsigned long)h1);
}

int main(void)
{
    test_targets();
    test_depth_floor_and_tuning();
    test_smoothing_and_mute_tail();
    test_reference_render();
    puts("PASS: APSIS M4d source-resolved persistent Drift DSP");
    return 0;
}
