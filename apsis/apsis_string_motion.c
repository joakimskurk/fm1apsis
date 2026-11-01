#include "apsis_string_motion.h"
#include "apsis_string.h"
#include "apsis_harmony.h"

static void p10_motion_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t p10_motion_clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int32_t p10_motion_abs_i32(int32_t v)
{
    return v < 0 ? -v : v;
}

static int32_t p10_motion_round_step(int32_t raw_milli)
{
    if (raw_milli >= 0) return (raw_milli + 500) / 1000;
    return -((-raw_milli + 499) / 1000);
}

static uint8_t p10_motion_side(uint8_t side)
{
    return side == APSIS_SIDE_LEFT ? APSIS_SIDE_LEFT : APSIS_SIDE_RIGHT;
}

static uint8_t p10_motion_degree(const ApsisStringMotionPitchContext *ctx,
                                 uint8_t side)
{
    int32_t d;
    if (!ctx) return 0u;
    side = p10_motion_side(side);
    if (ctx->surface_mode == APSIS_STRING_CLASSIC) d = ctx->degree;
    else if ((side == APSIS_SIDE_LEFT && ctx->left_follow_gravity) ||
             (side == APSIS_SIDE_RIGHT && ctx->right_follow_gravity)) d = ctx->degree;
    else d = side == APSIS_SIDE_LEFT ? ctx->left_gravity_degree
                                     : ctx->right_gravity_degree;
    if (d < 0) d = 0;
    if (d > 7) d = 7;
    return (uint8_t)d;
}

static int32_t p10_motion_anchor_step(const ApsisStringMotionPitchContext *ctx,
                                      uint8_t side)
{
    static const int8_t anchor_index[5] = {0, 2, 4, 6, 8};
    uint8_t anchor, degree;
    if (!ctx) return 0;
    side = p10_motion_side(side);
    degree = p10_motion_degree(ctx, side);
    if (ctx->surface_mode == APSIS_STRING_CLASSIC && side == APSIS_SIDE_RIGHT)
        anchor = APSIS_ANCHOR_ROOT;
    else
        anchor = side == APSIS_SIDE_LEFT ? ctx->left_anchor : ctx->right_anchor;
    if (anchor > APSIS_ANCHOR_NINTH) anchor = APSIS_ANCHOR_ROOT;
    return apsis_scale_at(ctx->tonic, ctx->scale,
                          (int32_t)degree + anchor_index[anchor]);
}

static int32_t p10_motion_quantize_scale(const ApsisStringMotionPitchContext *ctx,
                                         int32_t raw_milli)
{
    const int8_t *scale;
    int32_t cand, best = 0, best_err = 0x7fffffff;
    uint8_t i;
    if (!ctx) return raw_milli;
    scale = apsis_scale_intervals(ctx->scale);
    for (cand = -48; cand <= 84; ++cand) {
        int32_t pc = cand % 12;
        uint8_t allowed = 0u;
        if (pc < 0) pc += 12;
        for (i = 0u; i < 7u; ++i) {
            int32_t apc = ((int32_t)(ctx->tonic % 12u) + scale[i]) % 12;
            if (apc < 0) apc += 12;
            if (apc == pc) { allowed = 1u; break; }
        }
        if (allowed) {
            int32_t e = p10_motion_abs_i32(cand * 1000 - raw_milli);
            if (e < best_err) { best_err = e; best = cand; }
        }
    }
    return best * 1000;
}

static int32_t p10_motion_quantize_chord(const ApsisStringMotionPitchContext *ctx,
                                         uint8_t side,
                                         int32_t raw_milli)
{
    ApsisGravityState g;
    int16_t intervals[APSIS_GRAVITY_VOICES_MAX];
    uint8_t degree, n, i;
    int32_t root, oct, best = 0, best_err = 0x7fffffff;
    if (!ctx) return raw_milli;
    p10_motion_zero(&g, (uint32_t)sizeof(g));
    degree = p10_motion_degree(ctx, side);
    g.tonic = ctx->tonic;
    g.scale = ctx->scale;
    g.degree = (uint8_t)(degree % 7u);
    g.voicing = ctx->voicing;
    g.voices = ctx->voices;
    root = apsis_scale_at(ctx->tonic, ctx->scale, degree);
    n = apsis_gravity_ring_intervals(&g, intervals);
    for (oct = -4; oct <= 5; ++oct) {
        for (i = 0u; i < n; ++i) {
            int32_t cand = root + intervals[i] + oct * 12;
            int32_t e = p10_motion_abs_i32(cand * 1000 - raw_milli);
            if (e < best_err) { best_err = e; best = cand; }
        }
    }
    return best * 1000;
}

