#ifndef APSIS_SPACE_TARGET_H
#define APSIS_SPACE_TARGET_H

#include <stdint.h>
#include "apsis_state.h"
#include "apsis_space_room.h"

typedef struct {
    ApsisSpaceRoom room;
    uint8_t room_bound;
    uint8_t input_seeded;
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
void apsis_space_target_render(ApsisSpaceTarget *target,
                               const ApsisSpaceState *state,
                               int32_t *dry_out,
                               uint32_t frames);

#endif
