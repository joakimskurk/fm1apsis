#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_harmony.h"

static void test_defaults(void)
{
    ApsisState s;
    apsis_state_default(&s);

    assert(s.version == APSIS_STATE_VERSION);
    assert(s.string.surface_mode == APSIS_STRING_DUAL);
    assert(s.string.left_split_q15 == APSIS_Q15_MILLI(470));
    assert(s.string.right_split_q15 == APSIS_Q15_MILLI(530));
    assert(s.string.left_anchor == APSIS_ANCHOR_ROOT);
    assert(s.string.right_anchor == APSIS_ANCHOR_FIFTH);
    assert(s.string.filter_cutoff_hz == 6200u);
    assert(s.string.filter_attractor_seed == 317u);
    assert(s.string.filter_attractor_gates == 0x9535u);
    assert(s.string.send_q15 == APSIS_Q15_MILLI(720));

    assert(s.drift.amount_q15 == APSIS_Q15_MILLI(420));
    assert(s.drift.depth_q15 == APSIS_Q15_MILLI(720));

    assert(s.clock.n == 12u);
    assert(s.clock.bounds[0] == 3u && s.clock.bounds[1] == 6u && s.clock.bounds[2] == 9u);
    assert(s.clock.main_mask == 0x0FFFu);
    assert(s.clock.bpm == 72u);
    assert(s.clock.division_num == 1u && s.clock.division_den == 2u);

    assert(s.spiral.route_count == 0u);
    assert(s.spiral.attractor_seed == 911u);
    assert(s.spiral.attractor_gates == 0x2D2Du);
    assert(s.spiral.voice == APSIS_SPIRAL_THREAD);

    assert(s.bloom.cycles == 2u);
    assert(s.bloom.delay_ms == 60u);
    assert(s.bloom.length_ms == 1900u);
    assert(s.bloom.attack_ms == 18u);
    assert(s.bloom.voice == APSIS_BLOOM_VEIL);

    assert(s.temper.mode == APSIS_TUNING_EQUAL);
    assert(s.gravity.tonic == 0u);
    assert(s.gravity.scale == APSIS_SCALE_DORIAN);
    assert(s.gravity.voicing == APSIS_GRAVITY_COLOUR);
    assert(s.gravity.voices == 4u);
    assert(s.gravity.change_grid == 2u);

    assert(s.space.mix_q15 == APSIS_Q15_MILLI(480));
    assert(s.space.decay_ms == 8400u);
    assert(s.space.predelay_ms == 72u);
    assert(s.space.echo_time_ms == 362u);

    assert(s.mother.sync == 0u);
    assert(s.mother.horizon == 8u);
    assert(s.mother.mix_q15[0] == APSIS_Q15_ONE);
    assert(s.mother.mix_q15[4] == APSIS_Q15_ONE);

    assert(s.chain.default_cycles == 4u);
    assert(s.chain.scope_key_scale == 1u);
    assert(s.chain.scope_chord == 0u);
}

static void test_scales(void)
{
    const int8_t *dorian = apsis_scale_intervals(APSIS_SCALE_DORIAN);
    assert(dorian[0] == 0 && dorian[1] == 2 && dorian[2] == 3 && dorian[6] == 10);
    assert(apsis_scale_at(0u, APSIS_SCALE_DORIAN, 0) == 0);
    assert(apsis_scale_at(0u, APSIS_SCALE_DORIAN, 7) == 12);
    assert(apsis_scale_at(0u, APSIS_SCALE_DORIAN, -1) == -2);
    assert(apsis_scale_at(2u, APSIS_SCALE_DORIAN, 0) == 2);
}

static void test_quantize(void)
{
    assert(apsis_quantize_step_to_scale(1, 0u, APSIS_SCALE_DORIAN, 1u) == 0);
    assert(apsis_quantize_step_to_scale(4, 0u, APSIS_SCALE_DORIAN, 1u) == 3);
    assert(apsis_quantize_step_to_scale(4, 0u, APSIS_SCALE_DORIAN, 0u) == 4);
}

static void test_gravity(void)
{
    ApsisState s;
    int16_t ints[APSIS_GRAVITY_VOICES_MAX] = {0};
    int16_t steps[APSIS_GRAVITY_VOICES_MAX] = {0};
    uint8_t n;
    apsis_state_default(&s);

    n = apsis_gravity_ring_intervals(&s.gravity, ints);
    assert(n == 4u);
    assert(ints[0] == 0 && ints[1] == 3 && ints[2] == 7 && ints[3] == 10);

    n = apsis_gravity_chord_steps(&s.gravity, steps);
    assert(n == 4u);
    assert(steps[0] == 0 && steps[1] == 3 && steps[2] == 19 && steps[3] == 22);

    s.gravity.voicing = APSIS_GRAVITY_ANCHOR;
    s.gravity.inversion = 2u; /* source clamps ANCHOR inversion to one */
    n = apsis_gravity_chord_steps(&s.gravity, steps);
    assert(n == 4u);
    assert(steps[0] == 3 && steps[1] == 7 && steps[2] == 12 && steps[3] == 19);
}

static void test_tuning(void)
{
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_EQUAL, APSIS_Q15_ONE) == 100000);
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_JUST, APSIS_Q15_ONE) == 111731);
    assert(apsis_tuning_deviation_milli(1u, 0u, APSIS_TUNING_JUST, APSIS_Q15_ONE) == 11731);
    assert(apsis_tuning_cents_milli(7u, 0u, APSIS_TUNING_PYTH, APSIS_Q15_ONE) == 701955);
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_JUST, 0u) == 100000);
}

int main(void)
{
    test_defaults();
    test_scales();
    test_quantize();
    test_gravity();
    test_tuning();
    puts("PASS: APSIS source-derived core defaults/harmony");
    return 0;
}
