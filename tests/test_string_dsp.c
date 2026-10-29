#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_drift.h"
#include "apsis_string_dsp.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

static uint32_t absdiff_u32(uint32_t a, uint32_t b) { return a > b ? a - b : b - a; }
static int32_t abs_i32(int32_t v) { return v < 0 ? -v : v; }
static uint32_t fnv(uint32_t h, int32_t v)
{
    uint32_t x = (uint32_t)v;
    h ^= x & 255u; h *= 16777619u;
    h ^= (x >> 8) & 255u; h *= 16777619u;
    h ^= (x >> 16) & 255u; h *= 16777619u;
    h ^= (x >> 24) & 255u; return h * 16777619u;
}

static int target_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringDspTargets t;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&m);
    CHECK(apsis_drift_audible_delta_millihz(&s.drift) == 587u);

    /* Default Study-126 mode is Dual. Study 98 supplies Foundation; the two
       side voices retain Study-126 BODY/WIRE because Study 98 had no Dual. */
    apsis_string_dsp_targets(&s, &m, 1u, &t);
    CHECK(t.enabled[0] && t.enabled[1] && t.enabled[2] && t.enabled[3]);
    CHECK(absdiff_u32(t.freq_millihz[0], 65084u) <= 2u);
    CHECK(absdiff_u32(t.freq_millihz[1], 65730u) <= 2u);
    CHECK(t.freq_millihz[2] == 146833u);
    CHECK(t.freq_millihz[3] == 220000u);
    CHECK(t.gain_q15[0] == 218u && t.gain_q15[1] == 218u);
    CHECK(t.gain_q15[2] == 378u);
    CHECK(t.gain_q15[3] == 276u);
    CHECK(t.waveform[2] == APSIS_STRING_DSP_WAVE_BODY);
    CHECK(t.waveform[3] == APSIS_STRING_DSP_WAVE_TRIANGLE);
    CHECK(t.lowpass_alpha_q15[2] == 6493u);
    CHECK(t.lowpass_alpha_q15[3] == 13148u);
    CHECK(t.mix_left_q15[2] == 26955u && t.mix_right_q15[2] == 18630u);
    CHECK(t.mix_left_q15[3] == 18630u && t.mix_right_q15[3] == 26955u);

    /* Left and right are genuinely independent native String targets. */
    apsis_string_set_side_split(&s.string, APSIS_SIDE_LEFT, APSIS_Q15_MILLI(430), 0u);
    apsis_string_pitch_memory_reset(&m);
    apsis_string_dsp_targets(&s, &m, 1u, &t);
    CHECK(t.freq_millihz[2] != 146833u);
    CHECK(t.freq_millihz[3] == 220000u);

    /* Classic restores Study-98 sine/Drift sustain around one played pitch. */
    apsis_state_default(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    apsis_string_pitch_memory_reset(&m);
    apsis_string_dsp_targets(&s, &m, 1u, &t);
    CHECK(t.enabled[0] && t.enabled[1] && t.enabled[2] && t.enabled[3]);
    CHECK(absdiff_u32(t.freq_millihz[0], 65084u) <= 2u);
    CHECK(absdiff_u32(t.freq_millihz[1], 65730u) <= 2u);
    CHECK(absdiff_u32(t.freq_millihz[2], 146539u) <= 2u);
    CHECK(absdiff_u32(t.freq_millihz[3], 147127u) <= 2u);
    CHECK(t.gain_q15[0] == 218u && t.gain_q15[1] == 218u);
    CHECK(t.gain_q15[2] == 524u && t.gain_q15[3] == 524u);
    CHECK(t.waveform[2] == APSIS_STRING_DSP_WAVE_SINE &&
          t.waveform[3] == APSIS_STRING_DSP_WAVE_SINE);
    CHECK(t.mix_left_q15[2] == 32767u && t.mix_right_q15[2] == 32767u);

    s.string.foundation = 0u;
    apsis_string_dsp_targets(&s, &m, 1u, &t);
    CHECK(!t.enabled[0] && !t.enabled[1]);
    CHECK(t.gain_q15[0] == 0u && t.gain_q15[1] == 0u);
    CHECK(t.enabled[2] && t.enabled[3]);

    s.string.foundation = 1u;
    s.string.sound = 0u;
    apsis_string_dsp_targets(&s, &m, 1u, &t);
    CHECK(t.gain_q15[0] == 0u && t.gain_q15[1] == 0u &&
          t.gain_q15[2] == 0u && t.gain_q15[3] == 0u);
    s.string.sound = 1u;
    apsis_string_dsp_targets(&s, &m, 0u, &t);
    CHECK(t.gain_q15[0] == 0u && t.gain_q15[1] == 0u &&
          t.gain_q15[2] == 0u && t.gain_q15[3] == 0u);
    return 0;
}

