#include <assert.h>
#include <stdio.h>

#include "apsis_state.h"
#include "apsis_harmony.h"
#include "apsis_string.h"
#include "apsis_drift.h"
#include "apsis_clock.h"

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

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
    s.gravity.inversion = 2u;
    n = apsis_gravity_chord_steps(&s.gravity, steps);
    assert(n == 4u);
    assert(steps[0] == 3 && steps[1] == 7 && steps[2] == 12 && steps[3] == 12);

    apsis_state_default(&s);
    s.gravity.degree = 7u;
    n = apsis_gravity_chord_steps(&s.gravity, steps);
    assert(n == 4u);
    assert(steps[0] == 12 && steps[1] == 15 && steps[2] == 31 && steps[3] == 34);
}

static void test_tuning(void)
{
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_EQUAL, APSIS_Q15_ONE) == 100000);
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_JUST, APSIS_Q15_ONE) == 111731);
    assert(apsis_tuning_deviation_milli(1u, 0u, APSIS_TUNING_JUST, APSIS_Q15_ONE) == 11731);
    assert(apsis_tuning_cents_milli(7u, 0u, APSIS_TUNING_PYTH, APSIS_Q15_ONE) == 701955);
    assert(apsis_tuning_cents_milli(1u, 0u, APSIS_TUNING_JUST, 0u) == 100000);
}

static void test_string_ratio_and_mapping(void)
{
    ApsisStringRatio r;
    int32_t mag;
    r = apsis_string_ratio(APSIS_STRING_SPLIT_MID_Q15);
    assert(r.raw_ratio_q16 == 65536u);
    assert(r.near_simple == 1u && r.numerator == 1u && r.denominator == 1u);
    r = apsis_string_ratio(APSIS_Q15_MILLI(667));
    assert(r.near_simple == 1u && r.numerator == 2u && r.denominator == 1u);
    mag = apsis_string_magnitude_milli(APSIS_Q15_MILLI(530), APSIS_Q15_ONE);
    assert(iabs32(mag - 2080) <= 4);
    assert(apsis_string_magnitude_milli(APSIS_Q15_MILLI(800), APSIS_Q15_ONE) >= 23995);
    assert(apsis_string_magnitude_milli(APSIS_STRING_SPLIT_MAX_Q15, APSIS_Q15_ONE) == 36000);
}

static void test_string_split_link(void)
{
    ApsisState s;
    apsis_state_default(&s);
    apsis_string_set_side_split(&s.string, APSIS_SIDE_LEFT, APSIS_Q15_MILLI(400), 1u);
    assert(s.string.left_split_q15 == APSIS_Q15_MILLI(400));
    assert(iabs32((int32_t)s.string.right_split_q15 - (int32_t)APSIS_Q15_MILLI(600)) <= 1);
    assert(s.string.split_q15 == s.string.right_split_q15);
}

static void test_string_pitch(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    int32_t raw, target;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    assert(apsis_string_anchor_step(&s, APSIS_SIDE_LEFT) == 0);
    assert(apsis_string_anchor_step(&s, APSIS_SIDE_RIGHT) == 7);
    raw = apsis_string_raw_step_milli(&s, APSIS_SIDE_RIGHT);
    assert(iabs32(raw - 9080) <= 4);
    target = apsis_string_target_step_milli(&s, APSIS_SIDE_RIGHT, &mem, 1u);
    assert(target == 9000);
    s.string.quantize = 0u;
    target = apsis_string_target_step_milli(&s, APSIS_SIDE_RIGHT, &mem, 1u);
    assert(target == raw);
    assert(mem.right_valid == 0u);
    apsis_state_default(&s);
    s.gravity.degree = 7u;
    assert(apsis_string_anchor_step(&s, APSIS_SIDE_RIGHT) == 19);
    assert(apsis_string_target_step_milli(&s, APSIS_SIDE_RIGHT, &mem, 0u) == 21000);
}

static void test_string_hysteresis(void)
{
    ApsisState s;
    ApsisStringPitchMemory mem;
    apsis_state_default(&s);
    apsis_string_pitch_memory_reset(&mem);
    s.string.harmony = APSIS_HARMONY_CHROMATIC;
    s.string.right_anchor = APSIS_ANCHOR_ROOT;
    mem.right_target_step = 1;
    mem.right_valid = 1u;
    s.string.right_split_q15 = APSIS_Q15_MILLI(522);
    assert(apsis_string_target_step_milli(&s, APSIS_SIDE_RIGHT, &mem, 1u) == 1000);
    s.string.right_split_q15 = APSIS_Q15_MILLI(523);
    assert(apsis_string_target_step_milli(&s, APSIS_SIDE_RIGHT, &mem, 1u) == 2000);
}

static void test_drift(void)
{
    ApsisState s;
    apsis_state_default(&s);
    assert(apsis_drift_delta_millihz(0u) == 50u);
    assert(apsis_drift_delta_millihz(APSIS_Q15_ONE) == 4000u);
    assert(iabs32((int32_t)apsis_drift_delta_millihz(s.drift.amount_q15) - 994) <= 1);
    assert(iabs32((int32_t)apsis_drift_audible_delta_millihz(&s.drift) - 587) <= 1);
    s.drift.sound = 0u;
    assert(apsis_drift_audible_delta_millihz(&s.drift) == 0u);
}

static void test_clock(void)
{
    ApsisState s;
    ApsisClockPulse p;
    uint8_t groups[APSIS_CLOCK_GROUPS_MAX] = {0};
    uint8_t n;
    apsis_state_default(&s);

    n = apsis_clock_groups(&s.clock, groups);
    assert(n == 4u);
    assert(groups[0] == 3u && groups[1] == 3u && groups[2] == 3u && groups[3] == 3u);
    assert(apsis_clock_base_interval_us(&s.clock) == 416667u);
    assert(apsis_clock_interval_us(&s.clock, 0u) == 416667u);

    s.clock.swing_q15 = APSIS_Q15_MILLI(100);
    /* Q15 cannot encode 0.10 exactly; native timing must remain within single-digit microseconds of browser float timing. */
    assert(iabs32((int32_t)apsis_clock_interval_us(&s.clock, 0u) - 375000) <= 8);
    assert(iabs32((int32_t)apsis_clock_interval_us(&s.clock, 1u) - 458333) <= 8);

    p = apsis_clock_pulse(&s.clock, 0u, 0u);
    assert(p.main_on == 1u && p.accent == 1u && p.step_one_resync == 0u);
    p = apsis_clock_pulse(&s.clock, 3u, 3u);
    assert(p.main_on == 1u && p.accent == 1u);
    p = apsis_clock_pulse(&s.clock, 2u, 2u);
    assert(p.main_on == 1u && p.accent == 0u);

    apsis_clock_set_length(&s.clock, 16u);
    assert(s.clock.n == 16u);
    assert((s.clock.main_mask & 0xFFFFu) == 0xFFFFu);
    apsis_clock_set_length(&s.clock, 8u);
    assert(s.clock.n == 8u);
    assert((s.clock.main_mask & ~0xFFu) == 0u);
    assert(s.clock.bounds[2] == 0u);
}

int main(void)
{
    test_defaults();
    test_scales();
    test_quantize();
    test_gravity();
    test_tuning();
    test_string_ratio_and_mapping();
    test_string_split_link();
    test_string_pitch();
    test_string_hysteresis();
    test_drift();
    test_clock();
    puts("PASS: APSIS source-derived core defaults/harmony/string/drift/clock");
    return 0;
}
