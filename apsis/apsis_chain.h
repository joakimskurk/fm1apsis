#ifndef APSIS_CHAIN_H
#define APSIS_CHAIN_H

#include <stdint.h>
#include "apsis_keep.h"

#define APSIS_CHAIN_MORPH_NONE    0u
#define APSIS_CHAIN_MORPH_QUARTER 1u
#define APSIS_CHAIN_MORPH_HALF    2u
#define APSIS_CHAIN_MORPH_FULL    4u

typedef struct {
    uint8_t slot;
    uint8_t cycles;
    uint8_t fine_pulses;
    uint16_t chance_q15;
    int8_t transpose;
    uint8_t morph_quarters;
} ApsisChainCell;

typedef struct {
    ApsisChainCell cells[APSIS_CHAIN_MAX];
    uint8_t count;
    int8_t selected;
    uint8_t playing;
    uint8_t armed;
    uint8_t pos;
    uint32_t pulse;
    uint32_t phrase_pulse;
    uint32_t master_pulses;
    uint8_t hold_tail;
    uint8_t last_applied;
} ApsisChainSequence;

typedef struct {
    uint8_t morph_quarters;
    uint8_t has_tension;
    uint16_t target_left_split_q15;
    uint16_t target_right_split_q15;
    uint8_t has_drift;
    uint16_t target_drift_amount_q15;
} ApsisChainMorphTargets;

void apsis_chain_sequence_reset(ApsisChainSequence *seq);
uint8_t apsis_chain_morph_valid(uint8_t morph_quarters);
ApsisChainCell apsis_chain_default_cell(uint8_t slot, const ApsisChainState *settings,
                                        uint8_t clock_n);
ApsisChainCell apsis_chain_normalize_cell(const ApsisChainCell *cell,
                                          const ApsisChainState *settings,
                                          uint8_t clock_n);
uint16_t apsis_chain_cell_pulses(const ApsisChainCell *cell,
                                 const ApsisChainState *settings,
                                 uint8_t clock_n);
uint32_t apsis_chain_content_pulses(const ApsisChainSequence *seq,
                                    const ApsisChainState *settings,
                                    uint8_t clock_n);
uint32_t apsis_chain_master_pulses(const ApsisChainSequence *seq,
                                   const ApsisChainState *settings,
                                   uint8_t clock_n);
uint8_t apsis_chain_should_apply(uint16_t chance_q15, uint32_t random_u32);
void apsis_chain_morph_targets(const ApsisKeepSnapshot *snap,
                               const ApsisChainState *settings,
                               uint8_t morph_quarters,
                               ApsisChainMorphTargets *out);
uint8_t apsis_chain_apply_cell(ApsisState *state, const ApsisKeepBank *bank,
                               const ApsisChainState *settings,
                               const ApsisChainCell *cell, uint32_t random_u32,
                               ApsisChainMorphTargets *morph_targets);

#endif
