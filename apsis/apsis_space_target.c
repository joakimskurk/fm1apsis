#include "apsis_sine.h"
#include "apsis_space_target.h"
#include "apsis_fixed.h"

#ifdef __APPLE__                                /* (Mach-O host tests: no ELF section names) */
#define APSIS_SPACE_POOL __attribute__((used, aligned(4)))
#else
#define APSIS_SPACE_POOL __attribute__((section(".pool.apsis_space"), used, aligned(4)))
#endif
#define APSIS_SPACE_TARGET_SAMPLES (APSIS_SPACE_ROOM_BLOCK_MAX * 2u)

/* The room's delay memory lives in Dinghy's POOL (.pool*, app.ld): zeroed at boot, and the room's fill counters
 * keep unwritten history inaudible anyway. */
static int16_t apsis_space_target_history[APSIS_SPACE_HISTORY_STORAGE_SAMPLES]
    APSIS_SPACE_POOL;
static int16_t apsis_space_target_echo[APSIS_SPACE_ECHO_STORAGE_SAMPLES]
    APSIS_SPACE_POOL;
static int16_t apsis_space_target_fdn[APSIS_SPACE_FDN_STORAGE_SAMPLES]
    APSIS_SPACE_POOL;

/* One reusable source bus plus one shared room-input block. Keeping both here
 * costs POOL rather than the remaining ordinary static RAM. The source bus is
 * cleared as each source is committed. The input bus is overwritten by the
 * first committed source in every block, avoiding a separate 512-word clear. */
static int32_t apsis_space_target_source[APSIS_SPACE_TARGET_SAMPLES]
    APSIS_SPACE_POOL;
static int32_t apsis_space_target_input[APSIS_SPACE_TARGET_SAMPLES]
    APSIS_SPACE_POOL;

APSIS_INLINE int32_t apsis_space_target_clamp_q15(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return v;
}

APSIS_INLINE int32_t apsis_space_target_mul_q15(int32_t sample, uint16_t gain)
{
    if (!gain || !sample) return 0;
    if (gain >= APSIS_Q15_ONE) return sample;
    /* sample is a Q15 source-bus value: the product is bounded by 32767^2. The state's exact 32767
     * denominator, rounded half away from zero, without a divide (apsis_fixed.h). */
    return apsis_mul_q15_round(sample, (int32_t)gain);
}

uint8_t apsis_space_target_bind(ApsisSpaceTarget *target)
{
    uint32_t i;
    if (!target) return 0u;

    /* The reusable source scratch is NOLOAD and may contain stale RAM at boot.
     * The input bus needs no boot clear: the first source of every audio block
     * overwrites every active sample before any room render can read it. */
    for (i = 0u; i < APSIS_SPACE_TARGET_SAMPLES; ++i)
        apsis_space_target_source[i] = 0;

    target->input_seeded = 0u;
    target->room_frames = target->room_out_frames = 0u;
    target->wet_frames = target->wet_out_frames = 0u;
    target->room_bound = apsis_space_room_bind(
        &target->room,
        apsis_space_target_history, APSIS_SPACE_HISTORY_STORAGE_SAMPLES,
        apsis_space_target_echo, APSIS_SPACE_ECHO_STORAGE_SAMPLES,
        apsis_space_target_fdn, APSIS_SPACE_FDN_STORAGE_SAMPLES);
    return target->room_bound;
}

