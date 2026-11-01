#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_motion.h"

static void defaults(ApsisState *s)
{
    apsis_state_default(s);
    s->clock.n = 4u;
    s->string.motion_loop_length = 1u;
}

static void test_storage_budget(void)
{
    assert(sizeof(ApsisStringMotionPoint) == 8u);
    assert(sizeof(ApsisStringMotionStore) <= 19216u);
    assert(sizeof(ApsisStringMotionRuntime) < 128u);
}

static void test_target_resolution_and_arm(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    defaults(&s);
    apsis_string_motion_reset(&store, &rt);

    s.string.surface_mode = APSIS_STRING_CLASSIC;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_TARGET_LEFT) ==
           APSIS_STRING_MOTION_TARGET_RIGHT);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 1u;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_TARGET_LEFT) ==
           APSIS_STRING_MOTION_TARGET_BOTH);
    s.string.linked = 0u;
    assert(apsis_string_motion_resolve_target(&s, APSIS_STRING_MOTION_TARGET_LEFT) ==
           APSIS_STRING_MOTION_TARGET_LEFT);

    s.string.motion_loop_play = 1u;
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_LEFT) == 1u);
    assert(rt.armed == 1u);
    assert(s.string.motion_loop_play == 0u);
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_LEFT) == 0u);
    assert(rt.armed == 0u);
}

static void test_step_one_recording_length(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    uint64_t t = 10000u;
    const uint32_t interval = 1000u;
    uint8_t cycle, step;

    defaults(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 1u;
    s.string.motion_loop_length = 2u;
    apsis_string_motion_reset(&store, &rt);
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_BOTH));

    apsis_string_motion_pulse(&store, &rt, &s, 2u, t, interval);
    assert(rt.armed && !rt.recording);
    t += interval;
    apsis_string_motion_pulse(&store, &rt, &s, 0u, t, interval);
    assert(!rt.armed && rt.recording);
    assert(rt.record_cycles == 0u);

    for (cycle = 0u; cycle < 2u; ++cycle) {
        for (step = 1u; step < 4u; ++step) {
            t += interval;
            apsis_string_motion_pulse(&store, &rt, &s, step, t, interval);
            assert(rt.recording);
        }
        t += interval;
        apsis_string_motion_pulse(&store, &rt, &s, 0u, t, interval);
        if (cycle == 0u) assert(rt.recording);
    }
    assert(!rt.recording);
    assert(s.string.motion_loop_play == 1u);
    assert(store.left.count > 1u && store.right.count > 1u);
    assert(store.left.point[store.left.count - 1u].phase_q16 == 65535u);
    assert(store.right.point[store.right.count - 1u].phase_q16 == 65535u);
}

static void test_right_overdub_preserves_left(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    ApsisStringMotionLane before;
    uint64_t t = 5000u;

    defaults(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 0u;
    apsis_string_motion_reset(&store, &rt);
    store.left.count = 2u;
    store.left.point[0].phase_q16 = 0u;
    store.left.point[0].split_q15 = APSIS_Q15_MILLI(420);
    store.left.point[0].pitch_milli = 1000;
    store.left.point[1].phase_q16 = 65535u;
    store.left.point[1].split_q15 = APSIS_Q15_MILLI(460);
    store.left.point[1].pitch_milli = 3000;
    before = store.left;

    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_RIGHT));
    apsis_string_motion_pulse(&store, &rt, &s, 0u, t, 1000u);
    assert(rt.recording);
    assert(memcmp(&before, &store.left, sizeof(before)) == 0);
    s.string.right_split_q15 = APSIS_Q15_MILLI(680);
    t += 500u;
    assert(apsis_string_motion_capture(&store, &rt, &s, t, 0u));
    assert(memcmp(&before, &store.left, sizeof(before)) == 0);
    t += 500u;
    apsis_string_motion_pulse(&store, &rt, &s, 1u, t, 1000u);
    apsis_string_motion_pulse(&store, &rt, &s, 2u, t + 1000u, 1000u);
    apsis_string_motion_pulse(&store, &rt, &s, 3u, t + 2000u, 1000u);
    apsis_string_motion_pulse(&store, &rt, &s, 0u, t + 3000u, 1000u);
    assert(!rt.recording);
    assert(memcmp(&before, &store.left, sizeof(before)) == 0);
    assert(store.right.count > 1u);
}