static uint8_t p10_motion_target_has(uint8_t target, uint8_t side)
{
    if (target == APSIS_STRING_MOTION_TARGET_BOTH) return 1u;
    if (side == APSIS_SIDE_LEFT) return target == APSIS_STRING_MOTION_TARGET_LEFT;
    return target == APSIS_STRING_MOTION_TARGET_RIGHT;
}

static uint16_t p10_motion_phase_q16(uint16_t pulse_index,
                                     uint64_t pulse_sample,
                                     uint32_t interval_samples,
                                     uint16_t total_pulses,
                                     uint64_t now_sample,
                                     uint8_t recording)
{
    uint64_t elapsed, frac_q16, numerator, phase;
    if (!total_pulses) total_pulses = 1u;
    if (!interval_samples) interval_samples = 1u;
    elapsed = now_sample > pulse_sample ? now_sample - pulse_sample : 0u;
    if (elapsed > interval_samples) elapsed = interval_samples;
    frac_q16 = (elapsed * 65535u + interval_samples / 2u) / interval_samples;
    numerator = (uint64_t)pulse_index * 65535u + frac_q16;
    phase = (numerator + total_pulses / 2u) / total_pulses;
    if (recording && phase >= 65535u) return 65535u;
    if (!recording && phase >= 65535u) phase %= 65535u;
    return (uint16_t)phase;
}

static uint16_t p10_motion_total_pulses(const ApsisState *state)
{
    uint16_t n = state && state->clock.n ? state->clock.n : 12u;
    uint16_t cycles = state ? apsis_string_motion_length(state->string.motion_loop_length) : 1u;
    return (uint16_t)(n * cycles);
}

static const ApsisStringMotionLane *p10_motion_lane_const(const ApsisStringMotionStore *store,
                                                          uint8_t side)
{
    if (!store) return 0;
    return side == APSIS_SIDE_LEFT ? &store->left : &store->right;
}

static uint8_t p10_motion_lane_append(ApsisStringMotionLane *lane,
                                      uint16_t phase_q16,
                                      uint16_t split_q15,
                                      int32_t pitch_milli)
{
    ApsisStringMotionPoint *p;
    if (!lane) return 0u;
    if (lane->count) {
        p = &lane->point[lane->count - 1u];
        if (p->phase_q16 == phase_q16 && p->split_q15 == split_q15 &&
            p->pitch_milli == pitch_milli)
            return 0u;
    }
    if (lane->count >= APSIS_STRING_MOTION_POINTS_MAX) {
        /* Source simplifies to <=1200 after capture. Native bounds storage at
         * that same final ceiling; when saturated, preserve the start/shape and
         * keep the newest endpoint rather than allocating an unbounded buffer. */
        p = &lane->point[APSIS_STRING_MOTION_POINTS_MAX - 1u];
        p->phase_q16 = phase_q16;
        p->split_q15 = split_q15;
        p->pitch_milli = pitch_milli;
        return 1u;
    }
    p = &lane->point[lane->count++];
    p->phase_q16 = phase_q16;
    p->split_q15 = split_q15;
    p->pitch_milli = pitch_milli;
    return 1u;
}

static uint16_t p10_motion_split_sample(const ApsisStringMotionLane *lane,
                                        uint16_t phase_q16,
                                        uint16_t fallback)
{
    uint16_t hi, i;
    const ApsisStringMotionPoint *a, *b;
    uint32_t span, pos;
    int32_t delta;
    if (!lane || !lane->count) return fallback;
    if (lane->count == 1u) return lane->point[0].split_q15;
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
    if (!span) return b->split_q15;
    delta = (int32_t)b->split_q15 - a->split_q15;
    return (uint16_t)((int32_t)a->split_q15 +
        (int32_t)(((int64_t)delta * pos + (int32_t)span / 2) / (int32_t)span));
}

static int32_t p10_motion_pitch_sample(const ApsisStringMotionLane *lane,
                                       uint16_t phase_q16,
                                       uint8_t continuous,
                                       int32_t fallback)
{
    uint16_t hi, i;
    const ApsisStringMotionPoint *a, *b;
    uint32_t span, pos;
    int64_t delta;
    if (!lane || !lane->count) return fallback;
    if (lane->count == 1u) return lane->point[0].pitch_milli;
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
    if (!continuous) return a->pitch_milli;
    if (!span) return b->pitch_milli;
    delta = (int64_t)b->pitch_milli - a->pitch_milli;
    return a->pitch_milli + (int32_t)((delta * pos + (int32_t)span / 2) / (int32_t)span);
}

