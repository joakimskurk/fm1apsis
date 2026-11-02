#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"
#include "apsis_string_motion_dsp.h"

static void defaults(ApsisState *s)
{
    apsis_state_default(s);
    s->string.sound = 1u;
    s->string.level_q15 = APSIS_Q15_ONE;
}

static void test_dual_override_only_primary(void)
{
    ApsisState s;
    ApsisStringDsp dsp;
    ApsisStringPitchMemory memory;
    ApsisStringMotionPlayback p;
    uint32_t f0, f1, before_a, before_b;

    defaults(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    apsis_string_dsp_reset(&dsp);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_dsp_update(&dsp, &s, &memory, 1u);
    f0 = dsp.voice[APSIS_STRING_DSP_FOUNDATION_LO].target_phase_inc;
    f1 = dsp.voice[APSIS_STRING_DSP_FOUNDATION_HI].target_phase_inc;
    before_a = dsp.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc;
    before_b = dsp.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc;

    p.left_pitch_milli = 0;
    p.right_pitch_milli = 12000;
    p.left_split_q15 = s.string.left_split_q15;
    p.right_split_q15 = s.string.right_split_q15;
    p.phase_q16 = 0u;
    apsis_string_motion_dsp_override(&dsp, &s, &p);

    assert(dsp.voice[APSIS_STRING_DSP_FOUNDATION_LO].target_phase_inc == f0);
    assert(dsp.voice[APSIS_STRING_DSP_FOUNDATION_HI].target_phase_inc == f1);
    assert(dsp.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc != before_a ||
           dsp.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc != before_b);
    assert(dsp.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc <
           dsp.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc);
}

static void test_frozen_pitch_ignores_later_gravity_tuning(void)
{
    ApsisState s;
    ApsisStringDsp a, b;
    ApsisStringPitchMemory ma, mb;
    ApsisStringMotionPlayback p;
    uint32_t left, right;

    defaults(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    p.left_pitch_milli = 4000;
    p.right_pitch_milli = 11000;
    p.left_split_q15 = s.string.left_split_q15;
    p.right_split_q15 = s.string.right_split_q15;
    p.phase_q16 = 20000u;

    apsis_string_dsp_reset(&a);
    apsis_string_pitch_memory_reset(&ma);
    apsis_string_dsp_update(&a, &s, &ma, 1u);
    apsis_string_motion_dsp_override(&a, &s, &p);
    left = a.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc;
    right = a.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc;

    s.gravity.tonic = 9u;
    s.gravity.degree = 6u;
    s.gravity.scale = APSIS_SCALE_HARMONIC_MINOR;
    s.gravity.scale_lock = 1u;
    s.temper.mode = APSIS_TUNING_JUST;
    s.temper.strength_q15 = APSIS_Q15_ONE;
    apsis_string_dsp_reset(&b);
    apsis_string_pitch_memory_reset(&mb);
    apsis_string_dsp_update(&b, &s, &mb, 1u);
    apsis_string_motion_dsp_override(&b, &s, &p);

    assert(b.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc == left);
    assert(b.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc == right);
}

static void test_classic_keeps_native_drift_pair(void)
{
    ApsisState s;
    ApsisStringDsp dsp;
    ApsisStringPitchMemory memory;
    ApsisStringMotionPlayback p;
    uint32_t foundation;

    defaults(&s);
    s.string.surface_mode = APSIS_STRING_CLASSIC;
    s.drift.amount_q15 = APSIS_Q15_MILLI(800);
    s.drift.depth_q15 = APSIS_Q15_MILLI(700);
    p.left_pitch_milli = 0;
    p.right_pitch_milli = 7000;
    p.left_split_q15 = s.string.left_split_q15;
    p.right_split_q15 = s.string.right_split_q15;
    p.phase_q16 = 0u;

    apsis_string_dsp_reset(&dsp);
    apsis_string_pitch_memory_reset(&memory);
    apsis_string_dsp_update(&dsp, &s, &memory, 1u);
    foundation = dsp.voice[APSIS_STRING_DSP_FOUNDATION_LO].target_phase_inc;
    apsis_string_motion_dsp_override(&dsp, &s, &p);

    assert(dsp.voice[APSIS_STRING_DSP_FOUNDATION_LO].target_phase_inc == foundation);
    assert(dsp.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc > 0u);
    assert(dsp.voice[APSIS_STRING_DSP_PRIMARY_B].target_phase_inc >
           dsp.voice[APSIS_STRING_DSP_PRIMARY_A].target_phase_inc);
}

int main(void)
{
    test_dual_override_only_primary();
    test_frozen_pitch_ignores_later_gravity_tuning();
    test_classic_keeps_native_drift_pair();
    puts("PASS: Phase 10B String Motion DSP pitch override");
    return 0;
}
