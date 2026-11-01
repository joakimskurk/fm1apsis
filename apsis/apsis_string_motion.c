#include "apsis_string_motion.h"
#include "apsis_string.h"
#include "apsis_string_dsp.h"

#define MOTION_PHASE_ONE 32767u
#define MOTION_SPLIT_EPS_Q15 59u   /* round(.0018 * 32767) */
#define MOTION_PHASE_GAP_Q15 590u  /* round(.018 * 32767) */
#define MOTION_PITCH_EPS_CENTIHZ 8u /* Study-126: > .08 Hz */

static void motion_zero(void *ptr, uint32_t n)
{
    uint8_t *p = (uint8_t *)ptr;
    while (n--) *p++ = 0u;
}

static uint16_t motion_clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint32_t motion_abs_i32(int32_t v)
{
    return (uint32_t)(v < 0 ? -v : v);
}

static uint8_t motion_valid_length(uint8_t cycles)
{
    return cycles == 1u || cycles == 2u || cycles == 4u || cycles == 8u;
}

static uint8_t motion_length(const ApsisState *state)
{
    uint8_t v = state ? state->string.motion_loop_length : 1u;
    return motion_valid_length(v) ? v : 1u;
}

static void motion_freq_set(ApsisStringMotionTrack *track, uint16_t index,
                            uint32_t millihz)
{
    uint32_t centihz;
    uint32_t off;
    if (!track || index >= APSIS_STRING_MOTION_RAW_MAX) return;
    centihz = (millihz + 5u) / 10u;
    if (centihz > 0xFFFFFFu) centihz = 0xFFFFFFu;
    off = (uint32_t)index * 3u;
    track->frequency_centi_hz[off] = (uint8_t)centihz;
    track->frequency_centi_hz[off + 1u] = (uint8_t)(centihz >> 8);
    track->frequency_centi_hz[off + 2u] = (uint8_t)(centihz >> 16);
}

static uint32_t motion_freq_get_chz(const ApsisStringMotionTrack *track,
                                    uint16_t index)
{
    uint32_t off;
    if (!track || index >= track->count) return 0u;
    off = (uint32_t)index * 3u;
    return (uint32_t)track->frequency_centi_hz[off] |
           ((uint32_t)track->frequency_centi_hz[off + 1u] << 8) |
           ((uint32_t)track->frequency_centi_hz[off + 2u] << 16);
}

static uint32_t motion_freq_get_mhz(const ApsisStringMotionTrack *track,
                                    uint16_t index)
{
    return motion_freq_get_chz(track, index) * 10u;
}

static void motion_track_copy_point(ApsisStringMotionTrack *track,
                                    uint16_t dst, uint16_t src)
{
    if (!track || dst >= APSIS_STRING_MOTION_RAW_MAX ||
        src >= APSIS_STRING_MOTION_RAW_MAX) return;
    track->phase_q15[dst] = track->phase_q15[src];
    track->split_q15[dst] = track->split_q15[src];
    motion_freq_set(track, dst, motion_freq_get_mhz(track, src));
}

static void motion_track_append(ApsisStringMotionTrack *track,
                                uint16_t phase_q15, uint16_t split_q15,
                                uint32_t freq_millihz)
{
    uint16_t i, ix;
    if (!track) return;
    if (track->count >= APSIS_STRING_MOTION_RAW_MAX) {
        /* Browser preserves the first point and drops point 1 when raw capture
         * exceeds its ceiling. Mirror that bounded-history behavior. */
        for (i = 1u; i + 1u < track->count; ++i)
            motion_track_copy_point(track, i, (uint16_t)(i + 1u));
        track->count--;
    }
    ix = track->count++;
    track->phase_q15[ix] = phase_q15;
    track->split_q15[ix] = split_q15;
    motion_freq_set(track, ix, freq_millihz);
}