void apsis_string_motion_reset(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime)
{
    if (store) p10_motion_zero(store, (uint32_t)sizeof(*store));
    if (runtime) {
        p10_motion_zero(runtime, (uint32_t)sizeof(*runtime));
        runtime->record_target = APSIS_STRING_MOTION_TARGET_BOTH;
    }
}

uint8_t apsis_string_motion_length(uint8_t cycles)
{
    return cycles == 2u || cycles == 4u || cycles == 8u ? cycles : 1u;
}

uint8_t apsis_string_motion_resolve_target(const ApsisState *state,
                                           uint8_t requested_target)
{
    if (!state || state->string.surface_mode == APSIS_STRING_CLASSIC)
        return APSIS_STRING_MOTION_TARGET_RIGHT;
    if (state->string.linked) return APSIS_STRING_MOTION_TARGET_BOTH;
    return requested_target <= APSIS_STRING_MOTION_TARGET_BOTH
        ? requested_target : APSIS_STRING_MOTION_TARGET_BOTH;
}

uint8_t apsis_string_motion_arm(ApsisStringMotionRuntime *runtime,
                                ApsisState *state,
                                uint8_t requested_target)
{
    uint8_t target;
    if (!runtime || !state || runtime->recording) return 0u;
    target = apsis_string_motion_resolve_target(state, requested_target);
    if (runtime->armed && runtime->record_target == target) {
        runtime->armed = 0u;
        return 0u;
    }
    runtime->record_target = target;
    runtime->armed = 1u;
    state->string.motion_loop_play = 0u;
    return 1u;
}

void apsis_string_motion_clear(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime,
                               ApsisState *state)
{
    apsis_string_motion_reset(store, runtime);
    if (state) state->string.motion_loop_play = 0u;
}

uint8_t apsis_string_motion_toggle_play(const ApsisStringMotionStore *store,
                                        ApsisStringMotionRuntime *runtime,
                                        ApsisState *state)
{
    uint8_t has;
    if (!store || !runtime || !state) return 0u;
    has = (uint8_t)(store->left.count > 1u || store->right.count > 1u);
    if (!has) return 0u;
    state->string.motion_loop_play = state->string.motion_loop_play ? 0u : 1u;
    runtime->manual_override = 0u;
    if (state->string.motion_loop_play) {
        runtime->play_cycle = 0u;
        runtime->play_pulse_index = 0u;
    }
    return state->string.motion_loop_play;
}

void apsis_string_motion_freeze_pitch(const ApsisState *state,
                                      ApsisStringMotionPitchContext *out)
{
    if (!out) return;
    p10_motion_zero(out, (uint32_t)sizeof(*out));
    if (!state) return;
    out->span_q15 = state->string.span_q15;
    out->temper_strength_q15 = state->temper.strength_q15;
    out->surface_mode = state->string.surface_mode;
    out->left_anchor = state->string.left_anchor;
    out->right_anchor = state->string.right_anchor;
    out->left_follow_gravity = state->string.left_follow_gravity;
    out->right_follow_gravity = state->string.right_follow_gravity;
    out->left_gravity_degree = state->string.left_gravity_degree;
    out->right_gravity_degree = state->string.right_gravity_degree;
    out->quantize = state->string.quantize;
    out->harmony = state->string.harmony;
    out->tonic = state->gravity.tonic;
    out->scale = state->gravity.scale;
    out->degree = state->gravity.degree;
    out->voicing = state->gravity.voicing;
    out->voices = state->gravity.voices;
    out->scale_lock = state->gravity.scale_lock;
    out->temper_mode = state->temper.mode;
}

int32_t apsis_string_motion_pitch_for_split(const ApsisStringMotionPitchContext *ctx,
                                            uint8_t side,
                                            uint16_t split_q15)
{
    int32_t raw, pitch, whole, dev, pc_i;
    uint8_t pc;
    if (!ctx) return 0;
    side = p10_motion_side(side);
    raw = p10_motion_anchor_step(ctx, side) * 1000 +
          apsis_string_magnitude_milli(split_q15, ctx->span_q15);
    pitch = raw;
    if (ctx->quantize) {
        if (ctx->harmony == APSIS_HARMONY_CHROMATIC)
            pitch = p10_motion_round_step(raw) * 1000;
        else if (ctx->harmony == APSIS_HARMONY_CHORD)
            pitch = p10_motion_quantize_chord(ctx, side, raw);
        else
            pitch = p10_motion_quantize_scale(ctx, raw);
    }
    if (ctx->scale_lock) pitch = p10_motion_quantize_scale(ctx, pitch);

    whole = p10_motion_round_step(pitch);
    if (pitch == whole * 1000) {
        pc_i = whole % 12;
        if (pc_i < 0) pc_i += 12;
        pc = (uint8_t)pc_i;
        dev = apsis_tuning_deviation_milli(pc, ctx->tonic,
                                           ctx->temper_mode,
                                           ctx->temper_strength_q15);
        pitch += dev >= 0 ? (dev + 50) / 100 : -((-dev + 50) / 100);
    }
    return pitch;
}

