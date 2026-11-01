#ifndef APSIS_CONTROLS_MOTHER_H
#define APSIS_CONTROLS_MOTHER_H

#include <stdint.h>
#include "apsis_controls.h"
#include "apsis_mother.h"

#define APSIS_MOTHER_SOURCE_COUNT 5u
#define APSIS_MOTHER_MIX_STEP_Q15 APSIS_Q15_MILLI(30)
#define APSIS_MOTHER_MIX_MAX_Q15  APSIS_Q15_MILLI(1250)

typedef enum {
    APSIS_MOTHER_SOURCE_STRING = 0,
    APSIS_MOTHER_SOURCE_DRIFT,
    APSIS_MOTHER_SOURCE_CLOCK,
    APSIS_MOTHER_SOURCE_SPIRAL,
    APSIS_MOTHER_SOURCE_BLOOM
} ApsisMotherSource;

typedef struct {
    uint8_t source;
    uint16_t value_q15;
} ApsisMotherMixEdit;

uint8_t apsis_controls_mother_mix_edits(const ApsisMotherState *mother,
                                        const ApsisFm1InputFrame *input,
                                        ApsisMotherMixEdit out[APSIS_MOTHER_SOURCE_COUNT]);
uint8_t apsis_controls_mother_apply_mix(ApsisMotherState *mother,
                                        const ApsisMotherMixEdit *edit);
uint8_t apsis_controls_mother_mute_source_from_fm1(const ApsisFm1InputFrame *input,
                                                   uint8_t *source_out);
uint8_t apsis_controls_mother_toggle_source_mute(ApsisState *state,
                                                 uint8_t source);
uint8_t apsis_controls_mother_sync_from_fm1(ApsisMotherState *mother,
                                            ApsisMotherPending *pending,
                                            const ApsisFm1InputFrame *input);
uint8_t apsis_controls_mother_horizon_from_fm1(ApsisMotherState *mother,
                                               ApsisMotherPending *pending,
                                               const ApsisFm1InputFrame *input,
                                               int32_t last_abs_step);

#endif
