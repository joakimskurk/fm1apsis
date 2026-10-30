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
 * costs POOL rather than the remaining ordinary static RAM. */
static int32_t apsis_space_target_source[APSIS_SPACE_TARGET_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));
static int32_t apsis_space_target_input[APSIS_SPACE_TARGET_SAMPLES]
    __attribute__((section(".apsis_space_pool"), used, aligned(4)));

static int32_t apsis_space_target_clamp_q15(int64_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int32_t)v;
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
    if (!target) return 0u;
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
    uint32_t i, samples;
    (void)target;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    samples = frames * 2u;
    for (i = 0u; i < samples; ++i) apsis_space_target_source[i] = 0;
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
        int32_t source = apsis_space_target_mul_q15(
            apsis_space_target_source[i], dry_gain);
        int32_t send = apsis_space_target_mul_q15(source, send_q15);
        dry_out[i] = apsis_space_target_clamp_q15((int64_t)dry_out[i] + source);
        apsis_space_target_input[i] += send;
    }
}

void apsis_space_target_render(ApsisSpaceTarget *target,
                               const ApsisSpaceState *state,
                               int32_t *dry_out,
                               uint32_t frames)
{
    if (!target || !target->room_bound || !state || !dry_out) return;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    apsis_space_room_render(&target->room, state,
                            apsis_space_target_input, dry_out, frames);
}