static void p10_motion_begin(ApsisStringMotionStore *store,
                             ApsisStringMotionRuntime *runtime,
                             ApsisState *state,
                             uint64_t when_sample,
                             uint32_t interval_samples)
{
    uint8_t target;
    if (!store || !runtime || !state) return;
    target = apsis_string_motion_resolve_target(state, runtime->record_target);
    runtime->armed = 0u;
    runtime->recording = 1u;
    runtime->record_target = target;
    runtime->record_cycles = 0u;
    runtime->record_pulse_index = 0u;
    runtime->record_pulse_sample = when_sample;
    runtime->pulse_interval_samples = interval_samples ? interval_samples : 1u;
    runtime->last_capture_sample = 0u;
    runtime->owned_left_q15 = state->string.left_split_q15;
    runtime->owned_right_q15 = state->string.right_split_q15;
    apsis_string_motion_freeze_pitch(state, &runtime->pitch);
    state->string.motion_loop_pitch_continuous = state->string.quantize ? 0u : 1u;
    state->string.motion_loop_play = 0u;
    runtime->manual_override = 0u;
    if (p10_motion_target_has(target, APSIS_SIDE_LEFT)) store->left.count = 0u;
    if (p10_motion_target_has(target, APSIS_SIDE_RIGHT)) store->right.count = 0u;
    (void)apsis_string_motion_capture(store, runtime, state, when_sample, 1u);
}

static void p10_motion_finish(ApsisStringMotionStore *store,
                              ApsisStringMotionRuntime *runtime,
                              ApsisState *state,
                              uint64_t when_sample,
                              uint32_t interval_samples)
{
    uint8_t has;
    if (!store || !runtime || !state) return;
    (void)apsis_string_motion_capture(store, runtime, state, when_sample, 1u);
    runtime->recording = 0u;
    runtime->record_cycles = 0u;
    runtime->just_finished = 1u;
    runtime->play_cycle = 0u;
    runtime->play_pulse_index = 0u;
    runtime->play_pulse_sample = when_sample;
    runtime->pulse_interval_samples = interval_samples ? interval_samples : 1u;
    runtime->manual_override = 0u;
    has = (uint8_t)(store->left.count > 1u || store->right.count > 1u);
    state->string.motion_loop_play = has;
}

void apsis_string_motion_pulse(ApsisStringMotionStore *store,
                               ApsisStringMotionRuntime *runtime,
                               ApsisState *state,
                               uint8_t step,
                               uint64_t when_sample,
                               uint32_t interval_samples)
{
    uint8_t n, cycles;
    uint16_t total;
    if (!store || !runtime || !state) return;
    n = state->clock.n ? state->clock.n : 12u;
    cycles = apsis_string_motion_length(state->string.motion_loop_length);
    total = (uint16_t)n * cycles;
    if (step >= n) step = (uint8_t)(step % n);

    if (step == 0u) {
        if (runtime->armed && !runtime->recording)
            p10_motion_begin(store, runtime, state, when_sample, interval_samples);
        else if (runtime->recording) {
            runtime->record_cycles++;
            if (runtime->record_cycles >= cycles)
                p10_motion_finish(store, runtime, state, when_sample, interval_samples);
        }
    }

    if (runtime->recording) {
        uint16_t ix = (uint16_t)runtime->record_cycles * n + step;
        runtime->record_pulse_index = ix < total ? ix : (uint16_t)(total - 1u);
        runtime->record_pulse_sample = when_sample;
        runtime->pulse_interval_samples = interval_samples ? interval_samples : 1u;
        (void)apsis_string_motion_capture(store, runtime, state, when_sample, 1u);
    }

    if (state->string.motion_loop_play && !runtime->recording) {
        if (step == 0u) {
            if (runtime->just_finished) {
                runtime->play_cycle = 0u;
                runtime->just_finished = 0u;
            } else {
                runtime->play_cycle = (uint8_t)((runtime->play_cycle + 1u) % cycles);
            }
        }
        runtime->play_pulse_index = (uint16_t)(((uint16_t)runtime->play_cycle * n + step) % total);
        runtime->play_pulse_sample = when_sample;
        runtime->pulse_interval_samples = interval_samples ? interval_samples : 1u;
    }
}

