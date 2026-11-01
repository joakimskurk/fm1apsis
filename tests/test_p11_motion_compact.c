#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_motion.h"

typedef struct {
    uint16_t split_q15;
    int32_t pitch_milli;
} RefSample;

static void defaults(ApsisState *s)
{
    apsis_state_default(s);
    s->clock.n = 4u;
    s->string.motion_loop_length = 1u;
    s->string.surface_mode = APSIS_STRING_DUAL;
    s->string.linked = 0u;
}

static void record_cycle(ApsisStringMotionStore *store,
                         ApsisStringMotionRuntime *rt,
                         ApsisState *s,
                         uint8_t target,
                         uint64_t *time,
                         const uint16_t split[4])
{
    const uint32_t interval = 22050u;
    uint8_t step;
    assert(apsis_string_motion_arm(rt, s, target));
    if (target == APSIS_STRING_MOTION_TARGET_LEFT)
        s->string.left_split_q15 = split[0];
    else
        s->string.right_split_q15 = split[0];
    apsis_string_motion_pulse(store, rt, s, 0u, *time, interval);
    assert(rt->recording);

    for (step = 1u; step < 4u; ++step) {
        *time += interval;
        if (target == APSIS_STRING_MOTION_TARGET_LEFT)
            s->string.left_split_q15 = split[step];
        else
            s->string.right_split_q15 = split[step];
        apsis_string_motion_pulse(store, rt, s, step, *time, interval);
        assert(rt->recording);
    }
    *time += interval;
    apsis_string_motion_pulse(store, rt, s, 0u, *time, interval);
    assert(!rt->recording);
}

static RefSample reference_lane(const ApsisStringMotionLane *lane,
                                uint8_t side,
                                uint16_t phase_q16,
                                uint8_t continuous,
                                uint16_t fallback_split,
                                int32_t fallback_pitch)
{
    RefSample out;
    uint16_t hi, i;
    const ApsisStringMotionPoint *a, *b;
    uint32_t span, pos;
    int32_t split_delta, pa, pb;
    int64_t pitch_delta;

    if (!lane || !lane->count) {
        out.split_q15 = fallback_split;
        out.pitch_milli = fallback_pitch;
        return out;
    }
    if (lane->count == 1u) {
        out.split_q15 = lane->point[0].split_q15;
        out.pitch_milli = apsis_string_motion_pitch_for_split(
            &lane->pitch, side, lane->point[0].split_q15);
        return out;
    }

    hi = lane->count;
    for (i = 0u; i < lane->count; ++i) {
        if (lane->point[i].phase_q16 >= phase_q16) { hi = i; break; }
    }
    if (hi == lane->count) {
        a = &lane->point[lane->count - 1u];
        b = &lane->point[0];
        span = (uint32_t)(65535u - a->phase_q16) + b->phase_q16;
        pos = (uint32_t)(phase_q16 - a->phase_q16);
    } else if (hi == 0u) {
        a = &lane->point[lane->count - 1u];
        b = &lane->point[0];
        span = (uint32_t)(65535u - a->phase_q16) + b->phase_q16;
        pos = (uint32_t)phase_q16 + (65535u - a->phase_q16);
    } else {
        a = &lane->point[hi - 1u];
        b = &lane->point[hi];
        span = (uint32_t)b->phase_q16 - a->phase_q16;
        pos = (uint32_t)phase_q16 - a->phase_q16;
    }

    if (!span) {
        out.split_q15 = b->split_q15;
    } else {
        split_delta = (int32_t)b->split_q15 - a->split_q15;
        out.split_q15 = (uint16_t)((int32_t)a->split_q15 +
            (int32_t)(((int64_t)split_delta * pos + (int32_t)span / 2) /
                      (int32_t)span));
    }

    pa = apsis_string_motion_pitch_for_split(&lane->pitch, side, a->split_q15);
    pb = apsis_string_motion_pitch_for_split(&lane->pitch, side, b->split_q15);
    if (!continuous) {
        out.pitch_milli = pa;
    } else if (!span) {
        out.pitch_milli = pb;
    } else {
        pitch_delta = (int64_t)pb - pa;
        out.pitch_milli = pa +
            (int32_t)((pitch_delta * pos + (int32_t)span / 2) / (int32_t)span);
    }
    return out;
}

static void assert_playback_matches_reference(ApsisStringMotionStore *store,
                                              ApsisStringMotionRuntime *rt,
                                              ApsisState *s)
{
    static const uint16_t phase[] = {
        0u, 1u, 4096u, 10000u, 16384u, 24576u,
        32768u, 40960u, 50000u, 60000u, 65534u
    };
    ApsisStringMotionPlayback got;
    uint8_t continuous;
    uint32_t i;

    s->clock.n = 1u;
    s->string.motion_loop_length = 1u;
    s->string.motion_loop_play = 1u;
    rt->recording = 0u;
    rt->manual_override = 0u;
    rt->play_pulse_index = 0u;
    rt->play_pulse_sample = 1000u;
    rt->pulse_interval_samples = 65535u;

    for (continuous = 0u; continuous <= 1u; ++continuous) {
        s->string.motion_loop_pitch_continuous = continuous;
        rt->left_cache_hi = APSIS_STRING_MOTION_CACHE_INVALID;
        rt->right_cache_hi = APSIS_STRING_MOTION_CACHE_INVALID;
        for (i = 0u; i < sizeof(phase) / sizeof(phase[0]); ++i) {
            RefSample left, right;
            int32_t lf = apsis_string_motion_pitch_for_split(
                &rt->pitch, APSIS_SIDE_LEFT, s->string.left_split_q15);
            int32_t rf = apsis_string_motion_pitch_for_split(
                &rt->pitch, APSIS_SIDE_RIGHT, s->string.right_split_q15);
            left = reference_lane(&store->left, APSIS_SIDE_LEFT, phase[i], continuous,
                                  s->string.left_split_q15, lf);
            right = reference_lane(&store->right, APSIS_SIDE_RIGHT, phase[i], continuous,
                                   s->string.right_split_q15, rf);
            assert(apsis_string_motion_playback(store, rt, s,
                                                1000u + phase[i], &got));
            assert(got.phase_q16 == phase[i]);
            assert(got.left_split_q15 == left.split_q15);
            assert(got.right_split_q15 == right.split_q15);
            assert(got.left_pitch_milli == left.pitch_milli);
            assert(got.right_pitch_milli == right.pitch_milli);

            /* Repeat the same phase to exercise the cached endpoint path. */
            assert(apsis_string_motion_playback(store, rt, s,
                                                1000u + phase[i], &got));
            assert(got.left_pitch_milli == left.pitch_milli);
            assert(got.right_pitch_milli == right.pitch_milli);
        }
    }
}