static void test_frozen_pitch_context_survives_gravity(void)
{
    ApsisState s;
    ApsisStringMotionPitchContext frozen, changed;
    int32_t a, b, live;
    defaults(&s);
    s.string.surface_mode = APSIS_STRING_DUAL;
    s.string.linked = 0u;
    s.string.right_follow_gravity = 1u;
    s.string.right_anchor = APSIS_ANCHOR_FIFTH;
    apsis_string_motion_freeze_pitch(&s, &frozen);
    a = apsis_string_motion_pitch_for_split(&frozen, APSIS_SIDE_RIGHT,
                                            s.string.right_split_q15);

    s.gravity.degree = 5u;
    s.gravity.tonic = 7u;
    s.temper.mode = APSIS_TUNING_JUST;
    s.temper.strength_q15 = APSIS_Q15_ONE;
    b = apsis_string_motion_pitch_for_split(&frozen, APSIS_SIDE_RIGHT,
                                            s.string.right_split_q15);
    apsis_string_motion_freeze_pitch(&s, &changed);
    live = apsis_string_motion_pitch_for_split(&changed, APSIS_SIDE_RIGHT,
                                               s.string.right_split_q15);
    assert(a == b);
    assert(live != a);
}

static void test_recording_ownership_restore(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    defaults(&s);
    apsis_string_motion_reset(&store, &rt);
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_BOTH));
    apsis_string_motion_pulse(&store, &rt, &s, 0u, 1000u, 1000u);
    rt.owned_left_q15 = APSIS_Q15_MILLI(390);
    rt.owned_right_q15 = APSIS_Q15_MILLI(710);
    s.string.left_split_q15 = APSIS_Q15_MILLI(470);
    s.string.right_split_q15 = APSIS_Q15_MILLI(530);
    apsis_string_motion_restore_owned(&rt, &s);
    assert(s.string.left_split_q15 == APSIS_Q15_MILLI(390));
    assert(s.string.right_split_q15 == APSIS_Q15_MILLI(710));
}

static void test_capture_throttle(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    uint16_t count;
    defaults(&s);
    apsis_string_motion_reset(&store, &rt);
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_BOTH));
    apsis_string_motion_pulse(&store, &rt, &s, 0u, 10000u, 2000u);
    count = store.right.count;
    s.string.right_split_q15 = APSIS_Q15_MILLI(600);
    assert(apsis_string_motion_capture(&store, &rt, &s, 10010u, 0u) == 0u);
    assert(store.right.count == count);
    assert(apsis_string_motion_capture(&store, &rt, &s,
        10000u + APSIS_STRING_MOTION_RATE / APSIS_STRING_MOTION_CAPTURE_HZ, 0u) == 1u);
    assert(store.right.count > count);
}

static void test_pitch_hold_continuous_and_manual_resume(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    ApsisStringMotionPlayback out;
    defaults(&s);
    apsis_string_motion_reset(&store, &rt);
    apsis_string_motion_freeze_pitch(&s, &rt.pitch);

    store.right.count = 3u;
    store.right.point[0].phase_q16 = 0u;
    store.right.point[0].split_q15 = APSIS_Q15_MILLI(520);
    store.right.point[0].pitch_milli = 0;
    store.right.point[1].phase_q16 = 32768u;
    store.right.point[1].split_q15 = APSIS_Q15_MILLI(700);
    store.right.point[1].pitch_milli = 12000;
    store.right.point[2].phase_q16 = 65535u;
    store.right.point[2].split_q15 = APSIS_Q15_MILLI(520);
    store.right.point[2].pitch_milli = 0;
    s.string.motion_loop_play = 1u;
    rt.play_pulse_index = 1u;
    rt.play_pulse_sample = 1000u;
    rt.pulse_interval_samples = 1000u;

    s.string.motion_loop_pitch_continuous = 0u;
    assert(apsis_string_motion_playback(&store, &rt, &s, 1000u, &out));
    assert(out.phase_q16 >= 16380u && out.phase_q16 <= 16390u);
    assert(out.right_pitch_milli == 0);

    s.string.motion_loop_pitch_continuous = 1u;
    assert(apsis_string_motion_playback(&store, &rt, &s, 1000u, &out));
    assert(out.right_pitch_milli > 5900 && out.right_pitch_milli < 6100);

    apsis_string_motion_manual_override(&rt, 1u);
    assert(!apsis_string_motion_playback(&store, &rt, &s, 1500u, &out));
    apsis_string_motion_manual_override(&rt, 0u);
    assert(apsis_string_motion_playback(&store, &rt, &s, 1500u, &out));
    assert(out.phase_q16 > 16390u); /* resumed current phase; did not restart */
}

int main(void)
{
    test_storage_budget();
    test_target_resolution_and_arm();
    test_step_one_recording_length();
    test_right_overdub_preserves_left();
    test_frozen_pitch_context_survives_gravity();
    test_recording_ownership_restore();
    test_capture_throttle();
    test_pitch_hold_continuous_and_manual_resume();
    puts("PASS: Phase 10A String Motion source contract");
    return 0;
}
