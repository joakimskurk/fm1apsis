#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_gravity.h"
#include "apsis_bloom.h"

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

static void test_gravity_point_defaults(void)
{
    ApsisState s;
    const ApsisGravityPoint *p;
    apsis_state_default(&s);

    assert(APSIS_GRAVITY_POINTS == 24u);
    assert(s.gravity.selected_ring == APSIS_GRAVITY_ANCHOR);
    assert(s.gravity.selected_point_degree == 0u);

    p = apsis_gravity_point_const(&s.gravity, 0u, APSIS_GRAVITY_ANCHOR);
    assert(p != 0);
    assert(p->enabled == 1u && p->offset == 0 && p->octave == 0);
    assert(p->level_q15 == APSIS_Q15_MILLI(785));
    assert(p->brightness_q15 == APSIS_Q15_MILLI(258));
    assert(p->attack_ms == 50u);
    assert(p->release_ms == 920u);

    p = apsis_gravity_point_const(&s.gravity, 7u, APSIS_GRAVITY_AIR);
    assert(p != 0);
    assert(p->level_q15 == APSIS_Q15_MILLI(625));
    assert(p->brightness_q15 == APSIS_Q15_MILLI(578));
    assert(p->attack_ms == 20u);
    assert(p->release_ms == 1490u);

    {
        ApsisGravityPoint direct = apsis_gravity_point_default(7u, APSIS_GRAVITY_AIR);
        assert(direct.level_q15 == p->level_q15);
        assert(direct.brightness_q15 == p->brightness_q15);
        assert(direct.attack_ms == p->attack_ms);
        assert(direct.release_ms == p->release_ms);
    }
}

static void test_gravity_point_steps_and_sanitize(void)
{
    ApsisState s;
    ApsisGravityPoint *p;
    apsis_state_default(&s);

    assert(apsis_gravity_point_step(&s, 0u, APSIS_GRAVITY_ANCHOR) == 0);
    assert(apsis_gravity_point_step(&s, 0u, APSIS_GRAVITY_COLOUR) == 3);
    assert(apsis_gravity_point_step(&s, 7u, APSIS_GRAVITY_AIR) == 26);

    p = apsis_gravity_point(&s.gravity, 7u, APSIS_GRAVITY_AIR);
    assert(p != 0);
    p->offset = 2;
    p->octave = 1;
    assert(apsis_gravity_point_step(&s, 7u, APSIS_GRAVITY_AIR) == 41);

    p->enabled = 7u;
    p->offset = 9;
    p->octave = -9;
    p->level_q15 = 0u;
    p->brightness_q15 = 65535u;
    p->attack_ms = 1u;
    p->release_ms = 65535u;
    apsis_gravity_sanitize_point(p);
    assert(p->enabled == 1u);
    assert(p->offset == 2);
    assert(p->octave == -1);
    assert(p->level_q15 == APSIS_Q15_MILLI(150));
    assert(p->brightness_q15 == APSIS_Q15_MILLI(780));
    assert(p->attack_ms == 12u);
    assert(p->release_ms == 3000u);

    apsis_gravity_reset_point(&s.gravity, 7u, APSIS_GRAVITY_AIR);
    p = apsis_gravity_point(&s.gravity, 7u, APSIS_GRAVITY_AIR);
    assert(p->offset == 0 && p->octave == 0);
    assert(p->level_q15 == APSIS_Q15_MILLI(625));
    assert(p->release_ms == 1490u);
}

static void test_bloom_defaults_and_base_harmony(void)
{
    ApsisState s;
    ApsisBloomHarmony h;
    int16_t steps[APSIS_BLOOM_VOICES_MAX] = {0};
    uint8_t n;
    apsis_state_default(&s);

    assert(s.bloom.depth == APSIS_BLOOM_DEPTH_FOLLOW);
    assert(s.bloom.voicing_mode == APSIS_BLOOM_VOICING_FOLLOW);
    assert(s.bloom.inversion_follow == 1u && s.bloom.inversion == 0u);
    assert(s.bloom.tuning_mode == APSIS_BLOOM_TUNING_FOLLOW);
    assert(s.bloom.tuning_reference == APSIS_BLOOM_REF_CHORD);
    assert(s.temper.send_q15 == APSIS_Q15_MILLI(420));
    assert(s.temper.sound == 1u);
    assert(s.temper.level_q15 == APSIS_Q15_MILLI(580));

    apsis_bloom_harmony_from_state(&s, &h);
    assert(h.tonic == 0u && h.scale == APSIS_SCALE_DORIAN && h.degree == 0u);
    assert(h.voicing == APSIS_GRAVITY_COLOUR && h.inversion == 0u && h.voices == 4u);
    assert(apsis_bloom_voice_count(&s, &h) == 4u);

    n = apsis_bloom_base_chord_steps(&s, &h, steps);
    assert(n == 4u);
    assert(steps[0] == 0 && steps[1] == 3 && steps[2] == 7 && steps[3] == 10);

    s.bloom.depth = APSIS_BLOOM_DEPTH_TRIAD;
    assert(apsis_bloom_voice_count(&s, &h) == 3u);
    s.bloom.depth = APSIS_BLOOM_DEPTH_SEVENTH;
    assert(apsis_bloom_voice_count(&s, &h) == 4u);
    s.bloom.depth = APSIS_BLOOM_DEPTH_NINTH;
    assert(apsis_bloom_voice_count(&s, &h) == 5u);

    apsis_state_default(&s);
    s.gravity.degree = 7u;
    apsis_bloom_harmony_from_state(&s, &h);
    n = apsis_bloom_base_chord_steps(&s, &h, steps);
    assert(n == 4u);
    assert(steps[0] == 12 && steps[1] == 15 && steps[2] == 19 && steps[3] == 22);
}