static void test_compact_free_dual_equivalence_and_cache_invalidation(void)
{
    static const uint16_t left_split[4] = {
        APSIS_Q15_MILLI(460), APSIS_Q15_MILLI(390),
        APSIS_Q15_MILLI(310), APSIS_Q15_MILLI(430)
    };
    static const uint16_t right_split_a[4] = {
        APSIS_Q15_MILLI(540), APSIS_Q15_MILLI(610),
        APSIS_Q15_MILLI(790), APSIS_Q15_MILLI(680)
    };
    static const uint16_t right_split_b[4] = {
        APSIS_Q15_MILLI(570), APSIS_Q15_MILLI(740),
        APSIS_Q15_MILLI(830), APSIS_Q15_MILLI(650)
    };
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    ApsisStringMotionLane left_before;
    uint64_t t = 10000u;

    defaults(&s);
    apsis_string_motion_reset(&store, &rt);

    s.string.left_follow_gravity = 1u;
    s.string.left_anchor = APSIS_ANCHOR_ROOT;
    s.string.span_q15 = APSIS_Q15_MILLI(900);
    record_cycle(&store, &rt, &s, APSIS_STRING_MOTION_TARGET_LEFT, &t, left_split);
    left_before = store.left;

    s.string.right_follow_gravity = 1u;
    s.string.right_anchor = APSIS_ANCHOR_NINTH;
    s.string.span_q15 = APSIS_Q15_MILLI(1800);
    s.gravity.tonic = 11u;
    s.gravity.scale = APSIS_SCALE_LYDIAN;
    s.gravity.degree = 7u;
    s.temper.mode = APSIS_TUNING_PYTH;
    s.temper.strength_q15 = APSIS_Q15_ONE;
    t += 100000u;
    record_cycle(&store, &rt, &s, APSIS_STRING_MOTION_TARGET_RIGHT, &t, right_split_a);
    assert(memcmp(&left_before, &store.left, sizeof(left_before)) == 0);

    /* Later live Gravity/Tuning edits must not alter either recorded lane. */
    s.gravity.tonic = 3u;
    s.gravity.scale = APSIS_SCALE_PHRYGIAN;
    s.gravity.degree = 1u;
    s.temper.mode = APSIS_TUNING_JUST;
    s.string.span_q15 = APSIS_Q15_MILLI(500);
    assert_playback_matches_reference(&store, &rt, &s);
    assert(rt.left_cache_hi != APSIS_STRING_MOTION_CACHE_INVALID);
    assert(rt.right_cache_hi != APSIS_STRING_MOTION_CACHE_INVALID);

    /* A new right-only overdub must invalidate only right playback cache and
     * preserve the complete left lane/context bit-for-bit. */
    s.string.span_q15 = APSIS_Q15_MILLI(1300);
    s.gravity.tonic = 5u;
    s.gravity.scale = APSIS_SCALE_HARMONIC_MINOR;
    s.gravity.degree = 4u;
    t += 100000u;
    assert(apsis_string_motion_arm(&rt, &s, APSIS_STRING_MOTION_TARGET_RIGHT));
    s.string.right_split_q15 = right_split_b[0];
    apsis_string_motion_pulse(&store, &rt, &s, 0u, t, 22050u);
    assert(rt.recording);
    assert(rt.right_cache_hi == APSIS_STRING_MOTION_CACHE_INVALID);
    assert(memcmp(&left_before, &store.left, sizeof(left_before)) == 0);
    for (uint8_t step = 1u; step < 4u; ++step) {
        t += 22050u;
        s.string.right_split_q15 = right_split_b[step];
        apsis_string_motion_pulse(&store, &rt, &s, step, t, 22050u);
    }
    t += 22050u;
    apsis_string_motion_pulse(&store, &rt, &s, 0u, t, 22050u);
    assert(!rt.recording);
    assert(memcmp(&left_before, &store.left, sizeof(left_before)) == 0);
    assert_playback_matches_reference(&store, &rt, &s);
}

int main(void)
{
    assert(sizeof(ApsisStringMotionPitchContext) == 20u);
    assert(sizeof(ApsisStringMotionPoint) == 4u);
    assert(sizeof(ApsisStringMotionLane) == 4822u);
    assert(sizeof(ApsisStringMotionStore) == 9644u);
    assert(sizeof(ApsisStringMotionRuntime) == 88u);
    test_compact_free_dual_equivalence_and_cache_invalidation();
    puts("PASS: P11C compact Motion store is playback-equivalent with lane-safe cache invalidation");
    return 0;
}