static void motion_track_simplify(ApsisStringMotionTrack *track)
{
    uint16_t read, write;
    if (!track || track->count < 4u) {
        if (track && track->count > APSIS_STRING_MOTION_LOOP_MAX)
            track->count = APSIS_STRING_MOTION_LOOP_MAX;
        return;
    }
    write = 1u;
    for (read = 1u; read + 1u < track->count; ++read) {
        uint16_t ai = (uint16_t)(write - 1u), bi = read, ci = (uint16_t)(read + 1u);
        uint16_t ap = track->phase_q15[ai], bp = track->phase_q15[bi], cp = track->phase_q15[ci];
        uint32_t span = cp > ap ? (uint32_t)(cp - ap) : 0u;
        uint32_t keep = 0u;
        if (!span) {
            keep = 1u;
        } else {
            int32_t as = track->split_q15[ai];
            int32_t bs = track->split_q15[bi];
            int32_t cs = track->split_q15[ci];
            int32_t predicted = as +
                (int32_t)(((int64_t)(cs - as) * (bp - ap) + (int32_t)span / 2) /
                          (int32_t)span);
            uint32_t fa = motion_freq_get_chz(track, ai);
            uint32_t fb = motion_freq_get_chz(track, bi);
            if (motion_abs_i32(predicted - bs) > MOTION_SPLIT_EPS_Q15 ||
                (uint32_t)(bp - ap) > MOTION_PHASE_GAP_Q15 ||
                (fa > fb ? fa - fb : fb - fa) > MOTION_PITCH_EPS_CENTIHZ)
                keep = 1u;
        }
        if (keep) {
            if (write != read) motion_track_copy_point(track, write, read);
            ++write;
        }
    }
    motion_track_copy_point(track, write, (uint16_t)(track->count - 1u));
    ++write;
    track->count = write > APSIS_STRING_MOTION_LOOP_MAX
        ? APSIS_STRING_MOTION_LOOP_MAX : write;
}

static void motion_snapshot_context(ApsisStringMotionPitchContext *ctx,
                                    const ApsisState *state)
{
    if (!ctx) return;
    motion_zero(ctx, (uint32_t)sizeof(*ctx));
    if (!state) return;
    ctx->tonic = state->gravity.tonic;
    ctx->scale = state->gravity.scale;
    ctx->degree = state->gravity.degree;
    ctx->voicing = state->gravity.voicing;
    ctx->voices = state->gravity.voices;
    ctx->inversion = state->gravity.inversion;
    ctx->scale_lock = state->gravity.scale_lock;
    ctx->surface_mode = state->string.surface_mode;
    ctx->left_anchor = state->string.left_anchor;
    ctx->right_anchor = state->string.right_anchor;
    ctx->left_follow_gravity = state->string.left_follow_gravity;
    ctx->right_follow_gravity = state->string.right_follow_gravity;
    ctx->left_gravity_degree = state->string.left_gravity_degree;
    ctx->right_gravity_degree = state->string.right_gravity_degree;
    ctx->quantize = state->string.quantize;
    ctx->harmony = state->string.harmony;
    ctx->span_q15 = state->string.span_q15;
    ctx->temper_mode = state->temper.mode;
    ctx->temper_strength_q15 = state->temper.strength_q15;
}

static void motion_context_state(const ApsisStringMotionPitchContext *ctx,
                                 uint16_t left_split, uint16_t right_split,
                                 ApsisState *out)
{
    if (!out) return;
    motion_zero(out, (uint32_t)sizeof(*out));
    if (!ctx) return;
    out->string.left_split_q15 = left_split;
    out->string.right_split_q15 = right_split;
    out->string.split_q15 = right_split;
    out->string.surface_mode = ctx->surface_mode;
    out->string.left_anchor = ctx->left_anchor;
    out->string.right_anchor = ctx->right_anchor;
    out->string.left_follow_gravity = ctx->left_follow_gravity;
    out->string.right_follow_gravity = ctx->right_follow_gravity;
    out->string.left_gravity_degree = ctx->left_gravity_degree;
    out->string.right_gravity_degree = ctx->right_gravity_degree;
    out->string.quantize = ctx->quantize;
    out->string.harmony = ctx->harmony;
    out->string.span_q15 = ctx->span_q15;
    out->gravity.tonic = ctx->tonic;
    out->gravity.scale = ctx->scale;
    out->gravity.degree = ctx->degree;
    out->gravity.voicing = ctx->voicing;
    out->gravity.voices = ctx->voices;
    out->gravity.inversion = ctx->inversion;
    out->gravity.scale_lock = ctx->scale_lock;
    out->temper.mode = ctx->temper_mode;
    out->temper.strength_q15 = ctx->temper_strength_q15;
}

