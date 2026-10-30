#include "apsis_space_target.h"

#define APSIS_SPACE_TARGET_SAMPLES (APSIS_SPACE_ROOM_BLOCK_MAX * 2u)

/* Phase-6 production storage. This section is placed immediately before
 * Dinghy's ordinary .pool so the linker still accounts it against POOL while
 * fm1_cstart() does not spend boot time clearing it. The room's fill counters
 * make stale NOLOAD delay contents inaudible until overwritten. */
static int16_t apsis_space_target_history[APSIS_SPACE_HISTORY_STORAGE_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));
static int16_t apsis_space_target_echo[APSIS_SPACE_ECHO_STORAGE_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));
static int16_t apsis_space_target_fdn[APSIS_SPACE_FDN_STORAGE_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));

/* One reusable source bus plus one shared room-input block. Keeping both here
 * costs POOL rather than the remaining ordinary static RAM. The source bus is
 * cleared as each source is committed, eliminating a full extra buffer pass
 * before every renderer in the audio callback. */
static int32_t apsis_space_target_source[APSIS_SPACE_TARGET_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));
static int32_t apsis_space_target_input[APSIS_SPACE_TARGET_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));

static int32_t apsis_space_target_clamp_q15(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return v;
}

static int32_t apsis_space_target_mul_q15(int32_t sample, uint16_t gain)
{
    int32_t p;
    if (!gain || !sample) return 0;
    if (gain >= APSIS_Q15_ONE) return sample;
    /* sample is a Q15 source-bus value, so the 32-bit product is bounded by
     * 32767^2 and cannot overflow. Use the state's exact 32767 denominator. */
    p = sample * (int32_t)gain;
    if (p >= 0) p += (int32_t)(APSIS_Q15_ONE / 2u);
    else p -= (int32_t)(APSIS_Q15_ONE / 2u);
    return p / (int32_t)APSIS_Q15_ONE;
}

uint8_t apsis_space_target_bind(ApsisSpaceTarget *target)
{
    uint32_t i;
    if (!target) return 0u;

    /* These two scratch buses are NOLOAD and therefore may contain stale RAM
     * at boot. Clear only the small scratch area here; the 268-KiB delay pool
     * remains intentionally uncleared and is guarded by room fill counters. */
    for (i = 0u; i < APSIS_SPACE_TARGET_SAMPLES; ++i) {
        apsis_space_target_source[i] = 0;
        apsis_space_target_input[i] = 0;
    }

    target->room_bound = apsis_space_room_bind(
        &target->room,
        apsis_space_target_history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES,
        apsis_space_target_echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES,
        apsis_space_target_fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES);
    return target->room_bound;
}

void apsis_space_target_begin_block(ApsisSpaceTarget *target, uint32_t frames)
{
    uint32_t i, samples;
    (void)target;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    samples = frames * 2u;
    for (i = 0u; i < samples; ++i) apsis_space_target_input[i] = 0;
}

int32_t *apsis_space_target_source_buffer(ApsisSpaceTarget *target,
                                          uint32_t frames)
{
    (void)target;
    (void)frames;
    /* commit_source() consumes and clears every rendered sample. Returning the
     * already-zero scratch bus avoids one 512-word pass for every source. */
    return apsis_space_target_source;
}

void apsis_space_target_commit_source(ApsisSpaceTarget *target,
                                      int32_t *dry_out,
                                      uint32_t frames,
                                      uint16_t send_q15,
                                      uint16_t mother_mix_q15,
                                      uint8_t mother_muted)
{
    uint32_t i, samples;
    uint16_t dry_gain;
    if (!target || !dry_out) return;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    samples = frames * 2u;
    dry_gain = mother_muted ? 0u : mother_mix_q15;
    for (i = 0u; i < samples; ++i) {
        int32_t raw = apsis_space_target_source[i];
        int32_t source;
        int32_t send;
        /* Consume first so the scratch invariant holds even when the source is
         * silent/muted and no later renderer inherits stale samples. */
        apsis_space_target_source[i] = 0;
        source = apsis_space_target_mul_q15(raw, dry_gain);
        send = apsis_space_target_mul_q15(source, send_q15);
        dry_out[i] = apsis_space_target_clamp_q15(dry_out[i] + source);
        apsis_space_target_input[i] += send;
    }
}