static void test_bloom_legacy_plan(void)
{
    ApsisState s;
    ApsisBloomHarmony h;
    ApsisBloomPlan p;
    int16_t steps[APSIS_BLOOM_VOICES_MAX] = {0};
    uint8_t n;
    apsis_state_default(&s);
    apsis_bloom_harmony_from_state(&s, &h);

    p = apsis_bloom_legacy_plan(&s, &h);
    assert(p.count == 4u);
    assert(p.tone[0] == 0u && p.tone[1] == 1u && p.tone[2] == 2u && p.tone[3] == 3u);
    assert(p.octave[0] == 0 && p.octave[1] == 0 && p.octave[2] == 1 && p.octave[3] == 1);
    n = apsis_bloom_plan_steps(&s, &h, &p, steps);
    assert(n == 4u);
    assert(steps[0] == 0 && steps[1] == 3 && steps[2] == 19 && steps[3] == 22);

    assert(iabs32((int32_t)apsis_bloom_spread_target_q15(&s, &h) - 17379) <= 1);
}

static void test_bloom_voice_leading(void)
{
    ApsisState s;
    ApsisBloomHarmony h;
    ApsisBloomVoicingState vs;
    ApsisBloomPlan p;
    int16_t steps[APSIS_BLOOM_VOICES_MAX] = {0};
    uint8_t n;
    apsis_state_default(&s);
    apsis_bloom_harmony_from_state(&s, &h);
    apsis_bloom_voicing_reset(&vs);

    p = apsis_bloom_choose_voicing(&s, &h, &vs, 0u, 0x80000000u, 0u);
    assert(p.count == 4u);
    n = apsis_bloom_plan_steps(&s, &h, &p, steps);
    assert(n == 4u);
    assert(steps[0] == -12 && steps[1] == -9 && steps[2] == -5 && steps[3] == 10);
    assert(vs.serial == 0u && vs.last_count == 0u);

    p = apsis_bloom_choose_voicing(&s, &h, &vs, 1u, 0x80000000u, 0u);
    assert(p.count == 4u);
    assert(vs.serial == 1u && vs.last_count == 4u && vs.drift_milli == 0);
    assert(vs.plan.count == 4u);
    assert(vs.last_steps[0] == -12 && vs.last_steps[1] == -9 &&
           vs.last_steps[2] == -5 && vs.last_steps[3] == 10);

    n = apsis_bloom_chord_steps(&s, &h, &vs, steps);
    assert(n == 4u);
    assert(steps[0] == -12 && steps[1] == -9 && steps[2] == -5 && steps[3] == 10);

    apsis_bloom_voicing_reset(&vs);
    n = apsis_bloom_chord_steps(&s, &h, &vs, steps);
    assert(n == 4u);
    assert(steps[0] == 0 && steps[1] == 3 && steps[2] == 19 && steps[3] == 22);
}

static void test_bloom_corrupt_inversion_is_bounded(void)
{
    ApsisState s;
    ApsisBloomHarmony h;
    ApsisBloomVoicingState vs;
    ApsisBloomPlan p;
    apsis_state_default(&s);
    apsis_bloom_harmony_from_state(&s, &h);
    h.inversion = 255u;
    apsis_bloom_voicing_reset(&vs);
    p = apsis_bloom_choose_voicing(&s, &h, &vs, 0u, 0x80000000u, 0u);
    assert(p.count == 4u);
}

int main(void)
{
    test_gravity_point_defaults();
    test_gravity_point_steps_and_sanitize();
    test_bloom_defaults_and_base_harmony();
    test_bloom_legacy_plan();
    test_bloom_voice_leading();
    test_bloom_corrupt_inversion_is_bounded();
    puts("PASS: APSIS source-derived Gravity points and Bloom voice-leading");
    return 0;
}