static uint32_t motion_frequency(const ApsisStringMotionRuntime *motion,
                                 uint16_t left_split, uint16_t right_split,
                                 uint8_t side)
{
    ApsisState frozen;
    int32_t step;
    if (!motion) return 0u;
    motion_context_state(&motion->pitch_context, left_split, right_split, &frozen);
    step = apsis_string_target_step_milli(&frozen, side, 0, 0u);
    return apsis_string_dsp_frequency_millihz(&frozen, step);
}

static uint16_t motion_phase_q15(uint16_t pulse_index,
                                 uint64_t pulse_sample,
                                 uint32_t pulse_interval,
                                 uint16_t total_pulses,
                                 uint64_t sample_now,
                                 uint8_t recording)
{
    uint64_t elapsed;
    uint32_t frac_q15, value;
    if (!total_pulses) total_pulses = 1u;
    if (!pulse_interval) pulse_interval = 1u;
    elapsed = sample_now > pulse_sample ? sample_now - pulse_sample : 0u;
    if (elapsed > pulse_interval) elapsed = pulse_interval;
    frac_q15 = (uint32_t)((elapsed * MOTION_PHASE_ONE + pulse_interval / 2u) /
                          pulse_interval);
    value = ((uint32_t)pulse_index * MOTION_PHASE_ONE + frac_q15) /
            total_pulses;
    if (recording && value >= MOTION_PHASE_ONE) value = MOTION_PHASE_ONE - 1u;
    return (uint16_t)(value > MOTION_PHASE_ONE ? MOTION_PHASE_ONE : value);
}

static uint16_t motion_total_pulses(const ApsisState *state)
{
    uint16_t n = state && state->clock.n ? state->clock.n : 12u;
    if (n > APSIS_CLOCK_STEPS_MAX) n = APSIS_CLOCK_STEPS_MAX;
    if (n < 1u) n = 1u;
    return (uint16_t)(n * motion_length(state));
}

static uint8_t motion_target_has(uint8_t target, uint8_t side)
{
    if (target == APSIS_STRING_MOTION_BOTH) return 1u;
    if (side == APSIS_SIDE_LEFT) return target == APSIS_STRING_MOTION_LEFT;
    return target == APSIS_STRING_MOTION_RIGHT;
}

static void motion_track_capture(ApsisStringMotionTrack *track,
                                 const ApsisStringMotionRuntime *motion,
                                 const ApsisState *state,
                                 uint8_t side, uint16_t phase)
{
    uint16_t split;
    uint32_t freq;
    if (!track || !motion || !state) return;
    split = side == APSIS_SIDE_LEFT
        ? state->string.left_split_q15 : state->string.right_split_q15;
    freq = motion_frequency(motion, state->string.left_split_q15,
                            state->string.right_split_q15, side);
    motion_track_append(track, phase, split, freq);
}

static void motion_begin_record(ApsisStringMotionRuntime *motion,
                                ApsisState *state,
                                uint64_t sample, uint32_t interval)
{
    uint8_t target;
    if (!motion || !state) return;
    target = apsis_string_motion_resolve_target(state, motion->record_target);
    motion->record_target = target;
    motion->armed = 0u;
    motion->recording = 1u;
    motion->record_cycles = 0u;
    motion->record_pulse_index = 0u;
    motion->record_pulse_sample = sample;
    motion->record_pulse_interval = interval ? interval : 1u;
    motion->last_capture_sample = sample;
    motion->owned_left_q15 = state->string.left_split_q15;
    motion->owned_right_q15 = state->string.right_split_q15;
    motion_snapshot_context(&motion->pitch_context, state);
    state->string.motion_loop_pitch_continuous = state->string.quantize ? 0u : 1u;
    state->string.motion_loop_play = 0u;
    motion->manual_override = 0u;
    if (motion_target_has(target, APSIS_SIDE_LEFT)) motion->left.count = 0u;
    if (motion_target_has(target, APSIS_SIDE_RIGHT)) motion->right.count = 0u;
    (void)apsis_string_motion_capture(motion, state, sample, 1u);
}