void apsis_space_target_render(ApsisSpaceTarget *target,
                               const ApsisSpaceState *state,
                               int32_t *dry_out,
                               uint32_t frames)
{
    uint32_t i, room_frames, complete_groups, remainder;
    if (!target || !target->room_bound || !state || !dry_out) return;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;

    /* Source engines and the public callback stay at 44.1 kHz. The shared,
     * deliberately bandwidth-limited Cathedral bus advances at 11.025 kHz.
     * Four adjacent sends are box-filtered/averaged into one room frame; the
     * resulting wet sample is held across the same four output frames. The
     * generated target room is retimed by ci/patch_apsis_p6_room_rate.py so
     * predelay, early taps, echo, FDN decay, damping and modulation retain their
     * intended real-time behaviour. The host/reference room remains 44.1 kHz. */
    complete_groups = frames >> 2u;
    remainder = frames & 3u;
    room_frames = complete_groups + (remainder ? 1u : 0u);
    if (!room_frames) return;

    for (i = 0u; i < complete_groups; ++i) {
        uint32_t in0 = i * 8u;
        uint32_t room = i * 2u;
        apsis_space_target_input[room] =
            (apsis_space_target_input[in0] +
             apsis_space_target_input[in0 + 2u] +
             apsis_space_target_input[in0 + 4u] +
             apsis_space_target_input[in0 + 6u]) / 4;
        apsis_space_target_input[room + 1u] =
            (apsis_space_target_input[in0 + 1u] +
             apsis_space_target_input[in0 + 3u] +
             apsis_space_target_input[in0 + 5u] +
             apsis_space_target_input[in0 + 7u]) / 4;
        apsis_space_target_source[room] = 0;
        apsis_space_target_source[room + 1u] = 0;
    }

    if (remainder) {
        uint32_t frame0 = complete_groups * 4u;
        uint32_t in0 = frame0 * 2u;
        uint32_t room = complete_groups * 2u;
        int32_t sum_l = 0;
        int32_t sum_r = 0;
        uint32_t j;
        for (j = 0u; j < remainder; ++j) {
            sum_l += apsis_space_target_input[in0 + j * 2u];
            sum_r += apsis_space_target_input[in0 + j * 2u + 1u];
        }
        apsis_space_target_input[room] = sum_l / (int32_t)remainder;
        apsis_space_target_input[room + 1u] = sum_r / (int32_t)remainder;
        apsis_space_target_source[room] = 0;
        apsis_space_target_source[room + 1u] = 0;
    }

    apsis_space_room_render(&target->room, state,
                            apsis_space_target_input,
                            apsis_space_target_source,
                            room_frames);

    for (i = 0u; i < complete_groups; ++i) {
        uint32_t room = i * 2u;
        uint32_t out0 = i * 8u;
        uint32_t j;
        int32_t wet_l = apsis_space_target_source[room];
        int32_t wet_r = apsis_space_target_source[room + 1u];
        for (j = 0u; j < 4u; ++j) {
            uint32_t o = out0 + j * 2u;
            dry_out[o] = apsis_space_target_clamp_q15(dry_out[o] + wet_l);
            dry_out[o + 1u] = apsis_space_target_clamp_q15(dry_out[o + 1u] + wet_r);
        }
        apsis_space_target_source[room] = 0;
        apsis_space_target_source[room + 1u] = 0;
    }

    if (remainder) {
        uint32_t room = complete_groups * 2u;
        uint32_t out0 = complete_groups * 8u;
        uint32_t j;
        int32_t wet_l = apsis_space_target_source[room];
        int32_t wet_r = apsis_space_target_source[room + 1u];
        for (j = 0u; j < remainder; ++j) {
            uint32_t o = out0 + j * 2u;
            dry_out[o] = apsis_space_target_clamp_q15(dry_out[o] + wet_l);
            dry_out[o + 1u] = apsis_space_target_clamp_q15(dry_out[o + 1u] + wet_r);
        }
        apsis_space_target_source[room] = 0;
        apsis_space_target_source[room + 1u] = 0;
    }
}
