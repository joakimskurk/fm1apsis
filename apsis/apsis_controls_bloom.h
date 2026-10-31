#ifndef APSIS_CONTROLS_BLOOM_H
#define APSIS_CONTROLS_BLOOM_H

#include <stdint.h>
#include "apsis_controls.h"
#include "apsis_bloom.h"

/* The FM-1 has 27 note keys; index 13 is the physical centre key. Individual
 * petals use the lowest 3..5 keys, so CORE never collides with a petal key. */
#define APSIS_BLOOM_CORE_KEY 13u
#define APSIS_BLOOM_GROW_STEP_Q15 APSIS_Q15_MILLI(40)

typedef enum {
    APSIS_BLOOM_CONTROL_TRIGGER_NONE = 0,
    APSIS_BLOOM_CONTROL_TRIGGER_PETAL,
    APSIS_BLOOM_CONTROL_TRIGGER_CORE
} ApsisBloomControlTrigger;

typedef struct {
    uint8_t trigger;
    uint8_t petal_index;
    uint8_t grow_begin;
    uint8_t grow_end;
    int16_t grow_detents;
} ApsisBloomControlFrame;

typedef struct {
    uint16_t open_q15;
    uint8_t fired_mask;
} ApsisBloomGrowGesture;

typedef enum {
    APSIS_BLOOM_PARAM_NONE = 0,
    APSIS_BLOOM_PARAM_UNFURL,
    APSIS_BLOOM_PARAM_SPREAD,
    APSIS_BLOOM_PARAM_LENGTH,
    APSIS_BLOOM_PARAM_BRIGHTNESS
} ApsisBloomControlParam;

typedef struct {
    uint8_t param;
    int32_t value;
} ApsisBloomParamEdit;

uint8_t apsis_controls_bloom_frame(const ApsisState *state,
                                   const ApsisFm1InputFrame *input,
                                   ApsisBloomControlFrame *out);
void apsis_controls_bloom_grow_reset(ApsisBloomGrowGesture *gesture);
uint8_t apsis_controls_bloom_grow_update(ApsisBloomGrowGesture *gesture,
                                         uint8_t voices,
                                         int32_t select_detents);
uint8_t apsis_controls_bloom_param_edits(const ApsisBloomState *bloom,
                                         const ApsisFm1InputFrame *input,
                                         ApsisBloomParamEdit out[4]);

#endif