static void motion_finish_track(ApsisStringMotionTrack *track,
                                const ApsisStringMotionRuntime *motion,
                                const ApsisState *state, uint8_t side)
{
    uint16_t split;
    uint32_t freq;
    if (!track || !motion || !state) return;
    split = side == APSIS_SIDE_LEFT
        ? state->string.left_split_q15 : state->string.right_split_q15;
    freq = motion_frequency(motion, state->string.left_split_q15,
                            state->string.right_split_q15, side);
    motion_track_append(track, MOTION_PHASE_ONE, split, freq);
    motion_track_simplify(track);
}

static void motion_finish_record(ApsisStringMotionRuntime *motion,
                                 ApsisState *state,
                                 uint64_t sample, uint32_t interval)
{
    uint8_t target;
    if (!motion || !state) return;
    (void)apsis_string_motion_capture(motion, state, sample, 1u);
    target = motion->record_target;
    if (motion_target_has(target, APSIS_SIDE_LEFT))
        motion_finish_track(&motion->left, motion, state, APSIS_SIDE_LEFT);
    if (motion_target_has(target, APSIS_SIDE_RIGHT))
        motion_finish_track(&motion->right, motion, state, APSIS_SIDE_RIGHT);
    motion->recording = 0u;
    motion->record_cycles = 0u;
    motion->just_finished = 1u;
    motion->play_cycle = 0u;
    motion->play_pulse_index = 0u;
    motion->play_pulse_sample = sample;
    motion->play_pulse_interval = interval ? interval : 1u;
    motion->manual_override = 0u;
    state->string.motion_loop_play =
        (motion->left.count > 1u || motion->right.count > 1u) ? 1u : 0u;
}

static uint16_t motion_track_find_hi(const ApsisStringMotionTrack *track,
                                     uint16_t phase)
{
    uint16_t i;
    if (!track || !track->count) return 0u;
    for (i = 0u; i < track->count; ++i)
        if (track->phase_q15[i] >= phase) return i;
    return 0u;
}

static void motion_track_sample(const ApsisStringMotionTrack *track,
                                uint16_t phase,
                                ApsisStringMotionSideSample *out)
{
    uint16_t hi, ai, bi, ap, bp, p;
    uint32_t span, pos, frac;
    int32_t as, bs, split;
    if (!out) return;
    motion_zero(out, (uint32_t)sizeof(*out));
    if (!track || !track->count) return;
    if (track->count == 1u) {
        out->valid = 1u;
        out->split_q15 = track->split_q15[0];
        out->pitch_a_millihz = motion_freq_get_mhz(track, 0u);
        out->pitch_b_millihz = out->pitch_a_millihz;
        return;
    }
    hi = motion_track_find_hi(track, phase);
    bi = hi;
    ai = hi == 0u ? (uint16_t)(track->count - 1u) : (uint16_t)(hi - 1u);
    ap = track->phase_q15[ai];
    bp = track->phase_q15[bi];
    p = phase;
    if (hi == 0u) {
        span = (uint32_t)(MOTION_PHASE_ONE - ap) + bp;
        pos = phase < ap ? (uint32_t)(MOTION_PHASE_ONE - ap) + phase
                         : (uint32_t)(phase - ap);
    } else {
        span = bp > ap ? (uint32_t)(bp - ap) : 0u;
        pos = p > ap ? (uint32_t)(p - ap) : 0u;
    }
    frac = span ? (pos * MOTION_PHASE_ONE + span / 2u) / span : 0u;
    if (frac > MOTION_PHASE_ONE) frac = MOTION_PHASE_ONE;
    as = track->split_q15[ai];
    bs = track->split_q15[bi];
    split = as + (int32_t)(((int64_t)(bs - as) * frac + MOTION_PHASE_ONE / 2u) /
                           MOTION_PHASE_ONE);
    out->valid = 1u;
    out->split_q15 = (uint16_t)split;
    out->pitch_a_millihz = motion_freq_get_mhz(track, ai);
    out->pitch_b_millihz = motion_freq_get_mhz(track, bi);
    out->pitch_fraction_q15 = (uint16_t)frac;
}

