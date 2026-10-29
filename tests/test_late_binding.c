#include <assert.h>
#include <stdio.h>

#include "apsis_late_binding.h"

static void test_source_lead_constants(void)
{
    assert(APSIS_SPIRAL_BIND_LEAD_DESKTOP_MS == 80u);
    assert(APSIS_SPIRAL_BIND_LEAD_NARROW_MS == 110u);
    assert(APSIS_BLOOM_OCCURRENCE_LEAD_MS == 45u);
    assert(APSIS_BLOOM_BIND_LEAD_DESKTOP_MS == 50u);
    assert(APSIS_BLOOM_BIND_LEAD_NARROW_MS == 70u);
    assert(APSIS_SPIRAL_BIND_LEAD_NATIVE_MS == APSIS_SPIRAL_BIND_LEAD_DESKTOP_MS);
    assert(APSIS_BLOOM_BIND_LEAD_NATIVE_MS == APSIS_BLOOM_BIND_LEAD_DESKTOP_MS);
}

static void test_bloom_target_projects_only_pending_degree_ring(void)
{
    ApsisState s;
    ApsisLatePendingGravity pending;
    ApsisBloomHarmony h;

    apsis_state_default(&s);
    s.gravity.tonic = 5u;
    s.gravity.scale = APSIS_SCALE_LYDIAN;
    s.gravity.degree = 1u;
    s.gravity.voicing = APSIS_GRAVITY_ANCHOR;
    s.gravity.inversion = 2u;
    s.gravity.voices = 5u;

    pending.active = 1u;
    pending.degree = 6u;
    pending.ring = APSIS_GRAVITY_AIR;
    pending.target_sample = 1000u;

    /* Study 126 uses p.at <= when + 0.0005. At 44.1 kHz the integer native
     * tolerance is 22 samples: 23 samples early is not visible, 22 is. */
    h = apsis_late_bloom_harmony_at(&s, &pending, 977u, 44100u);
    assert(h.tonic == 5u && h.scale == APSIS_SCALE_LYDIAN);
    assert(h.degree == 1u && h.voicing == APSIS_GRAVITY_ANCHOR);
    assert(h.inversion == 2u && h.voices == 5u);

    h = apsis_late_bloom_harmony_at(&s, &pending, 978u, 44100u);
    assert(h.degree == 6u && h.voicing == APSIS_GRAVITY_AIR);
    assert(h.tonic == 5u && h.scale == APSIS_SCALE_LYDIAN);
    assert(h.inversion == 2u && h.voices == 5u);

    h = apsis_late_bloom_harmony_at(&s, &pending, 1000u, 44100u);
    assert(h.degree == 6u && h.voicing == APSIS_GRAVITY_AIR);
    assert(h.tonic == 5u && h.scale == APSIS_SCALE_LYDIAN);
    assert(h.inversion == 2u && h.voices == 5u);

    pending.target_sample = 1022u;
    h = apsis_late_bloom_harmony_at(&s, &pending, 1000u, 44100u);
    assert(h.degree == 6u && h.voicing == APSIS_GRAVITY_AIR);
    pending.target_sample = 1023u;
    h = apsis_late_bloom_harmony_at(&s, &pending, 1000u, 44100u);
    assert(h.degree == 1u && h.voicing == APSIS_GRAVITY_ANCHOR);
}

static void test_spiral_rebind_uses_current_sequence_register_tuning(void)
{
    ApsisState s;
    ApsisSpiralLateBinding b;

    apsis_state_default(&s);
    s.gravity.scale_lock = 0u;
    s.gravity.tonic = 0u;
    s.spiral.follow_frame = 0u;
    s.spiral.route_count = 3u;
    s.spiral.route[0] = 4u;
    s.spiral.route[1] = 7u;
    s.spiral.route[2] = 11u;
    s.spiral.register_oct = 0;
    s.temper.mode = APSIS_TUNING_EQUAL;
    s.temper.strength_q15 = APSIS_Q15_ONE;

    assert(apsis_late_spiral_resolve(&s, 2u, &b) == 1u);
    assert(b.valid == 1u && b.route_index == 2u && b.step == 11);
    assert(b.tuning_deviation_milli == 0);

    /* Occurrence identity is ordinal 2. At bind the route is shorter, so the
     * frozen ordinal is reduced modulo the *current* sequence length. */
    s.spiral.route_count = 2u;
    s.spiral.route[0] = 4u;
    s.spiral.route[1] = 9u;
    s.spiral.register_oct = 1;
    s.temper.mode = APSIS_TUNING_JUST;
    assert(apsis_late_spiral_resolve(&s, 2u, &b) == 1u);
    assert(b.route_index == 0u);
    assert(b.step == 16); /* route[0] 4 + current register octave */
    assert(b.tuning_mode == APSIS_TUNING_JUST);
    assert(b.tuning_deviation_milli != 0);

    s.spiral.route_count = 0u;
    assert(apsis_late_spiral_resolve(&s, 2u, &b) == 0u);
    assert(b.valid == 0u);
}