uint8_t apsis_string_motion_capture(ApsisStringMotionStore *store,
                                    ApsisStringMotionRuntime *runtime,
                                    const ApsisState *state,
                                    uint64_t now_sample,
                                    uint8_t force)
{
    uint16_t total, phase;
    uint8_t changed = 0u;
    if (!store || !runtime || !state || !runtime->recording) return 0u;
    if (!force && runtime->last_capture_sample &&
        now_sample - runtime->last_capture_sample < APSIS_STRING_MOTION_RATE / APSIS_STRING_MOTION_CAPTURE_HZ)
        return 0u;
    runtime->last_capture_sample = now_sample;
    total = p10_motion_total_pulses(state);
    phase = p10_motion_phase_q16(runtime->record_pulse_index,
                                 runtime->record_pulse_sample,
                                 runtime->pulse_interval_samples,
                                 total, now_sample, 1u);
    runtime->owned_left_q15 = state->string.left_split_q15;
    runtime->owned_right_q15 = state->string.right_split_q15;
    if (p10_motion_target_has(runtime->record_target, APSIS_SIDE_LEFT)) {
        changed |= p10_motion_lane_append(
            &store->left, phase, state->string.left_split_q15,
            apsis_string_motion_pitch_for_split(&runtime->pitch, APSIS_SIDE_LEFT,
                                                state->string.left_split_q15));
    }
    if (p10_motion_target_has(runtime->record_target, APSIS_SIDE_RIGHT)) {
        changed |= p10_motion_lane_append(
            &store->right, phase, state->string.right_split_q15,
            apsis_string_motion_pitch_for_split(&runtime->pitch, APSIS_SIDE_RIGHT,
                                                state->string.right_split_q15));
    }
    return changed;
}

void apsis_string_motion_restore_owned(const ApsisStringMotionRuntime *runtime,
                                       ApsisState *state)
{
    if (!runtime || !state || !runtime->recording) return;
    state->string.left_split_q15 = p10_motion_clamp_u16(
        runtime->owned_left_q15, APSIS_STRING_SPLIT_MIN_Q15, APSIS_STRING_SPLIT_MID_Q15);
    state->string.right_split_q15 = p10_motion_clamp_u16(
        runtime->owned_right_q15, APSIS_STRING_SPLIT_MID_Q15, APSIS_STRING_SPLIT_MAX_Q15);
    state->string.split_q15 = state->string.right_split_q15;
}

void apsis_string_motion_manual_override(ApsisStringMotionRuntime *runtime,
                                         uint8_t active)
{
    if (!runtime || runtime->recording) return;
    runtime->manual_override = active ? 1u : 0u;
}

uint8_t apsis_string_motion_playback(const ApsisStringMotionStore *store,
                                     const ApsisStringMotionRuntime *runtime,
                                     const ApsisState *state,
                                     uint64_t now_sample,
                                     ApsisStringMotionPlayback *out)
{
    uint16_t total, phase;
    int32_t left_fallback, right_fallback;
    if (!store || !runtime || !state || !out) return 0u;
    if (!state->string.motion_loop_play || runtime->recording || runtime->manual_override)
        return 0u;
    if (!store->left.count && !store->right.count) return 0u;
    total = p10_motion_total_pulses(state);
    phase = p10_motion_phase_q16(runtime->play_pulse_index,
                                 runtime->play_pulse_sample,
                                 runtime->pulse_interval_samples,
                                 total, now_sample, 0u);
    left_fallback = apsis_string_motion_pitch_for_split(&runtime->pitch, APSIS_SIDE_LEFT,
                                                        state->string.left_split_q15);
    right_fallback = apsis_string_motion_pitch_for_split(&runtime->pitch, APSIS_SIDE_RIGHT,
                                                         state->string.right_split_q15);
    out->phase_q16 = phase;
    out->left_split_q15 = p10_motion_split_sample(
        p10_motion_lane_const(store, APSIS_SIDE_LEFT), phase, state->string.left_split_q15);
    out->right_split_q15 = p10_motion_split_sample(
        p10_motion_lane_const(store, APSIS_SIDE_RIGHT), phase, state->string.right_split_q15);
    out->left_pitch_milli = p10_motion_pitch_sample(
        p10_motion_lane_const(store, APSIS_SIDE_LEFT), phase,
        state->string.motion_loop_pitch_continuous, left_fallback);
    out->right_pitch_milli = p10_motion_pitch_sample(
        p10_motion_lane_const(store, APSIS_SIDE_RIGHT), phase,
        state->string.motion_loop_pitch_continuous, right_fallback);
    return 1u;
}