void apsis_string_motion_reset(ApsisStringMotionRuntime *motion)
{
    if (!motion) return;
    motion_zero(motion, (uint32_t)sizeof(*motion));
    motion->record_target = APSIS_STRING_MOTION_BOTH;
    motion->preferred_target = APSIS_STRING_MOTION_BOTH;
}

uint32_t apsis_string_motion_storage_bytes(void)
{
    return (uint32_t)sizeof(ApsisStringMotionRuntime);
}

uint8_t apsis_string_motion_resolve_target(const ApsisState *state,
                                           uint8_t requested_target)
{
    if (!state) return APSIS_STRING_MOTION_BOTH;
    if (state->string.surface_mode == APSIS_STRING_CLASSIC)
        return APSIS_STRING_MOTION_RIGHT;
    if (state->string.linked) return APSIS_STRING_MOTION_BOTH;
    if (requested_target > APSIS_STRING_MOTION_BOTH)
        return APSIS_STRING_MOTION_BOTH;
    return requested_target;
}

uint8_t apsis_string_motion_arm(ApsisStringMotionRuntime *motion,
                                ApsisState *state,
                                uint8_t requested_target)
{
    uint8_t target;
    if (!motion || !state || motion->recording) return 0u;
    target = apsis_string_motion_resolve_target(state, requested_target);
    if (motion->armed && motion->record_target == target) {
        motion->armed = 0u;
        return 1u;
    }
    motion->preferred_target = target;
    motion->record_target = target;
    motion->armed = 1u;
    state->string.motion_loop_play = 0u;
    return 1u;
}

void apsis_string_motion_clear(ApsisStringMotionRuntime *motion,
                               ApsisState *state)
{
    if (!motion) return;
    motion->armed = 0u;
    motion->recording = 0u;
    motion->record_target = APSIS_STRING_MOTION_BOTH;
    motion->left.count = 0u;
    motion->right.count = 0u;
    motion->manual_override = 0u;
    motion->just_finished = 0u;
    if (state) state->string.motion_loop_play = 0u;
}

uint8_t apsis_string_motion_toggle_play(ApsisStringMotionRuntime *motion,
                                        ApsisState *state,
                                        uint64_t sample_now)
{
    if (!motion || !state ||
        (motion->left.count < 2u && motion->right.count < 2u)) return 0u;
    state->string.motion_loop_play = state->string.motion_loop_play ? 0u : 1u;
    motion->manual_override = 0u;
    if (state->string.motion_loop_play) {
        motion->play_cycle = 0u;
        motion->play_pulse_index = 0u;
        motion->play_pulse_sample = sample_now;
    }
    return 1u;
}

void apsis_string_motion_pulse(ApsisStringMotionRuntime *motion,
                               ApsisState *state,
                               uint8_t step,
                               uint64_t pulse_sample,
                               uint32_t pulse_interval_samples)
{
    uint16_t n, total;
    uint8_t cycles;
    if (!motion || !state) return;
    n = state->clock.n ? state->clock.n : 12u;
    if (n > APSIS_CLOCK_STEPS_MAX) n = APSIS_CLOCK_STEPS_MAX;
    if (!n) n = 1u;
    if (step >= n) step = (uint8_t)(step % n);
    cycles = motion_length(state);
    total = (uint16_t)(n * cycles);

    if (step == 0u) {
        if (motion->armed && !motion->recording) {
            motion_begin_record(motion, state, pulse_sample, pulse_interval_samples);
        } else if (motion->recording) {
            motion->record_cycles++;
            if (motion->record_cycles >= cycles)
                motion_finish_record(motion, state, pulse_sample, pulse_interval_samples);
        }
    }
    if (motion->recording) {
        motion->record_pulse_index = (uint16_t)(motion->record_cycles * n + step);
        if (motion->record_pulse_index >= total)
            motion->record_pulse_index = (uint16_t)(total - 1u);
        motion->record_pulse_sample = pulse_sample;
        motion->record_pulse_interval = pulse_interval_samples ? pulse_interval_samples : 1u;
        (void)apsis_string_motion_capture(motion, state, pulse_sample, 1u);
    }
    if (state->string.motion_loop_play && !motion->recording) {
        if (step == 0u) {
            if (motion->just_finished) {
                motion->play_cycle = 0u;
                motion->just_finished = 0u;
            } else {
                motion->play_cycle = (uint8_t)((motion->play_cycle + 1u) % cycles);
            }
        }
        motion->play_pulse_index = (uint16_t)((motion->play_cycle * n + step) % total);
        motion->play_pulse_sample = pulse_sample;
        motion->play_pulse_interval = pulse_interval_samples ? pulse_interval_samples : 1u;
    }
}

