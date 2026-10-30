#ifndef APSIS_PROJECT_FM1_H
#define APSIS_PROJECT_FM1_H

#include <stdint.h>
#include "apsis_project_store.h"

typedef struct {
    uint32_t generation;
    int8_t active_slot;
    uint8_t save_pending;
    int16_t last_result;
    uint8_t record[APSIS_PROJECT_RECORD_SIZE];
} ApsisProjectFm1;

void apsis_project_fm1_reset(ApsisProjectFm1 *store);

/* Reads only. Safe to call during app startup before APSIS runtime_ready. */
int apsis_project_fm1_load(ApsisProjectFm1 *store,
                           ApsisKeepBank *keep,
                           ApsisChainSequence *chain,
                           ApsisChainState *chain_settings);

/* Request is ISR/audio-safe because it only sets one byte. The actual flash
 * operation is deliberately serviced later from app_main, never app_audio. */
void apsis_project_fm1_request_save(ApsisProjectFm1 *store);

/* Caller must have quiesced FM-1 audio/DMA and must call this only from the
 * main loop. The save alternates 4 KiB sectors and commits the header last. */
int apsis_project_fm1_service_save(ApsisProjectFm1 *store,
                                   const ApsisKeepBank *keep,
                                   const ApsisChainSequence *chain,
                                   const ApsisChainState *chain_settings);

#endif
