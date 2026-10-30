#ifndef APSIS_PROJECT_STORE_H
#define APSIS_PROJECT_STORE_H

#include <stdint.h>
#include "apsis_chain.h"

/* Phase 7 persistent project format. The wire format is explicit little-endian
 * bytes; it never serializes native C structs or padding. */
#define APSIS_PROJECT_FORMAT_VERSION 1u
#define APSIS_PROJECT_RECORD_MAGIC 0x37535041u /* bytes: "APS7" */
#define APSIS_PROJECT_HEADER_SIZE 24u
#define APSIS_PROJECT_KEEP_WIRE_SIZE 72u
#define APSIS_PROJECT_CHAIN_CELL_WIRE_SIZE 7u
#define APSIS_PROJECT_CHAIN_SETTINGS_WIRE_SIZE 13u
#define APSIS_PROJECT_PAYLOAD_SIZE 816u
#define APSIS_PROJECT_RECORD_SIZE (APSIS_PROJECT_HEADER_SIZE + APSIS_PROJECT_PAYLOAD_SIZE)
#define APSIS_PROJECT_SECTOR_SIZE 4096u
#define APSIS_PROJECT_PAYLOAD_OFFSET APSIS_PROJECT_HEADER_SIZE

/* Dedicated FM-1 project sectors selected in Phase 7. They intentionally do
 * not use Dinghy's 0xFC000 one-sector settings store. */
#define APSIS_PROJECT_FM1_SLOT_A 0x000F4000u
#define APSIS_PROJECT_FM1_SLOT_B 0x000F5000u

typedef struct {
    uint8_t valid;
    uint32_t generation;
} ApsisProjectRecordInfo;

uint32_t apsis_project_crc32(const uint8_t *data, uint32_t n);

int apsis_project_payload_encode(uint8_t *dst, uint32_t capacity,
                                 const ApsisKeepBank *keep,
                                 const ApsisChainSequence *chain,
                                 const ApsisChainState *chain_settings);
int apsis_project_payload_decode(const uint8_t *src, uint32_t n,
                                 ApsisKeepBank *keep,
                                 ApsisChainSequence *chain,
                                 ApsisChainState *chain_settings);

/* Build a compact record. On FM-1, write bytes [HEADER_SIZE, RECORD_SIZE)
 * first and the header last. A torn save therefore cannot become valid. */
int apsis_project_record_build(uint8_t *dst, uint32_t capacity,
                               uint32_t generation,
                               const ApsisKeepBank *keep,
                               const ApsisChainSequence *chain,
                               const ApsisChainState *chain_settings);
int apsis_project_record_inspect(const uint8_t *record, uint32_t n,
                                 ApsisProjectRecordInfo *info);
int apsis_project_record_decode(const uint8_t *record, uint32_t n,
                                ApsisKeepBank *keep,
                                ApsisChainSequence *chain,
                                ApsisChainState *chain_settings,
                                uint32_t *generation);

/* Sequence-number ordering with wraparound. Equal generations are not newer. */
uint8_t apsis_project_generation_newer(uint32_t candidate, uint32_t reference);

/* Returns -1 if neither record is valid, otherwise 0 for A or 1 for B. */
int8_t apsis_project_journal_latest(const uint8_t *a, uint32_t a_n,
                                    const uint8_t *b, uint32_t b_n,
                                    uint32_t *generation);

#endif