uint8_t apsis_string_motion_capture(ApsisStringMotionRuntime *motion,
                                    const ApsisState *state,
                                    uint64_t sample_now,
                                    uint8_t force)
{
    uint16_t phase, total;
    if (!motion || !state || !motion->recording) return 0u;
    if (!force && sample_now >= motion->last_capture_sample &&
        sample_now - motion->last_capture_sample < APSIS_STRING_MOTION_CAPTURE_MIN_SAMPLES)
        return 0u;
    motion->last_capture_sample = sample_now;
    total = motion_total_pulses(state);
    phase = motion_phase_q15(motion->record_pulse_index,
                             motion->record_pulse_sample,
                             motion->record_pulse_interval,
                             total, sample_now, 1u);
    if (motion_target_has(motion->record_target, APSIS_SIDE_LEFT))
        motion_track_capture(&motion->left, motion, state, APSIS_SIDE_LEFT, phase);
    if (motion_target_has(motion->record_target, APSIS_SIDE_RIGHT))
        motion_track_capture(&motion->right, motion, state, APSIS_SIDE_RIGHT, phase);
    return 1u;
}

void apsis_string_motion_manual_split(ApsisStringMotionRuntime *motion,
                                      ApsisState *state,
                                      uint8_t side,
                                      uint16_t split_q15,
                                      uint8_t mirror,
                                      uint64_t sample_now)
{
    if (!motion || !state) return;
    if (state->string.motion_loop_play && !motion->recording)
        motion->manual_override = 1u;
    apsis_string_set_side_split(&state->string, side, split_q15, mirror);
    if (motion->recording) {
        motion->owned_left_q15 = state->string.left_split_q15;
        motion->owned_right_q15 = state->string.right_split_q15;
        (void)apsis_string_motion_capture(motion, state, sample_now, 0u);
    }
}

void apsis_string_motion_manual_release(ApsisStringMotionRuntime *motion)
{
    if (motion) motion->manual_override = 0u;
}

void apsis_string_motion_restore_owned(const ApsisStringMotionRuntime *motion,
                                       ApsisState *state)
{
    if (!motion || !state || !motion->recording) return;
    apsis_string_set_side_split(&state->string, APSIS_SIDE_LEFT,
                                motion_clamp_u16(motion->owned_left_q15,
                                                APSIS_STRING_SPLIT_MIN_Q15,
                                                APSIS_STRING_SPLIT_MID_Q15), 0u);
    apsis_string_set_side_split(&state->string, APSIS_SIDE_RIGHT,
                                motion_clamp_u16(motion->owned_right_q15,
                                                APSIS_STRING_SPLIT_MID_Q15,
                                                APSIS_STRING_SPLIT_MAX_Q15), 0u);
}

uint8_t apsis_string_motion_playback(const ApsisStringMotionRuntime *motion,
                                     const ApsisState *state,
                                     uint64_t sample_now,
                                     ApsisStringMotionPlayback *out)
{
    uint16_t phase, total;
    if (!out) return 0u;
    motion_zero(out, (uint32_t)sizeof(*out));
    if (!motion || !state || motion->recording || motion->manual_override ||
        !state->string.motion_loop_play ||
        state->string.pulse_diag != APSIS_STRING_PULSE_FULL ||
        (!motion->left.count && !motion->right.count)) return 0u;
    total = motion_total_pulses(state);
    phase = motion_phase_q15(motion->play_pulse_index,
                             motion->play_pulse_sample,
                             motion->play_pulse_interval,
                             total, sample_now, 0u);
    motion_track_sample(&motion->left, phase, &out->left);
    motion_track_sample(&motion->right, phase, &out->right);
    out->pitch_continuous = state->string.motion_loop_pitch_continuous ? 1u : 0u;
    return (uint8_t)(out->left.valid || out->right.valid);
}
