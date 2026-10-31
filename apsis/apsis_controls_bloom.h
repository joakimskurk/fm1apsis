#ifndef APSIS_CONTROLS_BLOOM_H
#define APSIS_CONTROLS_BLOOM_H

#include <stdint.h>
#include "apsis_controls.h"
#include "apsis_bloom.h"

/* The FM-1 has 27 note keys; index 13 is the physical centre key. Individual
 * petals use the lowest 3..5 keys, so CORE never collides with a petal key. */
#define APSIS_BLOOM_CORE_KEY 13u

typedef enum {
    APSIS_BLOOM_CONTROL_TRIGGER_NONE = 0,
    APSIS_BLOOM_CONTROL_TRIGGER_PETAL,
    APSIS_BLOOM_CONTROL_TRIGGER_CORE
} ApsisBloomControlTrigger;

/* P9E1 deliberately emits performance intent only. Grow sensitivity and the
 * browser's per-petal radial thresholds are not invented here: REC ownership,
 * SELECT detent motion and begin/end edges are preserved for P9H to bind to a
 * source-proven grow adapter. */
typedef struct {
    uint8_t trigger;
    uint8_t petal_index;
    uint8_t grow_begin;
    uint8_t grow_end;
    int16_t grow_detents;
} ApsisBloomControlFrame;

uint8_t apsis_controls_bloom_frame(const ApsisState *state,
                                   const ApsisFm1InputFrame *input,
                                   ApsisBloomControlFrame *out);

#endif