static int classic_render_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringDsp d;
    int32_t buf[512];
    uint32_t k, i, peak = 0u, h = 2166136261u, nonzero = 0u;
    int64_t sum = 0;

    apsis_state_default(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    apsis_string_pitch_memory_reset(&m);
    apsis_string_dsp_reset(&d);
    apsis_string_dsp_update(&d, &s, &m, 1u);

    for (k = 0u; k < 600u; ++k) {
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
        for (i = 0u; i < 512u; ++i) {
            uint32_t a = (uint32_t)abs_i32(buf[i]);
            if (a > peak) peak = a;
            if (buf[i]) nonzero++;
            sum += buf[i];
            h = fnv(h, buf[i]);
        }
    }
    CHECK(nonzero > 100000u);
    CHECK(peak > 100u && peak < 5000u);
    CHECK(llabs(sum) / (int64_t)(600u * 512u) < 8);

    {
        uint32_t phase = d.voice[2].phase;
        uint32_t old_target = d.voice[2].target_phase_inc;
        uint32_t old_inc = d.voice[2].phase_inc;
        apsis_string_set_side_split(&s.string, APSIS_SIDE_RIGHT, APSIS_Q15_MILLI(650), 0u);
        apsis_string_dsp_update(&d, &s, &m, 1u);
        CHECK(d.voice[2].phase == phase);
        CHECK(d.voice[2].target_phase_inc != old_target);
        CHECK(d.voice[2].phase_inc == old_inc);
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
        CHECK(d.voice[2].phase != phase);
        CHECK(d.voice[2].phase_inc != old_inc);
    }

    apsis_string_dsp_update(&d, &s, &m, 0u);
    for (k = 0u; k < 100u; ++k) {
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
    }
    CHECK(abs_i32(d.voice[0].gain_q15) <= 1 && abs_i32(d.voice[1].gain_q15) <= 1 &&
          abs_i32(d.voice[2].gain_q15) <= 1 && abs_i32(d.voice[3].gain_q15) <= 1);
    for (i = 0u; i < 512u; ++i) buf[i] = 0;
    apsis_string_dsp_render(&d, buf, 256u);
    for (i = 0u; i < 512u; ++i) CHECK(abs_i32(buf[i]) <= 4);

    {
        uint32_t p = d.voice[2].phase;
        apsis_string_dsp_update(&d, &s, &m, 1u);
        CHECK(d.voice[2].phase == p);
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
        CHECK(d.voice[2].gain_q15 > 0);
    }
    printf("PASS: M4a Classic render peak=%u hash=%08x\n", peak, h);
    return 0;
}

static int dual_render_and_transition_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory m;
    ApsisStringDsp d;
    int32_t buf[512];
    uint32_t k, i, left_peak = 0u, right_peak = 0u;
    uint32_t phase2, phase3;

    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&m);
    apsis_string_dsp_reset(&d);
    apsis_string_dsp_update(&d, &s, &m, 1u);
    CHECK(d.voice[2].waveform == APSIS_STRING_DSP_WAVE_BODY);
    CHECK(d.voice[3].waveform == APSIS_STRING_DSP_WAVE_TRIANGLE);
    for (k = 0u; k < 300u; ++k) {
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
        for (i = 0u; i < 256u; ++i) {
            uint32_t l = (uint32_t)abs_i32(buf[i * 2u]);
            uint32_t r = (uint32_t)abs_i32(buf[i * 2u + 1u]);
            if (l > left_peak) left_peak = l;
            if (r > right_peak) right_peak = r;
        }
    }
    CHECK(left_peak > 100u && right_peak > 100u);
    CHECK(left_peak != right_peak);

    /* Study-126 rebuild semantics become a native fade-to-zero timbre swap.
       No phase reset is allowed while changing Dual -> Classic. */
    phase2 = d.voice[2].phase;
    phase3 = d.voice[3].phase;
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    apsis_string_pitch_memory_reset(&m);
    apsis_string_dsp_update(&d, &s, &m, 1u);
    CHECK(d.voice[2].timbre_transition && d.voice[3].timbre_transition);
    CHECK(d.voice[2].phase == phase2 && d.voice[3].phase == phase3);
    for (k = 0u; k < 200u && (d.voice[2].timbre_transition || d.voice[3].timbre_transition); ++k) {
        for (i = 0u; i < 512u; ++i) buf[i] = 0;
        apsis_string_dsp_render(&d, buf, 256u);
    }
    CHECK(!d.voice[2].timbre_transition && !d.voice[3].timbre_transition);
    CHECK(d.voice[2].waveform == APSIS_STRING_DSP_WAVE_SINE);
    CHECK(d.voice[3].waveform == APSIS_STRING_DSP_WAVE_SINE);
    CHECK(d.voice[2].gain_q15 > 0 && d.voice[3].gain_q15 > 0);
    CHECK(d.voice[2].phase != phase2 && d.voice[3].phase != phase3);
    printf("PASS: M4a Dual render L=%u R=%u\n", left_peak, right_peak);
    return 0;
}

static int deterministic_contract(void)
{
    ApsisState s;
    ApsisStringPitchMemory ma, mb;
    ApsisStringDsp a, b;
    int32_t xa[512], xb[512];
    uint32_t k, i, ha = 2166136261u, hb = 2166136261u;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&ma);
    apsis_string_pitch_memory_reset(&mb);
    apsis_string_dsp_reset(&a);
    apsis_string_dsp_reset(&b);
    apsis_string_dsp_update(&a, &s, &ma, 1u);
    apsis_string_dsp_update(&b, &s, &mb, 1u);
    for (k = 0u; k < 200u; ++k) {
        for (i = 0u; i < 512u; ++i) xa[i] = xb[i] = 0;
        apsis_string_dsp_render(&a, xa, 256u);
        apsis_string_dsp_render(&b, xb, 256u);
        for (i = 0u; i < 512u; ++i) {
            CHECK(xa[i] == xb[i]);
            ha = fnv(ha, xa[i]);
            hb = fnv(hb, xb[i]);
        }
    }
    CHECK(ha == hb);
    printf("PASS: M4a Dual deterministic hash=%08x\n", ha);
    return 0;
}

int main(void)
{
    CHECK(target_contract() == 0);
    CHECK(classic_render_contract() == 0);
    CHECK(dual_render_and_transition_contract() == 0);
    CHECK(deterministic_contract() == 0);
    puts("PASS: APSIS M4a source-resolved persistent String DSP");
    return 0;
}
