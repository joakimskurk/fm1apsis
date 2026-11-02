#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apsis_state.h"
#include "apsis_string.h"
#include "apsis_string_motion.h"

typedef struct {
    uint16_t phase_q16;
    uint16_t split_q15;
} P11MotionPointCandidate;

typedef struct {
    P11MotionPointCandidate point[APSIS_STRING_MOTION_POINTS_MAX];
    ApsisStringMotionPitchContext pitch;
    uint16_t count;
} P11MotionLaneCandidate;

typedef struct {
    P11MotionLaneCandidate left;
    P11MotionLaneCandidate right;
} P11MotionStoreCandidate;

static void defaults(ApsisState *s)
{
    apsis_state_default(s);
    s->clock.n = 4u;
    s->string.motion_loop_length = 1u;
    s->string.surface_mode = APSIS_STRING_DUAL;
    s->string.linked = 0u;
}

static void assert_lane_pitch_derivable(const ApsisStringMotionLane *lane,
                                        const ApsisStringMotionPitchContext *ctx,
                                        uint8_t side)
{
    uint16_t i;
    assert(lane);
    assert(ctx);
    assert(lane->count > 1u);
    for (i = 0u; i < lane->count; ++i) {
        int32_t recomputed = apsis_string_motion_pitch_for_split(
            ctx, side, lane->point[i].split_q15);
        assert(lane->point[i].pitch_milli == recomputed);
    }
}

static void record_one_cycle(ApsisStringMotionStore *store,
                             ApsisStringMotionRuntime *rt,
                             ApsisState *s,
                             uint8_t target,
                             uint64_t *time,
                             uint16_t a,
                             uint16_t b,
                             uint16_t c,
                             uint16_t d,
                             ApsisStringMotionPitchContext *frozen)
{
    const uint32_t interval = 22050u;
    uint16_t split[4] = {a, b, c, d};
    uint8_t step;

    assert(apsis_string_motion_arm(rt, s, target));
    if (target == APSIS_STRING_MOTION_TARGET_LEFT)
        s->string.left_split_q15 = split[0];
    else
        s->string.right_split_q15 = split[0];
    apsis_string_motion_pulse(store, rt, s, 0u, *time, interval);
    assert(rt->recording);
    *frozen = rt->pitch;

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

static void test_lane_context_is_sufficient_across_free_dual_overdub(void)
{
    ApsisState s;
    ApsisStringMotionRuntime rt;
    ApsisStringMotionStore store;
    ApsisStringMotionLane left_before;
    ApsisStringMotionPitchContext left_ctx, right_ctx;
    uint64_t t = 10000u;
    uint16_t i;
    uint8_t contexts_are_observably_different = 0u;

    defaults(&s);
    apsis_string_motion_reset(&store, &rt);

    s.string.left_follow_gravity = 1u;
    s.string.left_anchor = APSIS_ANCHOR_ROOT;
    s.string.span_q15 = APSIS_Q15_MILLI(900);
    s.gravity.tonic = 0u;
    s.gravity.degree = 0u;
    s.temper.mode = APSIS_TUNING_EQUAL;
    s.temper.strength_q15 = 0u;

    record_one_cycle(&store, &rt, &s, APSIS_STRING_MOTION_TARGET_LEFT, &t,
                     APSIS_Q15_MILLI(460), APSIS_Q15_MILLI(390),
                     APSIS_Q15_MILLI(310), APSIS_Q15_MILLI(430), &left_ctx);
    assert_lane_pitch_derivable(&store.left, &left_ctx, APSIS_SIDE_LEFT);
    left_before = store.left;

    /* A later free-Dual overdub deliberately freezes a materially different
     * musical context for the right lane. The untouched left lane must retain
     * both its shape and the pitches derivable from its original context. */
    s.string.right_follow_gravity = 1u;
    s.string.right_anchor = APSIS_ANCHOR_NINTH;
    s.string.span_q15 = APSIS_Q15_MILLI(1800);
    s.gravity.tonic = 11u;
    s.gravity.scale = APSIS_SCALE_LYDIAN;
    s.gravity.degree = 7u;
    s.temper.mode = APSIS_TUNING_PYTH;
    s.temper.strength_q15 = APSIS_Q15_ONE;
    t += 100000u;

    record_one_cycle(&store, &rt, &s, APSIS_STRING_MOTION_TARGET_RIGHT, &t,
                     APSIS_Q15_MILLI(540), APSIS_Q15_MILLI(610),
                     APSIS_Q15_MILLI(790), APSIS_Q15_MILLI(680), &right_ctx);

    assert(memcmp(&store.left, &left_before, sizeof(left_before)) == 0);
    assert_lane_pitch_derivable(&store.left, &left_ctx, APSIS_SIDE_LEFT);
    assert_lane_pitch_derivable(&store.right, &right_ctx, APSIS_SIDE_RIGHT);

    for (i = 0u; i < store.left.count; ++i) {
        if (apsis_string_motion_pitch_for_split(&right_ctx, APSIS_SIDE_LEFT,
                                                store.left.point[i].split_q15) !=
            store.left.point[i].pitch_milli) {
            contexts_are_observably_different = 1u;
            break;
        }
    }
    assert(contexts_are_observably_different);
}

static void test_int16_pitch_is_not_a_lossless_alternative(void)
{
    ApsisStringMotionPitchContext ctx;
    int32_t pitch;
    memset(&ctx, 0, sizeof(ctx));
    ctx.span_q15 = APSIS_Q15_MILLI(1800);
    ctx.surface_mode = APSIS_STRING_DUAL;
    ctx.right_anchor = APSIS_ANCHOR_NINTH;
    ctx.right_follow_gravity = 0u;
    ctx.right_gravity_degree = 7;
    ctx.tonic = 11u;
    ctx.scale = APSIS_SCALE_LYDIAN;
    ctx.quantize = 0u;
    ctx.scale_lock = 0u;
    ctx.temper_mode = APSIS_TUNING_EQUAL;

    pitch = apsis_string_motion_pitch_for_split(&ctx, APSIS_SIDE_RIGHT,
                                                APSIS_STRING_SPLIT_MAX_Q15);
    assert(pitch > INT16_MAX);
}

static void test_candidate_layout_budget(void)
{
    const size_t current = sizeof(ApsisStringMotionStore);
    const size_t candidate = sizeof(P11MotionStoreCandidate);
    const size_t saving = current - candidate;

    assert(sizeof(ApsisStringMotionPitchContext) == 20u);
    assert(sizeof(P11MotionPointCandidate) == 4u);
    assert(sizeof(P11MotionLaneCandidate) == 4822u);
    assert(current == 19208u);
    assert(candidate == 9644u);
    assert(saving == 9564u);

    printf("P11B current_store=%zu candidate_store=%zu projected_saving=%zu "
           "projected_pool=%u projected_pool_headroom=%u\n",
           current, candidate, saving,
           (unsigned)(328740u - saving),
           (unsigned)(344064u - (328740u - saving)));
}

int main(void)
{
    test_lane_context_is_sufficient_across_free_dual_overdub();
    test_int16_pitch_is_not_a_lossless_alternative();
    test_candidate_layout_budget();
    puts("PASS: P11B per-lane frozen pitch context can losslessly replace per-point pitch storage");
    return 0;
}
