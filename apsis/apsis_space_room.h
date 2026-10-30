#ifndef APSIS_SPACE_ROOM_H
#define APSIS_SPACE_ROOM_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_space_dsp.h"

#define APSIS_SPACE_HISTORY_MAX_FRAMES 6174u
#define APSIS_SPACE_HISTORY_STORAGE_SAMPLES (APSIS_SPACE_HISTORY_MAX_FRAMES * 2u)
#define APSIS_SPACE_HISTORY_STORAGE_BYTES (APSIS_SPACE_HISTORY_STORAGE_SAMPLES * 2u)
#define APSIS_SPACE_ECHO_MAX_FRAMES 52920u
#define APSIS_SPACE_ECHO_STORAGE_SAMPLES (APSIS_SPACE_ECHO_MAX_FRAMES * 2u)
#define APSIS_SPACE_ECHO_STORAGE_BYTES (APSIS_SPACE_ECHO_STORAGE_SAMPLES * 2u)
#define APSIS_SPACE_EXTERNAL_STORAGE_BYTES \
    (APSIS_SPACE_HISTORY_STORAGE_BYTES + APSIS_SPACE_ECHO_STORAGE_BYTES + \
     APSIS_SPACE_FDN_STORAGE_BYTES)

#define APSIS_SPACE_EARLY_TAPS 4u
#define APSIS_SPACE_ROOM_BLOCK_MAX 256u

typedef struct {
    int32_t z1_q15;
    int32_t z2_q15;
} ApsisSpaceBiquadState;

typedef struct {
    ApsisSpaceFdn fdn;

    int16_t *history_storage;
    uint32_t history_storage_samples;
    uint16_t history_write_frame;
    uint16_t history_fill_frames;

    int16_t *echo_storage;
    uint32_t echo_storage_samples;
    uint32_t echo_write_frame;
    uint32_t echo_fill_frames;

    ApsisSpaceBiquadState guard_hp[2];
    ApsisSpaceBiquadState guard_lp[2];
    ApsisSpaceBiquadState tail_hp[2];
    ApsisSpaceBiquadState echo_lp[2];

    int32_t compressor_gain_q15;
    int32_t wet_q15;
    int32_t target_wet_q15;
    uint32_t predelay_frames_q16;
    uint32_t target_predelay_frames_q16;
    int32_t decay_ms_q16;
    int32_t target_decay_ms_q16;
    int32_t damping_hz_q16;
    int32_t target_damping_hz_q16;
    int32_t guard_lp_hz_q16;
    int32_t target_guard_lp_hz_q16;
    uint32_t echo_delay_frames_q16;
    uint32_t target_echo_delay_frames_q16;
    int32_t echo_feedback_q15;
    int32_t target_echo_feedback_q15;

    uint8_t storage_bound;
    uint8_t params_initialized;
} ApsisSpaceRoom;

/* Bind the three caller-owned NOLOAD-compatible storage classes. No storage is
 * cleared. History validity is represented entirely by fill counters so stale
 * memory remains acoustically invisible until overwritten by this room. */
uint8_t apsis_space_room_bind(ApsisSpaceRoom *room,
                              int16_t *history_storage,
                              uint32_t history_storage_samples,
                              int16_t *echo_storage,
                              uint32_t echo_storage_samples,
                              int16_t *fdn_storage,
                              uint32_t fdn_storage_samples);

void apsis_space_room_reset(ApsisSpaceRoom *room);

/* Add the shared Space contribution to interleaved stereo `out_stereo` from a
 * post-source/post-Mother stereo Q15 room excitation buffer.
 *
 * Source topology preserved here:
 * trim 0.72 -> guard HP/LP/compressor ->
 *   (a) 21/43/71/109 ms early taps -> early gain 0.16 -> direct output
 *   (b) predelay + quiet feedback echo -> 190 Hz HP -> selected 8-line FDN
 *       -> damped wet return.
 *
 * Early reflections are intentionally independent of Space wet mix, matching
 * Study 126 where `early` connects directly to master. Large memories remain
 * external to this small control/filter state object. */
void apsis_space_room_render(ApsisSpaceRoom *room,
                             const ApsisSpaceState *state,
                             const int32_t *input_stereo_q15,
                             int32_t *out_stereo,
                             uint32_t frames);

#endif