void apsis_space_target_begin_block(ApsisSpaceTarget *target, uint32_t frames)
{
    (void)frames;
    if (!target) return;
    /* The first commit seeds the entire active input block. This marker replaces
     * the old full-buffer zero pass while preserving exactly the same sum. */
    target->input_seeded = 0u;
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

void apsis_space_target_commit_buffer(ApsisSpaceTarget *target,
                                      int32_t *buffer,
                                      int32_t *dry_out,
                                      uint32_t frames,
                                      uint16_t send_q15,
                                      uint16_t mother_mix_q15,
                                      uint8_t mother_muted)
{
    uint32_t i, samples;
    uint16_t dry_gain;
    if (!target || !buffer || !dry_out) return;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    samples = frames * 2u;
    dry_gain = mother_muted ? 0u : mother_mix_q15;

    if (!target->input_seeded) {
        /* First source: overwrite the room input instead of clearing it in a
         * separate pass and then adding. This saves both a store-only pass and
         * one read/add per sample while producing the identical send sum. */
        for (i = 0u; i < samples; ++i) {
            int32_t raw = buffer[i];
            int32_t source;
            int32_t send;
            buffer[i] = 0;
            source = apsis_space_target_mul_q15(raw, dry_gain);
            send = apsis_space_target_mul_q15(source, send_q15);
            dry_out[i] = apsis_space_target_clamp_q15(dry_out[i] + source);
            apsis_space_target_input[i] = send;
        }
        target->input_seeded = 1u;
        return;
    }

    for (i = 0u; i < samples; ++i) {
        int32_t raw = buffer[i];
        int32_t source;
        int32_t send;
        buffer[i] = 0;
        source = apsis_space_target_mul_q15(raw, dry_gain);
        send = apsis_space_target_mul_q15(source, send_q15);
        dry_out[i] = apsis_space_target_clamp_q15(dry_out[i] + source);
        apsis_space_target_input[i] += send;
    }
}

void apsis_space_target_commit_source(ApsisSpaceTarget *target,
                                      int32_t *dry_out,
                                      uint32_t frames,
                                      uint16_t send_q15,
                                      uint16_t mother_mix_q15,
                                      uint8_t mother_muted)
{
    apsis_space_target_commit_buffer(target, apsis_space_target_source, dry_out, frames, send_q15, mother_mix_q15,
                                     mother_muted);
}

/* The room, split for the second core (docs/MARGIN_PLAN.md D): one block late.
 * room_prepare (core 0, after the commits): this block's sends, box-filtered to the room's 11.025 kHz, set aside.
 * room_wet (core 1, during the next block): the room over those, into its wet buffer.
 * room_add (core 0, after the next block's commits): the wet frames held across four output frames, clamped into
 * the dry bus, as the one-block render did. */
static int32_t apsis_space_target_room_in[APSIS_SPACE_TARGET_SAMPLES / 4u + 2u] APSIS_SPACE_POOL;
static int32_t apsis_space_target_wet_buf[APSIS_SPACE_TARGET_SAMPLES / 4u + 2u] APSIS_SPACE_POOL;

void apsis_space_target_room_prepare(ApsisSpaceTarget *target, uint32_t frames)
{
    uint32_t i, complete_groups, remainder;
    if (!target) return;
    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;
    complete_groups = frames >> 2u;
    remainder = frames & 3u;
    for (i = 0u; i < complete_groups; ++i) {
        uint32_t in0 = i * 8u;
        apsis_space_target_room_in[i * 2u] =
            (apsis_space_target_input[in0] + apsis_space_target_input[in0 + 2u] +
             apsis_space_target_input[in0 + 4u] + apsis_space_target_input[in0 + 6u]) / 4;
        apsis_space_target_room_in[i * 2u + 1u] =
            (apsis_space_target_input[in0 + 1u] + apsis_space_target_input[in0 + 3u] +
             apsis_space_target_input[in0 + 5u] + apsis_space_target_input[in0 + 7u]) / 4;
    }
    if (remainder) {
        uint32_t in0 = complete_groups * 8u, j;
        int32_t sum_l = 0, sum_r = 0;
        for (j = 0u; j < remainder; ++j) {
            sum_l += apsis_space_target_input[in0 + j * 2u];
            sum_r += apsis_space_target_input[in0 + j * 2u + 1u];
        }
        apsis_space_target_room_in[complete_groups * 2u] = sum_l / (int32_t)remainder;
        apsis_space_target_room_in[complete_groups * 2u + 1u] = sum_r / (int32_t)remainder;
    }
    target->room_frames = complete_groups + (remainder ? 1u : 0u);
    target->room_out_frames = frames;
}

void apsis_space_target_room_wet(ApsisSpaceTarget *target, const ApsisSpaceState *state)
{
    uint32_t i;
    if (!target) return;
    target->wet_frames = 0u;
    target->wet_out_frames = 0u;
    if (!target->room_bound || !state || !target->room_frames) return;
    for (i = 0u; i < target->room_frames * 2u; ++i) apsis_space_target_wet_buf[i] = 0;
    apsis_space_room_render(&target->room, state, apsis_space_target_room_in, apsis_space_target_wet_buf,
                            target->room_frames);
    target->wet_frames = target->room_frames;
    target->wet_out_frames = target->room_out_frames;
    target->room_frames = 0u;
}

void apsis_space_target_room_add(ApsisSpaceTarget *target, int32_t *dry_out, uint32_t frames)
{
    uint32_t f;
    if (!target || !dry_out) return;
    if (frames > target->wet_out_frames) frames = target->wet_out_frames;
    for (f = 0u; f < frames; ++f) {
        int32_t wet_l = apsis_space_target_wet_buf[(f >> 2) * 2u];
        int32_t wet_r = apsis_space_target_wet_buf[(f >> 2) * 2u + 1u];
        dry_out[f * 2u] = apsis_space_target_clamp_q15(dry_out[f * 2u] + wet_l);
        dry_out[f * 2u + 1u] = apsis_space_target_clamp_q15(dry_out[f * 2u + 1u] + wet_r);
    }
    target->wet_out_frames = 0u;
}
