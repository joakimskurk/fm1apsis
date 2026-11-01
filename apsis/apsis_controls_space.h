#ifndef APSIS_CONTROLS_SPACE_H
#define APSIS_CONTROLS_SPACE_H

#include <stdint.h>
#include "apsis_controls_mother.h"

typedef enum {
    APSIS_SPACE_CONTROL_ROOM = 0,
    APSIS_SPACE_CONTROL_ECHO_SEND,
    APSIS_SPACE_CONTROL_PAGE_COUNT
} ApsisSpaceControlPage;

typedef enum {
    APSIS_SPACE_PARAM_NONE = 0,
    APSIS_SPACE_PARAM_RETURN,
    APSIS_SPACE_PARAM_DECAY_MS,
    APSIS_SPACE_PARAM_PREDELAY_MS,
    APSIS_SPACE_PARAM_DAMPING_HZ,
    APSIS_SPACE_PARAM_ECHO_TIME_MS,
    APSIS_SPACE_PARAM_ECHO_FEEDBACK,
    APSIS_SPACE_PARAM_SOURCE_SEND
} ApsisSpaceControlParam;

typedef struct {
    uint8_t page;
    uint8_t source;
} ApsisSpaceControlState;

typedef struct {
    uint8_t param;
    uint8_t source;
    int32_t value;
} ApsisSpaceControlEdit;

void apsis_controls_space_reset(ApsisSpaceControlState *control);
uint8_t apsis_controls_space_page_step(ApsisSpaceControlState *control,
                                       int32_t detents);
uint8_t apsis_controls_space_source_step(ApsisSpaceControlState *control,
                                         int32_t detents);
uint8_t apsis_controls_space_edits(const ApsisState *state,
                                   const ApsisSpaceControlState *control,
                                   const ApsisFm1InputFrame *input,
                                   ApsisSpaceControlEdit out[4]);

#endif
