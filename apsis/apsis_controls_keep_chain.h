#ifndef APSIS_CONTROLS_KEEP_CHAIN_H
#define APSIS_CONTROLS_KEEP_CHAIN_H

#include <stdint.h>
#include "apsis_controls.h"
#include "apsis_chain.h"

#define APSIS_KEEP_DIRECT_KEYS APSIS_KEEP_SLOTS

typedef enum {
    APSIS_KEEP_SLOT_NONE = 0,
    APSIS_KEEP_SLOT_SELECTED,
    APSIS_KEEP_SLOT_RECALLED
} ApsisKeepSlotResult;

typedef enum {
    APSIS_KEEP_PANEL_NONE = 0,
    APSIS_KEEP_PANEL_CAPTURE,
    APSIS_KEEP_PANEL_ERASE
} ApsisKeepPanelAction;

typedef enum {
    APSIS_CHAIN_CONTEXT_NONE = 0,
    APSIS_CHAIN_CONTEXT_DUPLICATE,
    APSIS_CHAIN_CONTEXT_DELETE
} ApsisChainContextAction;

typedef enum {
    APSIS_CHAIN_TRANSPORT_NONE = 0,
    APSIS_CHAIN_TRANSPORT_START,
    APSIS_CHAIN_TRANSPORT_STOP
} ApsisChainTransportIntent;

typedef struct {
    uint8_t changed;
    uint8_t active_removed;
    uint8_t apply_current;
} ApsisChainDeleteResult;

uint8_t apsis_controls_keep_slot_from_fm1(const ApsisFm1InputFrame *input,
                                          uint8_t *slot_out);
ApsisKeepPanelAction apsis_controls_keep_panel_action(const ApsisFm1InputFrame *input);
ApsisKeepSlotResult apsis_controls_keep_activate_slot(ApsisKeepBank *bank,
                                                      ApsisState *state,
                                                      uint8_t slot);
int8_t apsis_controls_keep_capture(ApsisKeepBank *bank, const ApsisState *state);
uint8_t apsis_controls_keep_erase_selected(ApsisKeepBank *bank,
                                           ApsisChainSequence *chain);

uint8_t apsis_controls_chain_append_slot(ApsisChainSequence *chain,
                                         const ApsisKeepBank *bank,
                                         const ApsisChainState *settings,
                                         uint8_t clock_n,
                                         uint8_t slot);
int8_t apsis_controls_chain_cursor_step(ApsisChainSequence *chain, int32_t detents);
uint8_t apsis_controls_chain_apply_encoders(ApsisChainSequence *chain,
                                            const ApsisKeepBank *bank,
                                            const ApsisChainState *settings,
                                            uint8_t clock_n,
                                            const ApsisFm1InputFrame *input);
uint8_t apsis_controls_chain_duplicate_selected(ApsisChainSequence *chain,
                                                const ApsisChainState *settings,
                                                uint8_t clock_n);
ApsisChainDeleteResult apsis_controls_chain_delete_selected(ApsisChainSequence *chain);
ApsisChainContextAction apsis_controls_chain_context_action(const ApsisFm1InputFrame *input);
ApsisChainTransportIntent apsis_controls_chain_transport_intent(const ApsisChainSequence *chain,
                                                                const ApsisFm1InputFrame *input);

#endif