static void test_spiral_has_no_pending_gravity_projection(void)
{
    ApsisState s;
    ApsisLatePendingGravity pending;
    ApsisSpiralLateBinding before, after;

    apsis_state_default(&s);
    s.gravity.degree = 0u;
    s.gravity.voicing = APSIS_GRAVITY_COLOUR;
    s.spiral.follow_frame = 1u;
    s.spiral.route_count = 1u;
    s.spiral.route[0] = 0u;

    pending.active = 1u;
    pending.degree = 4u;
    pending.ring = APSIS_GRAVITY_AIR;
    pending.target_sample = 10000u;

    /* Spiral resolver intentionally has no pending parameter. Merely creating
     * a pending candidate cannot change its result before actual commit. */
    assert(apsis_late_spiral_resolve(&s, 0u, &before) == 1u);
    assert(s.gravity.degree == 0u);
    assert(pending.active == 1u);
    assert(apsis_late_spiral_resolve(&s, 0u, &after) == 1u);
    assert(before.step == after.step);

    s.gravity.degree = pending.degree;
    s.gravity.voicing = pending.ring;
    assert(apsis_late_spiral_resolve(&s, 0u, &after) == 1u);
    assert(after.step != before.step);
}

static ApsisBloomPlan fixed_bloom_plan(void)
{
    ApsisBloomPlan p = {0};
    p.count = 3u;
    p.tone[0] = 0u; p.octave[0] = 0;
    p.tone[1] = 1u; p.octave[1] = 0;
    p.tone[2] = 2u; p.octave[2] = 1; /* occurrence-frozen register/octave */
    return p;
}

static void test_bloom_petals_reread_target_harmony_with_frozen_plan(void)
{
    ApsisState s;
    ApsisLatePendingGravity pending;
    ApsisBloomPlan plan = fixed_bloom_plan();
    ApsisBloomLateBinding early, late;

    apsis_state_default(&s);
    s.gravity.tonic = 0u;
    s.gravity.scale = APSIS_SCALE_DORIAN;
    s.gravity.degree = 0u;
    s.gravity.voicing = APSIS_GRAVITY_COLOUR;
    s.bloom.depth = APSIS_BLOOM_DEPTH_TRIAD;
    s.bloom.tuning_mode = APSIS_BLOOM_TUNING_EQUAL;
    s.bloom.tuning_amount_q15 = APSIS_Q15_ONE;

    pending.active = 1u;
    pending.degree = 3u;
    pending.ring = APSIS_GRAVITY_AIR;
    pending.target_sample = 2000u;

    assert(apsis_late_bloom_resolve(&s, &plan, 2u, &pending, 1900u, 44100u, &early) == 1u);
    assert(early.harmony.degree == 0u && early.harmony.voicing == APSIS_GRAVITY_COLOUR);
    assert(early.plan_octave == 1);

    assert(apsis_late_bloom_resolve(&s, &plan, 2u, &pending, 2000u, 44100u, &late) == 1u);
    assert(late.harmony.degree == 3u && late.harmony.voicing == APSIS_GRAVITY_AIR);
    assert(late.plan_octave == 1); /* plan/register did not get recomputed */
    assert(late.step != early.step);
}

static void test_bloom_petals_reread_tuning_but_not_plan_register(void)
{
    ApsisState s;
    ApsisBloomPlan plan = fixed_bloom_plan();
    ApsisBloomLateBinding a, b;

    apsis_state_default(&s);
    s.gravity.tonic = 0u;
    s.gravity.scale = APSIS_SCALE_MAJOR;
    s.bloom.depth = APSIS_BLOOM_DEPTH_TRIAD;
    s.bloom.tuning_reference = APSIS_BLOOM_REF_TONIC;
    s.bloom.tuning_mode = APSIS_BLOOM_TUNING_FOLLOW;
    s.bloom.tuning_amount_q15 = APSIS_Q15_ONE;
    s.bloom.register_oct = -1; /* must not override the already-frozen plan */
    s.temper.mode = APSIS_TUNING_EQUAL;
    s.temper.strength_q15 = APSIS_Q15_ONE;

    assert(apsis_late_bloom_resolve(&s, &plan, 1u, 0, 1000u, 44100u, &a) == 1u);
    assert(a.plan_octave == 0);
    assert(a.tuning_deviation_milli == 0);

    s.bloom.register_oct = 1;
    s.temper.mode = APSIS_TUNING_JUST;
    assert(apsis_late_bloom_resolve(&s, &plan, 1u, 0, 2000u, 44100u, &b) == 1u);
    assert(b.plan_octave == 0); /* current register still cannot rewrite safePlan */
    assert(b.step == a.step);
    assert(b.tuning_mode == APSIS_TUNING_JUST);
    assert(b.tuning_deviation_milli != a.tuning_deviation_milli);
}

int main(void)
{
    test_source_lead_constants();
    test_bloom_target_projects_only_pending_degree_ring();
    test_spiral_rebind_uses_current_sequence_register_tuning();
    test_spiral_has_no_pending_gravity_projection();
    test_bloom_petals_reread_target_harmony_with_frozen_plan();
    test_bloom_petals_reread_tuning_but_not_plan_register();
    puts("PASS: APSIS Study 126 late-binding contract");
    return 0;
}
