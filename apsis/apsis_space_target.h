#ifndef APSIS_SPACE_TARGET_H
#define APSIS_SPACE_TARGET_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_space_room.h"

typedef struct {
    ApsisSpaceRoom room;
    uint8_t room_bound;
    uint8_t input_seeded;
    uint32_t room_frames;       /* set aside by room_prepare, at the room's rate, for room_wet */
    uint32_t room_out_frames;
    uint32_t wet_frames;        /* rendered by room_wet, for room_add */
    uint32_t wet_out_frames;
} ApsisSpaceTarget;

/* Target-only bridge between the five frozen dry source renderers and the
 * Phase-6 shared Space room. Large delay/scratch storage is owned by the
 * implementation in the dedicated NOLOAD pool section. */
uint8_t apsis_space_target_bind(ApsisSpaceTarget *target);
void apsis_space_target_begin_block(ApsisSpaceTarget *target, uint32_t frames);
int32_t *apsis_space_target_source_buffer(ApsisSpaceTarget *target,
                                          uint32_t frames);
void apsis_space_target_commit_source(ApsisSpaceTarget *target,
                                      int32_t *dry_out,
                                      uint32_t frames,
                                      uint16_t send_q15,
                                      uint16_t mother_mix_q15,
                                      uint8_t mother_muted);
/* commit_source() from a source's own zeroed buffer (rendered elsewhere, e.g. on the second core): the same sum
 * and clamp, the buffer zeroed again */
void apsis_space_target_commit_buffer(ApsisSpaceTarget *target,
                                      int32_t *buffer,
                                      int32_t *dry_out,
                                      uint32_t frames,
                                      uint16_t send_q15,
                                      uint16_t mother_mix_q15,
                                      uint8_t mother_muted);
/* the room one block late, in three steps that let the second core run the middle one (apsis_space_target.c):
 * prepare after a block's commits, wet during the next block, add after the next block's commits */
void apsis_space_target_room_prepare(ApsisSpaceTarget *target, uint32_t frames);
void apsis_space_target_room_wet(ApsisSpaceTarget *target, const ApsisSpaceState *state);
void apsis_space_target_room_add(ApsisSpaceTarget *target, int32_t *dry_out, uint32_t frames);

#endif
